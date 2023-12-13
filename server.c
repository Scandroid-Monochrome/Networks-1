#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <signal.h>
#include <dirent.h>     /* for checking files */

#define MAXPENDING 5 /* Maximum outstanding connection requests */
#define BUFFER_SIZE 1024
#define SEND_CHAR_SIZE 10000

void DieWithError(char *errorMessage); /* Error handling function */
void HandleTCPClient(int clntSocket);  /* TCP client handling function */
void parse_path_file(char* full_path, char *directory, char *file_name);
void sendFile(int client_socket, char* file_name);

int main(int argc, char *argv[])
{
    int servSock;                    /*Socket descriptor for server */
    int clntSock;                    /* Socket descriptor for client */
    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned short echoServPort;     /* Server port */
    unsigned int clntLen;            /* Length of client address data structure */
    // char *httpRequest;

    if (argc != 2) /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage: %s <Server Port>\n", argv[0]);
        exit(1);
    }

    echoServPort = atoi(argv[1]); /* First arg: local port */

    // printf("Server Port: %d\n", echoServPort);

    /* Create socket for incoming connections */
    if ((servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort);      /* Local port */

    /* Bind to the local address */
    if (bind(servSock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("bind() failed");

    /* Mark the socket so it will listen for incoming connections */
    if (listen(servSock, MAXPENDING) < 0)
        DieWithError("listen() failed");

    for (;;) /* Run forever */
    {
        /* Set the size of the in-out parameter */
        clntLen = sizeof(echoClntAddr); /* Wait for a client to connect */
        if ((clntSock = accept(servSock, (struct sockaddr *)&echoClntAddr, &clntLen)) < 0)
            DieWithError("accept() failed");
        /* clntSock is connected to a client! */
        printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));
        HandleTCPClient(clntSock);

        close(clntSock);

        // For Ctrl C
        signal(SIGINT, SIG_DFL);
    }

    close(servSock);
}

void DieWithError(char *errorMessage)
{
    printf("%s\n", errorMessage);
}

void HandleTCPClient(int clntSocket) /* TCP client handling function */
{
    // printf("*********CAUGHT REQUEST:*********\n");
    char httpRequest[1000];
    // if HTTP request is good
    recv(clntSocket, httpRequest, 1000, 0);
    printf("HTTP Request:\n%s\n", httpRequest);

    // Error 404
    char* error404 = "HTTP/1.1 404 Not Found\r\n\r\n<HTML>test</HTML>\n\n";

    // Extract the file request from the HTTP request
    char* parsed_HTTP = (char*)malloc(1000 * sizeof(char));
    parsed_HTTP = strtok(httpRequest, " ");
    char* subdirectory;
    int counter = 0;
    while(counter < 2) {
        if (counter == 1) {
            subdirectory = parsed_HTTP;
        }
        parsed_HTTP = strtok(NULL, " ");
        counter += 1;
    }

    // printf("Subdirectory: %s\n", subdirectory);
    
    // if there is a file request:
    if (strlen(subdirectory) > 1) {
        char* path = (char*)malloc(strlen(subdirectory)*sizeof(char));
        char* item = (char*)malloc(strlen(subdirectory)*sizeof(char));
        parse_path_file(subdirectory, path, item);

        // printf("Subdirectory: %s\n", path);
        // printf("Item: %s\n", item);

        // FIND OUR FILE
        //Create stuff to find file
        DIR *directory;
        struct dirent *entry;

        directory = (DIR*)malloc(50 * sizeof(DIR*));
        directory = opendir(path);

        if (path == NULL) {
            send(clntSocket, error404, strlen(error404), 0);
            printf("Error opening directory.\n");
            exit(1);
        }

        int is_there = 0;
        printf("OUR ITEMS:\n[");
        char* our_file;
        while ((entry = readdir(directory)) != NULL) {
            if(strcmp(entry->d_name, item) == 0) {
                is_there = 1;
                our_file = entry->d_name;
            }
            printf("%s\n", entry->d_name);
            printf(", ");
        }

        printf("]\n\n");

        // If item exists
        if (is_there) {
            sendFile(clntSocket, our_file);
        } else {
            // printf("Item not found.\n");
            send(clntSocket, error404, strlen(error404), 0);
        }

        if (closedir(directory) == -1) {
            printf("Error closing directory.\n");
            exit(1);
        }

        free(path);
        free(item);
    }

    // Close connection and wait for the next one
    close(clntSocket);
    printf("\n\n");
}

void parse_path_file(char* full_path, char *directory, char *file_name) {
    int full_length = strlen(full_path);
    int last_slash_index = -1;

    for (int i = full_length; i >= 0; i--) {
        if (full_path[i] == '/') {
            last_slash_index = i;
            break;
        }
    }

    if (last_slash_index == -1) {
        memcpy(directory, "", 0);
        memcpy(file_name, full_path, full_length);
    } else {
        memcpy(directory, full_path, last_slash_index);
        memcpy(file_name, full_path + last_slash_index + 1, full_length - last_slash_index + 1);
    }
}

void sendFile(int client_socket, char* file_name) {
    FILE* p_wanted_file = fopen(file_name, "r");
    // unsigned char current_line[SEND_CHAR_SIZE];

    // while (fread(current_line, sizeof(current_line), SEND_CHAR_SIZE, p_wanted_file) != 0) {
    int counter = 1;
    while (!feof(p_wanted_file)) {
        unsigned char current_line[SEND_CHAR_SIZE];
        fread(current_line, 1, SEND_CHAR_SIZE, p_wanted_file);

        printf("Packet No. %d\n", counter);
        // char buffer[1024] = "HTTP/1.1 200 OK\n";
        // printf("Okay until here.\n");
        // Print the read data
        // printf("%s", current_line);
        // printf("Okay until here.\n");
        send(client_socket, current_line, SEND_CHAR_SIZE, 0);
        // printf("Okay until here.\n");
        counter++;
    }
    
    fclose(p_wanted_file);
}
