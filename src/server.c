#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define PORT 8000
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

FILE *fptr;
pthread_mutex_t mutex;

int client_sockets[MAX_CLIENTS] = {0};
int num_clients = 0;

void *handle_client(void *);
void signal_handler(int sig);

int main()
{
    pthread_mutex_init(&mutex, NULL);

    signal(SIGINT, signal_handler);

    fptr = fopen("log.txt", "a");
    if (fptr == NULL)
    {
        printf("Error opening file.\n");
    }

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options to reuse address
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Initialize socket address structure
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket to address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, MAX_CLIENTS) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // ! Accept incoming connections and spawn threads to handle them
    while (num_clients < MAX_CLIENTS)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        printf("New connection from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

        pthread_t thread_id;
        int *client_fd = malloc(sizeof(int));
        *client_fd = new_socket;

        // Add new client socket to sockets list
        pthread_mutex_lock(&mutex);
        client_sockets[num_clients++] = new_socket;
        pthread_mutex_unlock(&mutex);

        if (pthread_create(&thread_id, NULL, handle_client, (void *)client_fd) < 0)
        {
            perror("Thread creation failed");
            exit(EXIT_FAILURE);
        }
    }

    // Close log file
    fclose(fptr);
    

    pthread_mutex_destroy(&mutex);

    return 0;
}

// ? THREAD
void *handle_client(void *socket_ptr)
{
    int socket = *(int *)socket_ptr;
    char buffer[BUFFER_SIZE];
    int valread;

    // ! Receive and display messages from client
    while ((valread = read(socket, buffer, BUFFER_SIZE)) > 0)
    {

        pthread_mutex_lock(&mutex);
        fprintf(fptr, "%s\n", buffer);
        fwrite(buffer, strlen(buffer), 1, fptr);
        pthread_mutex_unlock(&mutex);
        // printf("\n\tReceiving from %d\n", socket);

        printf("[%d]: %s", socket, buffer);

        pthread_mutex_lock(&mutex);
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (client_sockets[i] != 0 && client_sockets[i] != socket)
            {
                // send(socket, buffer, sizeof(buffer), 0);
                if (send(client_sockets[i], buffer, strlen(buffer), 0) < 0)
                {
                    printf("Error sending");
                    break;
                }
                // printf("\n\tsent %s to %d %d",buffer,  client_sockets[i], socket);
            }
        }
        // reset buffer
        memset(buffer, 0, BUFFER_SIZE);
        pthread_mutex_unlock(&mutex);
    }

    // Client has disconnected
    printf("Connection closed from %d\n", socket);

    // Remove socket from client sockets list
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (client_sockets[i] == socket)
        {
            client_sockets[i] = 0;

            num_clients--;
        }
    }
    pthread_mutex_unlock(&mutex);

    // Close socket
    close(socket);

    // Detach thread
    pthread_detach(pthread_self());

    return NULL;
}

void signal_handler(int sig) {
    fclose(fptr);
    exit(0);
}