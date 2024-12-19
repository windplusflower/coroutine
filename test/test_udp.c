#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "coheader.h"
#include "hook.h"

#define PORT 12345
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"

int sockfd;
void *send_coroutine(const void *arg) {
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    while (1) {
        co_sleep(1);
        snprintf(buffer, BUFFER_SIZE, "Hello from sender: %ld", time(NULL));
        ssize_t sent_bytes = sendto(sockfd, buffer, strlen(buffer), 0,
                                    (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (sent_bytes < 0) {
            perror("sendto failed");
            continue;
        }

        printf("[Sender] Sent: %s\n", buffer);
    }
    return NULL;
}

void *recv_coroutine(const void *arg) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    while (1) {
        //接收阻塞时，会自动让出cpu
        ssize_t recv_bytes = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                                      (struct sockaddr *)&client_addr, &addr_len);
        if (recv_bytes < 0) {
            perror("recvfrom failed");
            continue;
        }

        buffer[recv_bytes] = '\0';
        printf("[Receiver] Received: %s\n", buffer);
    }
    return NULL;
}

int main() {
    enable_hook();
    struct sockaddr_in local_addr;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(PORT);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    coroutine_t sender, receiver;
    sender = coroutine_create(send_coroutine, "sender", 0);
    receiver = coroutine_create(recv_coroutine, "receiver", 0);
    coroutine_join(sender);
    coroutine_join(receiver);
    close(sockfd);
    return 0;
}
/*正确输出：每隔一秒输出一条
[Sender] Sent: Hello from sender: 1734508896
[Receiver] Received: Hello from sender: 1734508896
[Sender] Sent: Hello from sender: 1734508897
[Receiver] Received: Hello from sender: 1734508897
[Sender] Sent: Hello from sender: 1734508898
[Receiver] Received: Hello from sender: 1734508898
[Sender] Sent: Hello from sender: 1734508899
[Receiver] Received: Hello from sender: 1734508899
[Sender] Sent: Hello from sender: 1734508900
[Receiver] Received: Hello from sender: 1734508900
...
*/