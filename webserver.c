//
// Created by Valeria Peschiera on 08.11.23.
//
#include <sys/socket.h>
#include <printf.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <regex.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>

#define BUFFER_SIZE 2000

void send_response(int client_socket, const char *status_line, const char *body) {
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response), "%s\r\nContent-Length: %zu\r\n\r\n%s\r\n", status_line, strlen(body), body);

    if (send(client_socket, response, strlen(response), 0) < 0) {
        printf("Error sending response\n");
    } else {
        printf("Successful response!\n");
    }
}

int main(int argc, char* argv[]){

/*    const char *ipAddressPattern = "^[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}$";
    printf("Test");*/
    //Get arguments from program start
/*    regex_t regex;
    int compile_status = regcomp(&regex, ipAddressPattern, 0);*/
/*    if (strcmp("localhost", argv[1]) == 0) {
        ipAddress = "127.0.0.1";
    } else if(compile_status != 0){
        printf("Invalid IP Address\n");
        return -1;
    } else {
         ipAddress = argv[1];
    }*/
    if(argc > 3) {
        printf("Too many arguments!\n");
        return -1;
    } else if(argc == 1) {
        printf("Missing arguments\n");
        return -1;
    }

    //Save arguments in variables
    char *ipAddress = argv[1];
    const char *port = argv[2];
    socklen_t addr_size;

    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

    status = getaddrinfo(ipAddress, port, &hints, &servinfo);
    if(status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\\n\"", gai_strerror(status));
        return -1;
    }

    //Create a socket
    /*struct addrinfo *res;*/

    int socket_id = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    if(socket_id < 0){
        fprintf(stderr, "Error creating socket\n");
        return -1;
    }

    printf("Socket created successfully\n");

    int bindStatus = bind(socket_id, servinfo->ai_addr, servinfo->ai_addrlen);
    if(bindStatus != 0) {
        fprintf(stderr, "Binding error: %s\n", gai_strerror(bindStatus));
        return -1;
    }

    printf("Socket bound successfully\n");

    int listenStatus = listen(socket_id, 5);
    if(listenStatus != 0) {
        fprintf(stderr, "Listening error: %s\n", gai_strerror(listenStatus));
        return -1;
    }

    printf("Listening successfully\n");

    /*const char *pattern = "^(GET|POST|PUT|DELETE) / HTTP/1\\.1\\(r|x0D)\\(n|x0A)(.*: .*\\(r|x0D)\\(n|x0A))?(.*\\(r|x0D)\\(n|x0A))*\\(r|x0D)\\(n|x0A)(.*)$";*/
    /*const char *pattern = "^(GET|POST|PUT|DELETE) \\/ HTTP\\/1\\.1\\\\(r|x0D)\\\\(n|x0A).*: .*\\\\(r|x0D)\\\\(n|x0A)?(.*\\\\(r|x0D)\\\\(n|x0A))*\\\\(r|x0D)\\\\(n|x0A)(.*)$"*/
    //const char *pattern = "^(GET|POST|PUT|DELETE) / HTTP/1\\.1\\(r|x0D)\\(n|x0A).*: .*\\(r|x0D)\\(n|x0A)?(.*\\(r|x0D)\\(n|x0A))*\\(r|x0D)\\(n|x0A)(.*)$";
/*
    const char *pattern = "^(GET|POST|PUT|DELETE) \/ HTTP\/1\.1\\(r|x0D)\\(n|x0A)(.*: .*\\(r|x0D)\\(n|x0A))?(.*\\(r|x0D)\\(n|x0A))*\\(r|x0D)\\(n|x0A)(.*)$";
*/

    const char *pattern = "^(GET|POST|PUT|DELETE) / HTTP/1\\.1([\r\n]|[\x0D\x0A])(.*: .*([\r\n]|[\x0D\x0A]))*([\r\n]|[\x0D\x0A])(.*)$";

    regex_t regex;
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        fprintf(stderr, "Failed to compile regex\n");
        return 1;
    }

    while(1) {
        struct sockaddr_storage their_addr;
        socklen_t addr_size = sizeof their_addr;

        int connection_id = accept(socket_id, (struct sockaddr *)&their_addr, &addr_size);
        if (connection_id == -1) {
            perror("Accepting error");
            return -1;
        }

        printf("Connection accepted\n");

        char recv_buffer[3000];
        int recv_status = recv(connection_id, recv_buffer, sizeof(recv_buffer) - 1, 0);
        if(recv_status == -1) {
            perror("Receiving error");
            return -1;
        }
        recv_buffer[recv_status] = '\0';
        printf("Received message: ");
        for (int i = 0; i < recv_status; ++i) {
            if (isprint(recv_buffer[i])) {
                putchar(recv_buffer[i]);
            } else {
                printf("\\x%02X", recv_buffer[i] & 0xFF);
            }
        }
        printf("\n");

        char *msg;
        regmatch_t matches[6];
        if (regexec(&regex, recv_buffer, 6, matches, 0) == 0) {
            // Match found
            printf("HTTP Method: %.*s\n", (int)(matches[1].rm_eo - matches[1].rm_so), recv_buffer + matches[1].rm_so);
            printf("Host: %.*s\n", (int)(matches[3].rm_eo - matches[3].rm_so), recv_buffer + matches[3].rm_so);
            printf("Payload: %.*s\n", (int)(matches[5].rm_eo - matches[5].rm_so), recv_buffer + matches[5].rm_so);

            msg = "Reply\r\n\r\n";

        } else {
            msg = "No match found";
            printf("No match\n");
        }

        int len = strlen(msg);
        int bytes_sent = send(connection_id, msg, len, 0);
        if (bytes_sent == -1) {
            perror("Sending error");
            return -1;
        }
        printf("Message sent successfully\n");
        // Close the connection after sending the message
        close(connection_id);
    }
