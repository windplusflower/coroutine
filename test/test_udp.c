#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "coroutine.h"
#include "hook.h"
#include "utils.h"

#define PORT 12345
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"

int sockfd;
void send_coroutine(void *arg) {
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    while (1) {
        //这里的sleep不会让出cpu，只是为了让输出慢一点，不刷屏。
        sleep(1);
        snprintf(buffer, BUFFER_SIZE, "Hello from sender: %ld", time(NULL));
        ssize_t sent_bytes = co_sendto(sockfd, buffer, strlen(buffer), 0,
                                       (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (sent_bytes < 0) {
            perror("sendto failed");
            continue;
        }

        printf("[Sender] Sent: %s\n", buffer);
        //发送不会阻塞，所以需要手动让出
        coroutine_yield();
    }
}

void recv_coroutine(void *arg) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    while (1) {
        //这里的sleep不会让出cpu，只是为了让输出慢一点，不刷屏。
        sleep(1);
        ssize_t recv_bytes = co_recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                                         (struct sockaddr *)&client_addr, &addr_len);
        if (recv_bytes < 0) {
            perror("recvfrom failed");
            continue;
        }

        buffer[recv_bytes] = '\0';
        printf("[Receiver] Received: %s\n", buffer);
        //接收阻塞时，会自动让出cpu
    }
}

int main() {
    log_set_level_from_env();
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

    Coroutine sender, receiver;
    coroutine_init(&sender, send_coroutine, "sender", STACKSIZE);
    coroutine_init(&receiver, recv_coroutine, "receiver", STACKSIZE);
    start_eventloop();
    while (1) coroutine_yield();
    close(sockfd);
    return 0;
}
/*正确输出：

[Sender] Sent: Hello from sender: 1733905953
[Receiver] Received: Hello from sender: 1733905953
[Sender] Sent: Hello from sender: 1733905955
[Receiver] Received: Hello from sender: 1733905955
[Sender] Sent: Hello from sender: 1733905957
[Receiver] Received: Hello from sender: 1733905957
[Sender] Sent: Hello from sender: 1733905959
[Receiver] Received: Hello from sender: 1733905959
...
*/