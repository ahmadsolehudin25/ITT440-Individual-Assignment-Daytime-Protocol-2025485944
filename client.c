#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 13
#define BUFFER_SIZE 256

void request_daytime(const char *server_ip, int use_udp, const char *format_mode) {
    SOCKET sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    LARGE_INTEGER freq, start, end;

    QueryPerformanceFrequency(&freq);

    // Create TCP or UDP socket
    if (use_udp) {
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    } else {
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }

    if (sock == INVALID_SOCKET) {
        printf("[!] Socket creation failed. Error: %d\n", WSAGetLastError());
        return;
    }

    // Set 3-second receive timeout
    DWORD timeout = 3000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    QueryPerformanceCounter(&start);

    if (!use_udp) {
        // --- TCP FLOW ---
        if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            printf("[!] TCP Connection failed! Make sure Ubuntu server is running.\n");
            closesocket(sock);
            return;
        }
        send(sock, format_mode, strlen(format_mode), 0);

        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        QueryPerformanceCounter(&end);

        if (bytes > 0) {
            double rtt = (double)(end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;
            printf("\n[+] TCP Response from %s:%d (RTT: %.2f ms)\n", server_ip, PORT, rtt);
            printf("    Server Time: %s\n", buffer);
        } else {
            printf("[!] No response received.\n");
        }
    } else {
        // --- UDP FLOW ---
        int server_len = sizeof(server_addr);
        sendto(sock, format_mode, strlen(format_mode), 0,
               (struct sockaddr *)&server_addr, server_len);

        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recvfrom(sock, buffer, BUFFER_SIZE - 1, 0,
                             (struct sockaddr *)&server_addr, &server_len);
        QueryPerformanceCounter(&end);

        if (bytes > 0) {
            double rtt = (double)(end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;
            printf("\n[+] UDP Response from %s:%d (RTT: %.2f ms)\n", server_ip, PORT, rtt);
            printf("    Server Time: %s\n", buffer);
        } else {
            printf("[!] UDP Request timed out! Make sure Ubuntu server is running.\n");
        }
    }

    closesocket(sock);
}

int main() {
    WSADATA wsa;
    char server_ip[64] = "127.0.0.1";
    char input_ip[64];
    int choice;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    printf("==================================================\n");
    printf("   DAYTIME PROTOCOL CLIENT (RFC 867) - WINDOWS\n");
    printf("==================================================\n");
    printf("Enter Ubuntu Server IP (Press Enter for 127.0.0.1): ");
    
    if (fgets(input_ip, sizeof(input_ip), stdin) != NULL) {
        input_ip[strcspn(input_ip, "\r\n")] = 0;
        if (strlen(input_ip) > 0) {
            strncpy(server_ip, input_ip, sizeof(server_ip) - 1);
            server_ip[sizeof(server_ip) - 1] = '\0';
        }
    }

    while (1) {
        printf("\n--- Select Daytime Query Mode ---\n");
        printf("1. Standard Daytime via TCP (Malaysia Time)\n");
        printf("2. Standard Daytime via UDP (Malaysia Time)\n");
        printf("3. Extended: Get UTC Time via TCP\n");
        printf("4. Extended: Get ISO-8601 Timestamp via UDP\n");
        printf("5. Extended: Get Unix Epoch Time via TCP\n");
        printf("0. Exit\n");
        printf("Choice: ");
        
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            continue;
        }
        while (getchar() != '\n');

        if (choice == 0) break;

        switch (choice) {
            case 1: request_daytime(server_ip, 0, "MYT"); break;
            case 2: request_daytime(server_ip, 1, "MYT"); break;
            case 3: request_daytime(server_ip, 0, "UTC"); break;
            case 4: request_daytime(server_ip, 1, "ISO"); break;
            case 5: request_daytime(server_ip, 0, "UNIX"); break;
            default: printf("Invalid option.\n");
        }
    }

    WSACleanup();
    return 0;
}
    