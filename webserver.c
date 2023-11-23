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
    const int port = atoi(argv[2]);

    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

    status = getaddrinfo(ipAddress, argv[2], &hints, &servinfo);
    if(status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\\n\"", gai_strerror(status));
        return -1;
    }

    //Create a socket
    struct addrinfo *res;
    int socket_id = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if(socket_id < 0){
        printf("Error creating socket\n");
        return -1;
    }

    printf("Socket created successfully\n");

    if(bind(socket_id, res->ai_addr, res->ai_addrlen) != 0) {
        fprintf(stderr, "Binding error\n");
        return -1;
    }
/*
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
        *//*const char* reply = "Reply\r\n\r\n";
        printf("Sending response: %s", reply);
        if(send(client_socket, reply, strlen(reply), 0) < 0) {
            printf("Error replying message");
            return -1;
        } else {
            printf("Successful reply!\n");
        }*//*
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

    *//*client_message[bytes_received] = '\0';

    if (strncmp(client_message, "GET ", 4) == 0) {
        printf("Received HTTP/0.9 request\n");
    } else {
        char *end_of_request = strstr(client_message, "\r\n\r\n");
        if (end_of_request != NULL) {
            printf("Recieved HTTP request %s\n", client_message);
        } else {
            printf("Incomplete request received\n");
        }
    }*//*

    //Reply to client
    //char reply[] = "Reply\r\n\r\n";

    printf("Successful reply!");

    close(client_socket);
    close(socket_id);*/
    return 0;
}
