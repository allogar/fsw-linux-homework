// Compile: gcc -pthread -std=c11 -Wall -O2 client1.c -o client1
// Execute: ./client1 0.0.0.0 4001 0.0.0.0 4002 0.0.0.0 4003
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>  // for nanosleep


#define NUM_OF_THREADS  3
#define DEFAULT_VALUE   "--"

// State of each client TCP thread
typedef struct {
    char last_value[4096];              // last value received from server TCP port
    pthread_mutex_t lock;               // thread mutex: concurrent access protection to last_value
} thread_state_t;

typedef struct {
    char ip[64];
    int port;
    int id;
} thread_args_t;

// Global variables
int g_sockets[NUM_OF_THREADS];              // one socket for each thread
thread_state_t g_states[NUM_OF_THREADS];    // one state for each thread
pthread_mutex_t g_snapshot_lock = PTHREAD_MUTEX_INITIALIZER;  // global mutex

volatile sig_atomic_t g_stop_flag = 0;      // Stop signal

// Handler SIGINT: To detect CTRL-C
void handle_sigint(int sig) {
    g_stop_flag = 1;

    // Close all registered sockets (async-signal-safe)
    for (int i = 0; i < NUM_OF_THREADS; i++) {
        if (g_sockets[i] > 0) {
            close(g_sockets[i]);
            g_sockets[i] = -1;
        }
    }
}

void setup_sigint_handler() {
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
}

void *connection_thread(void *arg) {
    thread_args_t *t = (thread_args_t *)arg;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        fprintf(stderr, "[Thread %d] Error in socket: %s\n", t->id, strerror(errno));
        return NULL;
    }

    // Register socker globally
    g_sockets[t->id] = sock;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(t->port);

    if (inet_pton(AF_INET, t->ip, &addr.sin_addr) <= 0) {
        fprintf(stderr, "[Thread %d] inet_pton failed: %s\n", t->id, strerror(errno));
        close(sock);
        g_sockets[t->id] = -1;
        return NULL;
    }

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "[Thread %d] Error when connecting to socket: %s\n", t->id, strerror(errno));
        close(sock);
        g_sockets[t->id] = -1;
        return NULL;
    }

    char buffer[4096];
    ssize_t n;

    // Received value update loop
    while (!g_stop_flag && (n = read(sock, buffer, sizeof(buffer))) > 0) {
        // Safety storage of last received value
        pthread_mutex_lock(&g_snapshot_lock); // Global lock
        pthread_mutex_lock(&g_states[t->id].lock); // Thread lock

        size_t copy_len = (n < sizeof(g_states[t->id].last_value) - 1) ? n : sizeof(g_states[t->id].last_value) - 1;
        memcpy(g_states[t->id].last_value, buffer, copy_len);
        g_states[t->id].last_value[copy_len] = '\0';

        pthread_mutex_unlock(&g_states[t->id].lock);
        pthread_mutex_unlock(&g_snapshot_lock);
    }

     if (!g_stop_flag && n < 0) {
        fprintf(stderr, "[Thread %d] Error when reading from socket: %s\n", t->id, strerror(errno));
    }

    close(sock);
    g_sockets[t->id] = -1;

    printf("[Thread %d] Finishing...\n", t->id);

    return NULL;
}

void main_loop_read_states(int num_ths) {
    const long period_ns = 100 * 1000000L; // 1 ms = 1,000,000 ns
    struct timespec ts_next;

    // Using Fixed Absolute Schedule for accurate timing
    // Initial time: first execution
    clock_gettime(CLOCK_REALTIME, &ts_next);

    while (!g_stop_flag) {
        // Current loop timestamp (ms)
        long long ms_since_epoch = ts_next.tv_sec * 1000LL + ts_next.tv_nsec / 1000000LL;
        printf("[Main] %lld ms\n", ms_since_epoch);

        pthread_mutex_lock(&g_snapshot_lock); // Global lock

        for (int id = 0; id < num_ths; id++) {
            char value_copy[4096];

            pthread_mutex_lock(&g_states[id].lock); // Thread lock
            
            // Copy value
            strncpy(value_copy, g_states[id].last_value, sizeof(value_copy));
            value_copy[sizeof(value_copy)-1] = '\0';
            
            // Reset value
            strncpy(g_states[id].last_value, DEFAULT_VALUE, sizeof(g_states[id].last_value));
            g_states[id].last_value[sizeof(g_states[id].last_value)-1] = '\0';
            
            pthread_mutex_unlock(&g_states[id].lock);

            printf("[Main] Thread %d value: %s\n", id, value_copy);
        }

        pthread_mutex_unlock(&g_snapshot_lock);

        // Next execution time
        ts_next.tv_nsec += period_ns;
        // Normalize to avoid tv_nsec overflow
        while (ts_next.tv_nsec >= 1000000000L) {
            ts_next.tv_nsec -= 1000000000L;
            ts_next.tv_sec++;
        }

        // Wait until next execution absolute time
        clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &ts_next, NULL);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3 || (argc % 2) == 0) {
        fprintf(stderr, "Usage: %s <ip_address1> <port_no1> [<ip_address2> <port_no2> ...]\n", argv[0]);
        return 1;
    }

    int nthreads = (argc - 1) / 2;
    if (nthreads > NUM_OF_THREADS) {
        fprintf(stderr, "Error: Max number of threads must be %d\n", NUM_OF_THREADS);
        return 1;
    }

    setup_sigint_handler();

    // Init sockets
    for (int i = 0; i < NUM_OF_THREADS; i++) {
        g_sockets[i] = -1;
    }

    // Init thread states
    for (int i = 0; i < NUM_OF_THREADS; i++) {
        pthread_mutex_init(&g_states[i].lock, NULL);
        strncpy(g_states[i].last_value, DEFAULT_VALUE, sizeof(g_states[i].last_value));
        g_states[i].last_value[sizeof(g_states[i].last_value)-1] = '\0';
    }

    pthread_t threads[nthreads];
    thread_args_t *targs[nthreads];

    for (int i = 0; i < nthreads; i++) {
        targs[i] = malloc(sizeof(thread_args_t));

        snprintf(targs[i]->ip, sizeof(targs[i]->ip), "%s", argv[1 + i * 2]);
        targs[i]->port = atoi(argv[2 + i * 2]);
        targs[i]->id = i;

        if (pthread_create(&threads[i], NULL, connection_thread, targs[i]) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i);
            return 1;
        }
    }

    // Main loop: read variables every 100 ms
    main_loop_read_states(nthreads);

    // In case of CTRL-C: Wait for all threads to finish
    for (int i = 0; i < nthreads; i++) {
        pthread_join(threads[i], NULL);
        free(targs[i]);
    }

    printf("Process finished!.\n");

    return 0;
}
