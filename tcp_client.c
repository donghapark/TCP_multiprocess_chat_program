#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define TCP_PORT 5100
#define MAX_NAME_LEN 128
#define MAX_MSG_LEN 896

// 서버로부터 데이터를 수신하는 스레드 함수
void *receive_thread(void *socket) {
    int sock = *(int *)socket;
    char buffer[1024];

    while (1) {
        int received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {
            printf("Connection closed or error occurred.\n");
            break;
        }
        buffer[received] = '\0';
        printf("Received: %s\n", buffer);
    }

    return NULL;
}

int main(int argc, char **argv) {
    int sock;
    struct sockaddr_in servaddr;
    char send_msg[MAX_MSG_LEN], recv_msg[1024];
    pthread_t recv_thread_id;

    if (argc < 2) {
        printf("Usage: %s IP_ADDRESS\n", argv[0]);
        return -1;
    }

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(TCP_PORT);
    inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

    // Connect to server
    if (connect(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Connection failed");
        close(sock);
        return -1;
    }

    printf("Enter your name: ");
    fgets(send_msg, MAX_NAME_LEN, stdin);
    send_msg[strcspn(send_msg, "\n")] = '\0'; // Remove newline

    // Send name to server
    if (send(sock, send_msg, strlen(send_msg), 0) < 0) {
        perror("Failed to send name");
        close(sock);
        return -1;
    }

    // Start the receiving thread
    pthread_create(&recv_thread_id, NULL, receive_thread, &sock);

    // Send messages to server
    while (fgets(send_msg, MAX_MSG_LEN, stdin) != NULL) {
        if (send(sock, send_msg, strlen(send_msg), 0) < 0) {
            perror("Failed to send message");
            break;
        }
    }

    // Cleanup
    pthread_join(recv_thread_id, NULL);
    close(sock);
    return 0;
}