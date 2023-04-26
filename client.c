#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
//
#define PORT 8000
#define BUFFER_SIZE 1024

pthread_mutex_t mutex;


void *recieve_messages(void *);

int main(int argc, char const *argv[]) {
    pthread_mutex_init(&mutex, NULL);

    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char message[BUFFER_SIZE] = {0};
    char username[100];

    // Create socket file descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    // Initialize server address to all zeroes
    memset(&serv_addr, '0', sizeof(serv_addr));

    // Set socket parameters
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    // Create a thread to receive messages from server
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, recieve_messages, (void *)&sock) < 0) {
            perror("Thread creation failed");
            exit(EXIT_FAILURE);
    }

    printf("[%d] Enter your username: ", getpid());
    scanf("%s", username);
    // fgets(username, sizeof(username), stdin);


    while (1) {
        printf("Enter message (type 'exit' to quit): ");
        fgets(message, sizeof(message), stdin);

        if (strncmp(message, "exit", 4) == 0) {
            break;
        }

        // Format the message and store it in the buffer
        sprintf(buffer, "%s: %s", username, message);

        // printf("\n\tSending to %d\n", sock);
        // pthread_mutex_lock(&mutex);
        // Send message to server
        send(sock, buffer, strlen(buffer), 0);
        // pthread_mutex_unlock(&mutex);

        // Clear buffer
        memset(buffer, 0, sizeof(buffer));
    }

    // Close socket
    close(sock);

    return 0;
}


// ? THREAD 
void *recieve_messages(void *socket_ptr) {
    int socket = *(int *)socket_ptr;
    char buffer[BUFFER_SIZE];
    int valread;

    
    while(1) {
        // printf("\n\tReceiving from %d\n", socket);
        // pthread_mutex_lock(&mutex);
        // valread = read(socket, buffer, BUFFER_SIZE);
        valread = recv(socket, buffer, BUFFER_SIZE, 0);
        // pthread_mutex_unlock(&mutex);

        if(valread < 0) {
            perror("Server Disconnected");
            exit(EXIT_FAILURE);
        } 
        printf("[%d]: %s", socket, buffer);
        memset(buffer, 0, BUFFER_SIZE);
    }
     

    close(socket);

    pthread_detach(pthread_self());
    
    return NULL;
}