#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP address> <port>\n", argv[0]);
        return 1;
    }

    const char *ip   = argv[1];
    int         port = atoi(argv[2]);

    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port: %s\n", argv[2]);
        return 1;
    }
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) {
        perror("socket() failed");
        return 1;
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid IP address: %s\n", ip);
        close(sock);
        return 1;
    }
    printf("Connecting to %s:%d ...\n", ip, port);
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect() failed");
        close(sock);
        return 1;
    }
    printf("Connected!\n\n");
    char greeting[4096];
    ssize_t glen = recv(sock, greeting, sizeof(greeting) - 1, 0);
    if (glen > 0) {
        greeting[glen] = '\0';
        printf("----------------------------------\n");
        printf("[Server]: %s\n", greeting);
        printf("----------------------------------\n\n");
    }
    printf("Type messages (type 'exit' to quit):\n");
    char buf[1024];
    char resp[1100];

    while (1) {
        printf("> ");
        fflush(stdout);
        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            printf("\nEOF detected. Exiting...\n");
            break;
        }
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[--len] = '\0';
        }
        if (strcmp(buf, "exit") == 0) {
            printf("Exiting...\n");
            break;
        }
        if (len == 0) continue;
        ssize_t sent = send(sock, buf, len, 0);
        if (sent == -1) {
            perror("send() failed");
            break;
        }
        printf("Sent %zd bytes.\n", sent);
        ssize_t rlen = recv(sock, resp, sizeof(resp) - 1, 0);
        if (rlen > 0) {
            resp[rlen] = '\0';
            printf("%s\n\n", resp);
        } else if (rlen == 0) {
            printf("Server closed the connection.\n");
            break;
        } else {
            perror("recv() failed");
            break;
        }
    }

    close(sock);
    printf("Connection closed.\n");
    return 0;
}