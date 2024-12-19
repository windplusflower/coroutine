#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "coheader.h"

int PORT = 1000;
#define BUFFER_SIZE 1024

void send_coroutine(const void* arg) {
    int server_sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    int client_sock;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return;
    }

    if (listen(server_sock, 3) < 0) {
        perror("Listen failed");
        return;
    }

    printf("Server is listening on port %d\n", PORT);
    if ((client_sock = accept(server_sock, (struct sockaddr*)&server_addr, &addr_len)) < 0) {
        perror("Accept failed");
        return;
    }
    printf("accept success\n");
    while (1) {
        snprintf(buffer, BUFFER_SIZE, "Message from server thread(%ld)", time(0));
        if (send(client_sock, buffer, strlen(buffer), 0) < 0) {
            perror("Send failed");
            break;
        }
        co_sleep(1);
    }

    close(client_sock);
    close(server_sock);
    return;
}

void recv_coroutine(const void* arg) {
    int client_sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    if ((client_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (connect(client_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return;
    }
    printf("connect success\n");
    while (1) {
        int bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received < 0) {
            perror("Receive failed");
            break;
        }
        buffer[bytes_received] = '\0';
        printf("Received: %s\n", buffer);
    }

    close(client_sock);
    return;
}

int main() {
    enable_hook();
    srand(time(0));
    PORT += rand() % 10000;
    coroutine_t sender, receiver;
    sender = coroutine_create(send_coroutine, "sender", 0);
    receiver = coroutine_create(recv_coroutine, "receiver", 0);
    coroutine_join(sender);
    coroutine_join(receiver);
    return 0;
}
/*正确输出：

Server is listening on port 5782
accept success
connect success
Received: Message from server thread(1734509288)
Received: Message from server thread(1734509289)
Received: Message from server thread(1734509290)
Received: Message from server thread(1734509291)
Received: Message from server thread(1734509292)
...
*/
