// Compile: gcc -pthread -std=c11 -Wall -O2 client1.c -o client1
// Execute: ./client1 0.0.0.0 4001 0.0.0.0 4002 0.0.0.0 4003
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>


typedef struct {
    char ip[64];
    int port;
    int id;
} thread_args_t;

void *connection_thread(void *arg) {
    thread_args_t *t = (thread_args_t *)arg;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        fprintf(stderr, "[Thread %d] Error in socket: %s\n", t->id, strerror(errno));
        return NULL;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(t->port);

    if (inet_pton(AF_INET, t->ip, &addr.sin_addr) <= 0) {
        fprintf(stderr, "[Thread %d] inet_pton failed: %s\n", t->id, strerror(errno));
        close(sock);
        return NULL;
    }

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "[Thread %d] Error when connecting to socket: %s\n", t->id, strerror(errno));
        close(sock);
        return NULL;
    }

    char buffer[4096];
    ssize_t n;

    // Read from socket and write to stdout
    while ((n = read(sock, buffer, sizeof(buffer))) > 0) {
        if (write(STDOUT_FILENO, buffer, n) < 0) {
            fprintf(stderr, "[Thread %d] Error when writing to stdout: %s\n", t->id, strerror(errno));
            close(sock);
            return NULL;
        }
    }

    if (n < 0) {
        fprintf(stderr, "[Thread %d] Error when reading from socket: %s\n", t->id, strerror(errno));
    }

    close(sock);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3 || (argc % 2) == 0) {
        fprintf(stderr, "Usage: %s <ip_address1> <port_no1> [<ip_address2> <port_no2> ...]\n", argv[0]);
        return 1;
    }

    int nthreads = (argc - 1) / 2;
    pthread_t *threads = malloc(sizeof(pthread_t) * nthreads);
    thread_args_t *targs = malloc(sizeof(thread_args_t) * nthreads);

    for (int i = 0; i < nthreads; i++) {
        strncpy(targs[i].ip, argv[1 + i*2], sizeof(targs[i].ip));
        targs[i].port = atoi(argv[2 + i*2]);
        targs[i].id = i;

        if (pthread_create(&threads[i], NULL, connection_thread, &targs[i]) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i);
            return 1;
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < nthreads; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(targs);

    return 0;
}
