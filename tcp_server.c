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
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <port> <greeting_file> <log_file>\n", argv[0]);
        return 1;
    }
    int         port          = atoi(argv[1]);
    const char *greeting_file = argv[2];
    const char *log_file      = argv[3];

    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port: %s\n", argv[1]);
        return 1;
    }
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port        = htons(port);
    if (bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind() failed");
        close(listener);
        return 1;
    }
    if (listen(listener, 5) == -1) {
        perror("listen() failed");
        close(listener);
        return 1;
    }
    printf("========================================\n");
    printf("  Server listening on port %d\n", port);
    printf("  Greeting file : %s\n", greeting_file);
    printf("  Log file      : %s\n", log_file);
    printf("  Press Ctrl+C to stop.\n");
    printf("========================================\n\n");
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        printf("Waiting for a new client...\n");

        int client = accept(listener,
                            (struct sockaddr *)&client_addr,
                            &client_addr_len);
        if (client == -1) {
            perror("accept() failed");
            continue;
        }
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("----------------------------------------\n");
        printf("Client connected: %s:%d\n",
               client_ip, ntohs(client_addr.sin_port));
        FILE *gf = fopen(greeting_file, "r");
        if (gf == NULL) {
            perror("Cannot open greeting file");
        } else {
            char greeting[4096];
            size_t glen = fread(greeting, 1, sizeof(greeting) - 1, gf);
            fclose(gf);

            if (glen > 0) {
                greeting[glen] = '\0';
                /* Bỏ newline cuối nếu có */
                if (greeting[glen - 1] == '\n')
                    greeting[--glen] = '\0';

                if (send(client, greeting, glen, 0) == -1)
                    perror("send() greeting failed");
                else
                    printf("Sent greeting (%zu bytes) to client.\n", glen);
            }
        }
        FILE *lf = fopen(log_file, "a");
        if (lf == NULL) {
            perror("Cannot open log file");
            close(client);
            continue;
        }

        fprintf(lf, "=== Session: %s:%d ===\n",
                client_ip, ntohs(client_addr.sin_port));

        char buf[1024];
        char response[1100];
        ssize_t received;
        size_t total = 0;

        printf("\n--- Messages from client ---\n");

        while (1) {
            received = recv(client, buf, sizeof(buf) - 1, 0);

            if (received <= 0) {
                if (received == 0)
                    printf("\nClient disconnected.\n");
                else
                    perror("recv() failed");
                break;
            }
            buf[received] = '\0';
            printf("[Client %s]: %s\n", client_ip, buf);
            fwrite(buf, 1, received, lf);
            total += received;
            snprintf(response, sizeof(response), "[Server echo]: %s", buf);
            if (send(client, response, strlen(response), 0) == -1) {
                perror("send() echo failed");
                break;
            }
        }

        fprintf(lf, "\n");
        fclose(lf);

        printf("Total received: %zu bytes. Saved to '%s'.\n", total, log_file);
        printf("----------------------------------------\n\n");

        close(client);
    }

    close(listener);
    return 0;
}