/*    struct sockaddr_storage their_addr;
    int connection_id;
    addr_size = sizeof their_addr;
    connection_id = accept(socket_id, (struct sockaddr *)&their_addr, &addr_size);
    if(connection_id == -1) {
        fprintf(stderr, "Accepting error: %s\n", gai_strerror(connection_id));
        fprintf(stderr, "Accepting error: %s\n", strerror(errno));
        return -1;
    }

    printf("Connection accepted\n");

    char *msg = "reply";
    int len, bytes_sent;

    len = strlen(msg);
    bytes_sent = send(connection_id, msg, len, 0);
    if(bytes_sent == -1) {
        fprintf(stderr, "Sending error: %s\n", gai_strerror(listenStatus));
        fprintf(stderr, "Sending error: %s\n", strerror(errno));
        return -1;
    }

    printf("Message sent successfully\n");

    //Set socket ip address and port
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr(ipAddress);

    //Bind socket to port
    if(bind(socket_id, (struct sockaddr*)&server_address, sizeof(server_address)) < 0){
        printf("Error binding socket");
        return -1;
    }

    printf("Socket bound successfully");

    //Listen on socket
    if(listen(socket_id, 1) < 0){
        printf("Error listening");
        return -1;
    }
    printf("Listening");

    //Create client address variable
    struct sockaddr_in client_address;

    //Accept connection and check status
    socklen_t client_length = sizeof(client_address);
    int client_socket = accept(socket_id, (struct sockaddr*)&client_address, &client_length);
    if (client_socket < 0){
        printf("Error accepting connection");
        return -1;
    }

    printf("Connection established");

    //Receive message from client and reply
    char client_message[2000];

    ssize_t bytes_received = recv(client_socket, client_message, sizeof(client_message), 0);
    if( bytes_received < 0){
        printf("Error receiving message");
        return -1;
    }

    client_message[bytes_received] = '\0';
    char *end_of_headers = strstr(client_message, "\r\n\r\n");
    if (end_of_headers != NULL) {
        // Complete HTTP request received
        printf("Received HTTP request: %s\n", client_message);

        // For simplicity, assume all requests are valid, otherwise, respond with 400 Bad Request
        const char *status_line = "HTTP/1.1 200 OK";
        const char *body = "Reply";

        // Send a response (you can modify this based on your requirements)
        send_response(client_socket, status_line, body);
        //const char* reply = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\nContent-Type: text/plain\r\n\r\nTEST REPLY\r\n";
        const char* reply = "Reply\r\n\r\n";
        printf("Sending response: %s", reply);
        if(send(client_socket, reply, strlen(reply), 0) < 0) {
            printf("Error replying message");
            return -1;
        } else {
            printf("Successful reply!\n");
        }
    } else {
        // Incomplete request received, respond with 400 Bad Request
        printf("Incomplete request received: %s\n", client_message);

        const char *status_line = "HTTP/1.1 400 Bad Request";
        const char *body = "Bad Request";

        // Send a response
        send_response(client_socket, status_line, body);
        // Incomplete request received, handle accordingly (e.g., buffer it for future reads)
        //  printf("Incomplete request received: %s\n", client_message);
    }
    printf("Client message: %s", client_message);

    client_message[bytes_received] = '\0';

    if (strncmp(client_message, "GET ", 4) == 0) {
        printf("Received HTTP/0.9 request\n");
    } else {
        char *end_of_request = strstr(client_message, "\r\n\r\n");
        if (end_of_request != NULL) {
            printf("Recieved HTTP request %s\n", client_message);
        } else {
            printf("Incomplete request received\n");
        }
    }

    //Reply to client
    //char reply[] = "Reply\r\n\r\n";

    printf("Successful reply!");

    close(client_socket);
    close(socket_id);*/
    regfree(&regex);
    return 0;
}
