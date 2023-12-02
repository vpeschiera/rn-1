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
#include <assert.h>

#define BUFFER_SIZE 8192
#define MAX_HEADERS 40
#define MAX_HEADER_SIZE 256
#define MAX_RESOURCE_COUNT 100
#define MAX_PATH_SIZE 256

/*typedef struct {
    char method[10];
    char path[255];
    char version[10];
    char headers[255];
    char payload[BUFFER_SIZE - 600]; // Adjust the size based on your requirements
} HttpRequest;*/
typedef struct {
    char method[MAX_HEADER_SIZE];
    char path[MAX_PATH_SIZE];
    char version[MAX_HEADER_SIZE];
    char headers[MAX_HEADERS][2][MAX_HEADER_SIZE];
    char payload[BUFFER_SIZE];
    size_t content_length;
} HttpRequest;


void print_msg_buffer(char buffer[], int buffer_len) {
    printf("Received message: ");
    for (int i = 0; i < buffer_len; ++i) {
        if (isprint(buffer[i])) {
            putchar(buffer[i]);
        } else {
            printf("\\x%02X", buffer[i] & 0xFF);
        }
    }
    printf("\n");
}

void process_request(const HttpRequest *request) {
    // Your logic to process the complete and valid HTTP request goes here
    printf("Processing request:\nMethod: %s\nPath: %s\nVersion: %s\nHeaders: %s\nPayload: %s\n",
           request->method, request->path, request->version, request->headers, request->payload);
}

// Function to parse the HTTP request string into an HttpRequest structure
/*int parse_http_request(const char *request_str, HttpRequest *parsed_request) {
    // Simple parsing logic (you may need to enhance this based on your requirements)
    sscanf(request_str, "%s %s %s\r\n%[^\r\n]\r\n\r\n", parsed_request->method,
           parsed_request->path, parsed_request->version, parsed_request->headers);
*//*    sscanf(request_str, "%9s %99s %19s\r\n%499[^\r\n]\r\n\r\n%499[^\r\n]",
           parsed_request->method, parsed_request->path, parsed_request->version,
           parsed_request->headers, parsed_request->payload);*//*

    if(strcmp(parsed_request->method, "GET") != 0
        && strcmp(parsed_request->method, "POST") != 0
        && strcmp(parsed_request->method, "PUT") != 0
        && strcmp(parsed_request->method, "DELETE") != 0
    ) {
        return -1;
    } else if (strcmp(parsed_request->method, "GET") == 0) {
        return 1;
    }

    // Find the start of the payload
    const char *payload_start = strstr(request_str, "\r\n\r\n");
    if (payload_start != NULL) {
        payload_start += 4;  // Move past the "\r\n\r\n"
        strncpy(parsed_request->payload, payload_start, sizeof(parsed_request->payload) - 1);
        parsed_request->payload[sizeof(parsed_request->payload) - 1] = '\0';  // Null-terminate the payload
    }
    return 0;
}*/

int parse_http_request(char *buffer, HttpRequest *request) {
    char *token;
    char *saveptr;
    char *line;
    int header_count = 0;

    // Reset request structure
    memset(request, 0, sizeof(HttpRequest));

    // Parse start line
    line = strtok_r(buffer, "\r\n", &saveptr);
    if (!line) {
        // Invalid request
        return -1;
    }
    sscanf(line, "%s %s %s", request->method, request->path, request->version);

    // Copy line pointer for later use in payload extraction
    char *payload_start = saveptr;

    // Parse headers
    while ((line = strtok_r(NULL, "\r\n", &saveptr)) && header_count < MAX_HEADERS) {
        char *colon_pos = strstr(line, ": ");
        if (line[0] == '\0' || !colon_pos) {
            break; // Empty line indicates end of headers
        }

        // Update payload_start pointer
        payload_start = saveptr;

        // Find the position of the colon in the line
        //char *colon_pos = strstr(line, ": ");
        if (colon_pos) {
            // Extract the header name and value
            strncpy(request->headers[header_count][0], line, colon_pos - line);
            strcpy(request->headers[header_count][1], colon_pos + 2);
            header_count++;
        }
/*        token = strtok(line, ": ");
        strncpy(request->headers[header_count][0], token, MAX_HEADER_SIZE);
        token = strtok(NULL, "");
        strncpy(request->headers[header_count][1], token, MAX_HEADER_SIZE);
        header_count++;*/
    }

    // Parse payload based on Content-Length header
    for (int i = 0; i < header_count; i++) {
        if (strcmp(request->headers[i][0], "Content-Length") == 0) {
            sscanf(request->headers[i][1], "%zu", &(request->content_length));
            break;
        }
    }

    // Skip leading whitespace characters before the payload
    while (isspace(*payload_start)) {
        payload_start++;
    }

    // Copy payload if present
    if (request->content_length > 0) {
        strncpy(request->payload, payload_start, request->content_length);
    }

    return 0; // Successfully parsed
}

int check_string_ends_with_crlf(const char *str) {
    size_t len = strlen(str);
    return (len >= 4 && str[len - 4] == '\r' && str[len - 3] == '\n' && str[len - 2] == '\r' && str[len - 1] == '\n');
}


