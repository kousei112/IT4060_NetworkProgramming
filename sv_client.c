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

    char choice;
    do {
        char mssv[20];
        char hoten[100];
        char ngaysinh[20];  
        float diemtb;

        printf("========== NHAP THONG TIN SINH VIEN ==========\n");

        printf("MSSV       : ");
        fgets(mssv, sizeof(mssv), stdin);
        mssv[strcspn(mssv, "\n")] = '\0';

        printf("Ho ten     : ");
        fgets(hoten, sizeof(hoten), stdin);
        hoten[strcspn(hoten, "\n")] = '\0';

        printf("Ngay sinh  (YYYY-MM-DD): ");
        fgets(ngaysinh, sizeof(ngaysinh), stdin);
        ngaysinh[strcspn(ngaysinh, "\n")] = '\0';

        printf("Diem TB    : ");
        scanf("%f", &diemtb);
        while (getchar() != '\n');
        char packet[256];
        snprintf(packet, sizeof(packet),
                 "%s %s %s %.2f",
                 mssv, hoten, ngaysinh, diemtb);

        printf("\nDu lieu se gui: [%s]\n", packet);
        ssize_t sent = send(sock, packet, strlen(packet), 0);
        if (sent == -1) {
            perror("send() failed");
            break;
        }
        printf("Da gui %zd bytes.\n", sent);

        char resp[512];
        ssize_t rlen = recv(sock, resp, sizeof(resp) - 1, 0);
        if (rlen > 0) {
            resp[rlen] = '\0';
            printf("[Server]: %s\n", resp);
        } else if (rlen == 0) {
            printf("Server dong ket noi.\n");
            break;
        } else {
            perror("recv() failed");
            break;
        }

        printf("\nNhap tiep sinh vien khac? (y/n): ");
        scanf(" %c", &choice);
        while (getchar() != '\n');
        printf("\n");

    } while (choice == 'y' || choice == 'Y');

    send(sock, "QUIT", 4, 0);

    close(sock);
    printf("Da dong ket noi.\n");
    return 0;
}
