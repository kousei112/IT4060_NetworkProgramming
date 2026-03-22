#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

/*
 * sv_client <địa chỉ IP> <cổng>
 *
 * - Kết nối đến sv_server
 * - Nhập thông tin sinh viên: MSSV, họ tên, ngày sinh, điểm TB
 * - Đóng gói thành 1 dòng: "MSSV HoTen NgaySinh DiemTB"
 * - Gửi sang sv_server
 * - Hỏi người dùng có muốn nhập tiếp không
 */

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

    /* --- Tạo socket TCP --- */
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) {
        perror("socket() failed");
        return 1;
    }

    /* --- Khai báo địa chỉ server --- */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid IP address: %s\n", ip);
        close(sock);
        return 1;
    }

    /* --- Kết nối đến server --- */
    printf("Connecting to %s:%d ...\n", ip, port);
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect() failed");
        close(sock);
        return 1;
    }
    printf("Connected!\n\n");

    /* --- Vòng lặp nhập và gửi thông tin sinh viên --- */
    char choice;
    do {
        char mssv[20];
        char hoten[100];
        char ngaysinh[20];  /* định dạng YYYY-MM-DD */
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
        while (getchar() != '\n'); /* xóa newline còn lại */

        /*
         * Đóng gói thành 1 dòng theo định dạng:
         * "MSSV HoTen NgaySinh DiemTB"
         * Ví dụ: "20201234 Nguyen Van A 2002-04-10 3.99"
         */
        char packet[256];
        snprintf(packet, sizeof(packet),
                 "%s %s %s %.2f",
                 mssv, hoten, ngaysinh, diemtb);

        printf("\nDu lieu se gui: [%s]\n", packet);

        /* --- Gửi đến server --- */
        ssize_t sent = send(sock, packet, strlen(packet), 0);
        if (sent == -1) {
            perror("send() failed");
            break;
        }
        printf("Da gui %zd bytes.\n", sent);

        /* --- Nhận phản hồi từ server --- */
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

        /* --- Hỏi tiếp tục không --- */
        printf("\nNhap tiep sinh vien khac? (y/n): ");
        scanf(" %c", &choice);
        while (getchar() != '\n');
        printf("\n");

    } while (choice == 'y' || choice == 'Y');

    /* Gửi tín hiệu kết thúc */
    send(sock, "QUIT", 4, 0);

    close(sock);
    printf("Da dong ket noi.\n");
    return 0;
}