/*void get_http_request(char *str) {
    const char *substrings[] = {"GET", "POST", "PUT", "DELETE"};
    size_t substring_count = sizeof(substrings) / sizeof(substrings[0]);

    for (size_t i = 0; i < substring_count; ++i) {
        const char *substring = substrings[i];
        const char *pos = strstr(str, substring);
        if (pos != NULL && check_string_ends_with_crlf(pos + strlen(substring))) {
            printf("Found substring '%s' at position %zu\n", substring, pos - str);
        }
    }
}*/
int get_http_request(char *str, int *start, int *end) {
    const char *substrings[] = {"GET", "POST", "PUT", "DELETE"};
    size_t substring_count = sizeof(substrings) / sizeof(substrings[0]);

    *start = 0;
    *end = 0;

    for (size_t i = 0; i < substring_count; ++i) {
        char *substring = substrings[i];
        const char *pos = strstr(str, substring);
        if (pos != NULL && check_string_ends_with_crlf(pos + strlen(substring))) {
            *start = pos - str;  // Set the start position
            const char *end_pos = strstr(pos, "\r\n\r\n");
            if (end_pos != NULL) {
                *end = end_pos + 4 - str;  // Set the end position after "\r\n\r\n"
                strncpy(substring, str + *start, *end - *start);
                substring[*end - *start] = '\0';
                printf("Substring: %.*s\n", *end - *start, str + *start);
                printf("Found substring '%s' at position %d\n", substring, *start);

                return 1;  // Valid HTTP request found
            }
        }
    }

    return 0;  // No valid HTTP request found
}
/*HttpRequestPosition get_http_request(char *str) {
    const char *substrings[] = {"GET", "POST", "PUT", "DELETE"};
    size_t substring_count = sizeof(substrings) / sizeof(substrings[0]);

    HttpRequestPosition position = {0, 0};  // Initialize positions to 0

    for (size_t i = 0; i < substring_count; ++i) {
        const char *substring = substrings[i];
        const char *pos = strstr(str, substring);
        if (pos != NULL && check_string_ends_with_crlf(pos + strlen(substring))) {
            position.start = pos - str;  // Set the start position
            const char *end_pos = strstr(pos, "\r\n\r\n");
            if (end_pos != NULL) {
                position.end = end_pos + 4 - str;  // Set the end position after "\r\n\r\n"
                printf("Found substring '%s' at position %zu\n", substring, position.start);
                break;  // Stop searching after the first valid HTTP request is found
            }
            *//*position.end = pos + strlen(substring) - str;  // Set the end position
            printf("Found substring '%s' at position %zu\n", substring, position.start);
            break;  // Stop searching after the first valid HTTP request is found*//*
        }
    }

    return position;
}*/

/*void print_request(HttpRequest *request) {
    printf("Method: %s\n", request->method);
    printf("Path: %s\n", request->path);
    printf("Version: %s\n", request->version);

    printf("Headers:\n");
    for (int i = 0; i < MAX_HEADERS && request->headers[i][0][0] != '\0'; i++) {
        printf("%s: %s\n", request->headers[i][0], request->headers[i][1]);
    }

    if (request->content_length > 0) {
        printf("Payload: %.*s\n", (int)request->content_length, request->payload);
    }
}*/

int main(int argc, char* argv[]){
    char buffer[BUFFER_SIZE] = {0};
    size_t buffer_len = 0;

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
    //socklen_t addr_size;

    int status;
    struct addrinfo hints = {
            .ai_family = AF_INET,
    };
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

    status = getaddrinfo(ipAddress, port, &hints, &servinfo);
    if(status != 0) {
        fprintf(stderr, "Error parsing host/port");
        return -1;
    }

    //Create a socket
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

    char *msg;
    while(1) {
        struct sockaddr_storage their_addr;
        socklen_t addr_size = sizeof their_addr;

        int connection_id = accept(socket_id, (struct sockaddr *)&their_addr, &addr_size);
        if (connection_id == -1) {
            perror("Accepting error");
            return -1;
        }

        printf("Connection accepted\n");
        //--------------------
        assert(buffer_len < BUFFER_SIZE);
        ssize_t  amt = read(connection_id, buffer + buffer_len, sizeof(buffer) - buffer_len);
        if(amt < 0) {
            perror("Reading failed\n");
            return -1;
        }

        print_msg_buffer(buffer, buffer_len);

        buffer_len += amt;
        char *e = strstr(buffer, "\r\n\r\n");
        if(e) {
            size_t msglen = e - buffer;

            // Parse HTTP request
            HttpRequest parsed_request;
            parse_http_request(buffer, &parsed_request);
/*            if (parse_http_request(buffer, buffer_len, &parsed_request) == 0) {
                // Print the parsed request for debugging
                //print_request(&parsed_request);
                printf("REQUEST VALID ");
                // Handle the HTTP request
            } else {
                // Handle parsing error or invalid request
                printf("NOT VALID");
            }*/
/*            int parse_status = parse_http_request(buffer, &parsed_request);
            if (parse_status < 0) {
                msg = "HTTP/1.1 501 Not Implemented\r\nContent-Length: 17\r\n\r\nNot Implemented\r\n\r\n";
            } else if (parse_status == 1) {
                msg = "HTTP/1.1 404 Not Found\r\nContent-Length: 13\r\n\r\nNot Found\r\n\r\n";
            } else {
                msg = "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\r\n\r\n";
            }*/

            process_request(&parsed_request);

            memset(buffer, '\0', BUFFER_SIZE);
            buffer_len = 0;
        }

        msg = "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\r\n\r\n";
        int len = strlen(msg);
        int bytes_sent = send(connection_id, msg, len, 0);
        if (bytes_sent == -1) {
            perror("Sending error");
            return -1;
        }
        printf("Message sent successfully\n");
        // Close the connection after sending the message
    }

    printf("Successful reply!");

    // Close the sockets
    //regfree(&regex);
    //close(connection_id);
    //close(socket_id);
    return 0;
}
