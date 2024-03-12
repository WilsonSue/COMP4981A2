#include "../include/client.h"

void start_client(const char *address, uint16_t port);

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

    start_client(argv[1], (uint16_t)port_long);
    return 0;
}

void start_client(const char *address, uint16_t port)
{
    int                client_socket;
    struct sockaddr_in server_addr;
    int                flags;

#ifdef SOCK_CLOEXEC
    client_socket = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
#else
    client_socket = socket(AF_INET, SOCK_STREAM, 0);    // NOLINT(android-cloexec-socket)
#endif

    if(client_socket == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    flags = fcntl(client_socket, F_GETFD);
    if(flags == -1)
    {
        close(client_socket);
        perror("Error getting flags on socket");
        exit(EXIT_FAILURE);
    }

    flags |= FD_CLOEXEC;
    if(fcntl(client_socket, F_SETFD, flags) == -1)
    {
        close(client_socket);
        perror("Error setting FD_CLOEXEC on socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(address);
    server_addr.sin_port        = htons(port);

    // Connect to the server
    if(connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("\nConnection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server. Type your messages and press Enter to send. "
           "Press Ctrl-Z to exit or Ctrl-D to close the Server Connection.\n");

    // Start a simple chat loop
    while(1)
    {
        int    activity;
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET((long unsigned int)client_socket, &readfds);
        FD_SET((long unsigned int)STDIN_FILENO, &readfds);

        // Wait for activity on the socket or user input
        activity = select(client_socket + 1, &readfds, NULL, NULL, NULL);

        if(activity < 0)
        {
            perror("Select error");
            break;
        }

        // Check if there is a message from the server or other clients
        if(FD_ISSET((long unsigned int)client_socket, &readfds))
        {
            char    server_buffer[BUFFER_SIZE];
            ssize_t bytes_received = recv(client_socket, server_buffer, sizeof(server_buffer) - 1, 0);

            if(bytes_received <= 0)
            {
                printf("\nServer closed the connection.\n");
                break;
            }

            server_buffer[bytes_received] = '\0';
            printf("Received: %s", server_buffer);
        }

        // Check if there is user input
        if(FD_ISSET((long unsigned int)STDIN_FILENO, &readfds))
        {
            char client_buffer[BUFFER_SIZE];
            if(fgets(client_buffer, sizeof(client_buffer), stdin) == NULL)
            {
                // Ctrl-D was pressed, causing EOF
                printf("EOF detected. Closing connection.\n");
                break;
            }

            if(send(client_socket, client_buffer, strlen(client_buffer), 0) == -1)
            {
                perror("Error sending message");
                break;
            }
        }
    }

    // Close the client socket when the loop exits
    close(client_socket);
}
