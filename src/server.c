#include "../include/server.h"

struct ClientInfo
{
    int client_socket;
};

noreturn void *handle_client(void *arg);
noreturn void  start_server(const char *address, uint16_t port);

int main(int argc, char *argv[])
{
    char    *endptr;
    long int port_long = strtol(argv[2], &endptr, BASE_TEN);

    if(argc != 3)
    {
        fprintf(stderr, "Usage: %s <address> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if(*endptr != '\0' || port_long < 0 || port_long > UINT16_MAX)
    {
        fprintf(stderr, "Invalid port number: %s\n", argv[2]);
        exit(EXIT_FAILURE);
    }

    start_server(argv[1], (uint16_t)port_long);
}

noreturn void *handle_client(void *arg)
{
    struct ClientInfo *client_info = (struct ClientInfo *)arg;
    char               buffer[BUFFER_SIZE];
    ssize_t            bytes_received;
    const char        *args[] = {"/bin/sh", "-c", buffer, NULL};

    while(1)
    {
        bytes_received = recv(client_info->client_socket, buffer, BUFFER_SIZE - 1, 0);
        // Fork a new process to execute the command
        pid_t pid = fork();
        if(bytes_received <= 0)
        {
            printf("Client disconnected or an error occurred.\n");
            close(client_info->client_socket);
            free(client_info);     // Free the allocated memory for client_info
            pthread_exit(NULL);    // Exit the thread
        }

        buffer[bytes_received] = '\0';    // Null-terminate the received string

        if(pid == 0)
        {    // Child process
            // Redirect standard output and standard error to the socket
            dup2(client_info->client_socket, STDOUT_FILENO);
            dup2(client_info->client_socket, STDERR_FILENO);
            close(client_info->client_socket);

            execv(args[0], (char *const *)args);

            // If execv returns, it means there was an error
            perror("execv");
            exit(EXIT_FAILURE);
        }
        else if(pid > 0)
        {    // Parent process
            // Wait for the child process to complete
            int status;
            waitpid(pid, &status, 0);

            // Send end message to client to indicate command execution is complete
            send(client_info->client_socket, end_msg, strlen(end_msg), 0);
        }
        else
        {
            perror("Failed to fork");
        }
    }
}

noreturn void start_server(const char *address, uint16_t port)
{
    int                server_fd;
    int                new_socket;
    struct sockaddr_in server_addr;
    int                opt = 1;

    // Creating socket file descriptor
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(address);
    server_addr.sin_port        = htons(port);

    // Bind the socket to the port
    if(bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if(listen(server_fd, MAX_CLIENTS) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on %s:%d\n", address, port);

    while(1)
    {
        new_socket = accept(server_fd, (struct sockaddr *)&server_addr, (socklen_t *)&opt);
        if(new_socket < 0)
        {
            perror("accept");
            continue;
        }

        struct ClientInfo *client_info = malloc(sizeof(struct ClientInfo));
        if(!client_info)
        {
            perror("Failed to allocate memory for client_info");
            close(new_socket);
            continue;
        }
        client_info->client_socket = new_socket;

        pthread_t thread_id;
        if(pthread_create(&thread_id, NULL, handle_client, (void *)client_info) != 0)
        {
            perror("Failed to create thread");
            close(new_socket);
            free(client_info);
        }

        pthread_detach(thread_id);
    }
}
