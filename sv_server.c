#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

/*
 * sv_server <cổng> <file_log>
 *
 * - Lắng nghe kết nối tại <cổng>
 * - Nhận gói tin từ sv_client (định dạng: "MSSV HoTen NgaySinh DiemTB")
 * - In ra màn hình thông tin sinh viên
 * - Ghi vào <file_log> theo định dạng:
 *   <IP> <YYYY-MM-DD> <HH:MM:SS> <MSSV> <HoTen> <NgaySinh> <DiemTB>
 *   Ví dụ: 127.0.0.1 2023-04-10 09:00:00 20201234 Nguyen Van A 2002-04-10 3.99
 * - Gửi phản hồi xác nhận cho client
 */

/* Lấy thời gian hiện tại dạng "YYYY-MM-DD HH:MM:SS" */
void get_timestamp(char *buf, size_t bufsize)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buf, bufsize, "%Y-%m-%d %H:%M:%S", t);
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <log_file>\n", argv[0]);
        return 1;
    }

    int         port     = atoi(argv[1]);
    const char *log_file = argv[2];

    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port: %s\n", argv[1]);
        return 1;
    }

    /* --- Tạo socket TCP --- */
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* --- Gắn địa chỉ --- */
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

    printf("==========================================\n");
    printf("  SV_SERVER lang nghe cong  : %d\n", port);
    printf("  File log                  : %s\n", log_file);
    printf("  Nhan Ctrl+C de dung.\n");
    printf("==========================================\n\n");

    /* --- Vòng lặp chấp nhận client --- */
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        printf("Dang cho ket noi...\n");

        int client = accept(listener,
                            (struct sockaddr *)&client_addr,
                            &client_len);
        if (client == -1) {
            perror("accept() failed");
            continue;
        }

        /* Lấy IP client */
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr,
                  client_ip, sizeof(client_ip));

        printf("------------------------------------------\n");
        printf("Client ket noi: %s:%d\n",
               client_ip, ntohs(client_addr.sin_port));

        /* --- Vòng lặp nhận gói tin --- */
        char buf[512];
        int  count = 0;

        while (1) {
            ssize_t received = recv(client, buf, sizeof(buf) - 1, 0);

            if (received <= 0) {
                if (received == 0)
                    printf("Client ngat ket noi.\n");
                else
                    perror("recv() failed");
                break;
            }

            buf[received] = '\0';

            /* Kiểm tra tín hiệu kết thúc */
            if (strcmp(buf, "QUIT") == 0) {
                printf("Nhan lenh QUIT. Ket thuc phien.\n");
                break;
            }

            count++;

            /*
             * Gói tin có định dạng: "MSSV HoTen NgaySinh DiemTB"
             * HoTen có thể gồm nhiều từ, nên tách từ cuối (DiemTB)
             * và từ áp cuối (NgaySinh) trước, phần còn lại là HoTen
             *
             * Thuật toán: tách token từ cuối chuỗi
             */
            char data[512];
            strncpy(data, buf, sizeof(data) - 1);
            data[sizeof(data) - 1] = '\0';

            /* Tách DiemTB - token cuối cùng */
            char *last_space = strrchr(data, ' ');
            if (!last_space) {
                printf("Goi tin khong hop le: %s\n", buf);
                send(client, "Loi: Du lieu khong hop le.", 26, 0);
                continue;
            }
            char diemtb_str[20];
            strncpy(diemtb_str, last_space + 1, sizeof(diemtb_str) - 1);
            diemtb_str[sizeof(diemtb_str) - 1] = '\0';
            float diemtb = atof(diemtb_str);
            *last_space = '\0';  /* cắt DiemTB ra khỏi data */

            /* Tách NgaySinh - token áp cuối */
            last_space = strrchr(data, ' ');
            if (!last_space) {
                printf("Goi tin khong hop le: %s\n", buf);
                send(client, "Loi: Du lieu khong hop le.", 26, 0);
                continue;
            }
            char ngaysinh[20];
            strncpy(ngaysinh, last_space + 1, sizeof(ngaysinh) - 1);
            ngaysinh[sizeof(ngaysinh) - 1] = '\0';
            *last_space = '\0';  /* cắt NgaySinh ra khỏi data */

            /* Tách MSSV - token đầu tiên */
            char *space_after_mssv = strchr(data, ' ');
            if (!space_after_mssv) {
                printf("Goi tin khong hop le: %s\n", buf);
                send(client, "Loi: Du lieu khong hop le.", 26, 0);
                continue;
            }
            char mssv[20];
            size_t mssv_len = space_after_mssv - data;
            if (mssv_len >= sizeof(mssv)) mssv_len = sizeof(mssv) - 1;
            strncpy(mssv, data, mssv_len);
            mssv[mssv_len] = '\0';

            /* Phần còn lại là HoTen */
            char *hoten = space_after_mssv + 1;

            /* Lấy timestamp lúc nhận */
            char timestamp[32];
            get_timestamp(timestamp, sizeof(timestamp));

            /* --- In ra màn hình --- */
            printf("\n========== SINH VIEN #%d ==========\n", count);
            printf("  Thoi gian : %s\n", timestamp);
            printf("  IP client : %s\n", client_ip);
            printf("  MSSV      : %s\n", mssv);
            printf("  Ho ten    : %s\n", hoten);
            printf("  Ngay sinh : %s\n", ngaysinh);
            printf("  Diem TB   : %.2f\n", diemtb);
            printf("====================================\n");

            /* --- Ghi vào file log theo định dạng đề bài ---
             * <IP> <YYYY-MM-DD> <HH:MM:SS> <MSSV> <HoTen> <NgaySinh> <DiemTB>
             * Ví dụ: 127.0.0.1 2023-04-10 09:00:00 20201234 Nguyen Van A 2002-04-10 3.99
             */
            FILE *lf = fopen(log_file, "a");
            if (lf == NULL) {
                perror("Cannot open log file");
            } else {
                fprintf(lf, "%s %s %s %s %s %.2f\n",
                        client_ip,
                        timestamp,   /* "YYYY-MM-DD HH:MM:SS" */
                        mssv,
                        hoten,
                        ngaysinh,
                        diemtb);
                fclose(lf);
                printf("Da ghi vao file log: %s\n", log_file);
            }

            /* --- Gửi phản hồi cho client --- */
            char resp[256];
            snprintf(resp, sizeof(resp),
                     "Da nhan SV #%d: [%s] %s - %s - Diem TB: %.2f",
                     count, mssv, hoten, ngaysinh, diemtb);
            send(client, resp, strlen(resp), 0);
        }

        printf("Tong so SV nhan trong phien nay: %d\n", count);
        printf("------------------------------------------\n\n");

        close(client);
    }

    close(listener);
    return 0;
}