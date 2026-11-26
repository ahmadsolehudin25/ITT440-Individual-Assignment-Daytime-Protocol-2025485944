#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define PORT 13
#define BUFFER_SIZE 256

// Extended Feature: Format time based on client request
void get_formatted_time(const char *mode, char *output, size_t max_len) {
    time_t now = time(NULL);
    struct tm *t;

    if (strncmp(mode, "UTC", 3) == 0) {
        t = gmtime(&now);
        strftime(output, max_len, "%A, %B %d, %Y %H:%M:%S UTC\r\n", t);
    } else if (strncmp(mode, "ISO", 3) == 0) {
        t = localtime(&now);
        strftime(output, max_len, "%Y-%m-%dT%H:%M:%S+08:00 (ISO-8601)\r\n", t);
    } else if (strncmp(mode, "UNIX", 4) == 0) {
        snprintf(output, max_len, "Unix Epoch Timestamp: %ld\r\n", (long)now);
    } else {
        // Default RFC 867 Daytime Format (Local Malaysia Time)
        t = localtime(&now);
        strftime(output, max_len, "%A, %B %d, %Y %H:%M:%S MYT\r\n", t);
    }
}

int main() {
    int tcp_fd, udp_fd, client_fd, max_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    char time_str[BUFFER_SIZE];
    fd_set read_fds;

    // 1. Create TCP Socket (SOCK_STREAM)
    if ((tcp_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("TCP socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(tcp_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 2. Create UDP Socket (SOCK_DGRAM)
    if ((udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("UDP socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 3. Configure Server Address (Port 13)
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 4. Bind BOTH TCP and UDP sockets to Port 13
    if (bind(tcp_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("TCP bind failed (Did you run with sudo?)");
        exit(EXIT_FAILURE);
    }
    if (bind(udp_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("UDP bind failed");
        exit(EXIT_FAILURE);
    }

    // 5. Listen for incoming TCP connections
    if (listen(tcp_fd, 5) < 0) {
        perror("TCP listen failed");
        exit(EXIT_FAILURE);
    }

    printf("==================================================\n");
    printf("  RFC 867 DAYTIME SERVER (TCP & UDP) - PORT %d\n", PORT);
    printf("  Running on Ubuntu WSL | Waiting for clients...\n");
    printf("==================================================\n\n");

    max_fd = (tcp_fd > udp_fd) ? tcp_fd : udp_fd;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(tcp_fd, &read_fds);
        FD_SET(udp_fd, &read_fds);

        // Monitor both TCP and UDP simultaneously
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("Select error");
            continue;
        }

        // --- HANDLE TCP CLIENT ---
        if (FD_ISSET(tcp_fd, &read_fds)) {
            client_fd = accept(tcp_fd, (struct sockaddr *)&client_addr, &addr_len);
            if (client_fd >= 0) {
                // Wait up to 50ms for optional format mode (UTC, ISO, UNIX)
                struct timeval tv = {0, 50000};
                setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

                memset(buffer, 0, BUFFER_SIZE);
                recv(client_fd, buffer, BUFFER_SIZE - 1, 0);

                get_formatted_time(buffer, time_str, sizeof(time_str));
                send(client_fd, time_str, strlen(time_str), 0);

                printf("[TCP] Client %s:%d connected -> Sent: %s",
                       inet_ntoa(client_addr.sin_addr),
                       ntohs(client_addr.sin_port),
                       time_str);

                close(client_fd);
            }
        }

        // --- HANDLE UDP CLIENT ---
        if (FD_ISSET(udp_fd, &read_fds)) {
            memset(buffer, 0, BUFFER_SIZE);
            int n = recvfrom(udp_fd, buffer, BUFFER_SIZE - 1, 0,
                             (struct sockaddr *)&client_addr, &addr_len);
            if (n >= 0) {
                get_formatted_time(buffer, time_str, sizeof(time_str));
                sendto(udp_fd, time_str, strlen(time_str), 0,
                       (struct sockaddr *)&client_addr, addr_len);

                printf("[UDP] Datagram from %s:%d -> Sent: %s",
                       inet_ntoa(client_addr.sin_addr),
                       ntohs(client_addr.sin_port),
                       time_str);
            }
        }
    }

    close(tcp_fd);
    close(udp_fd);
    return 0;
}