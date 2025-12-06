// Compile: gcc -std=c11 -Wall -O2 client1.c -o client1
// Execute: ./client1 0.0.0.0 400x
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>


int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <ip_address> <port_no>\n", argv[0]);
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        fprintf(stderr, "Error in socket: %s\n", strerror(errno));
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        fprintf(stderr, "inet_pton failed: %s\n", strerror(errno));
        close(sock);
        return 1;
    }

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "Error when connecting to socket: %s\n", strerror(errno));
        close(sock);
        return 1;
    }

    char buffer[4096];
    ssize_t n;

    // Read from socket and write to stdout
    while ((n = read(sock, buffer, sizeof(buffer))) > 0) {
        if (write(STDOUT_FILENO, buffer, n) < 0) {
            fprintf(stderr, "Error when writing to stdout: %s\n", strerror(errno));
            close(sock);
            return 1;
        }
    }

    if (n < 0) {
        fprintf(stderr, "Error when reading from socket: %s\n", strerror(errno));
    }

    close(sock);
    return 0;
}
