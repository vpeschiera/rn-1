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

#define BUFFER_SIZE 2000

typedef struct {
    char method[10];
    char path[255];
    char version[10];
    char headers[255];
    char payload[BUFFER_SIZE - 600]; // Adjust the size based on your requirements
} HttpRequest;

typedef struct {
    size_t start;
    size_t end;
} HttpRequestPosition;

void send_response(int client_socket, const char *status_line, const char *body) {
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response), "%s\r\nContent-Length: %zu\r\n\r\n%s\r\n", status_line, strlen(body), body);

    if (send(client_socket, response, strlen(response), 0) < 0) {
        printf("Error sending response\n");
    } else {
        printf("Successful response!\n");
    }
}

void process_request(const HttpRequest *request) {
    // Your logic to process the complete and valid HTTP request goes here
    printf("Processing request:\nMethod: %s\nPath: %s\nVersion: %s\nHeaders: %s\nPayload: %s\n",
           request->method, request->path, request->version, request->headers, request->payload);
}

/*int parse_http_request(const char *request_str, HttpRequest *request) {
    // Basic parsing logic (you might want to enhance this for a full parser)
    return sscanf(request_str, "%9s %254s %9s\r\n%254[^\r\n]\r\n\r\n%999[^\0]", request->method, request->path,
                  request->version, request->headers, request->payload);
}*/
// Function to parse the HTTP request string into an HttpRequest structure
void parse_http_request(const char *request_str, HttpRequest *parsed_request) {
    printf("REQ -- %s", request_str);
    // Simple parsing logic (you may need to enhance this based on your requirements)
    sscanf(request_str, "%s %s %s\r\n%[^\r\n]\r\n\r\n", parsed_request->method,
           parsed_request->path, parsed_request->version, parsed_request->headers);

    // Find the start of the payload
    const char *payload_start = strstr(request_str, "\r\n\r\n");
    if (payload_start != NULL) {
        payload_start += 4;  // Move past the "\r\n\r\n"
        strncpy(parsed_request->payload, payload_start, sizeof(parsed_request->payload) - 1);
        parsed_request->payload[sizeof(parsed_request->payload) - 1] = '\0';  // Null-terminate the payload
    }
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

/*    const char *pattern = "^(GET|POST|PUT|DELETE) / HTTP/1\\.1([\r\n]|[\x0D\x0A])(.*: .*([\r\n]|[\x0D\x0A]))*([\r\n]|[\x0D\x0A])(.*)$";

    regex_t regex;
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        fprintf(stderr, "Failed to compile regex\n");
        return 1;
    }*/

    //char msg[1024];
    //size_t pos = 0;
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
        /*else if(amt == 0) {
            printf("READING AMT 0");
            break;
        }*/
        buffer_len += amt;
        char *e = strstr(buffer, "\r\n\r\n");
        if(e) {
            size_t msglen = e - buffer;

            HttpRequest parsed_request;
            parse_http_request(buffer, &parsed_request);

            process_request(&parsed_request);

            memset(buffer, '\0', BUFFER_SIZE);
            buffer_len = 0;
        }

        // Receive data from the client
        /*ssize_t bytes_received;
        size_t inbuf_remain = BUFFER_SIZE - buffer_len;
        //while ((bytes_received = recv(connection_id, (void*)&buffer[buffer_len], inbuf_remain, 0)) > 0) {
            bytes_received = recv(connection_id, (void*)&buffer[buffer_len], inbuf_remain, 0);
            if(bytes_received < 0) {
                perror("Reading failed\n");
                return -1;
            } else if(bytes_received == 0) {
                printf("READING AMT 0");
                break;
            }
            buffer_len += bytes_received;
            // Identify complete HTTP requests in the buffer
            int start, end;
            int status = get_http_request(buffer, &start, &end);
            if(status != 0) {
                printf("++++++++++++++++++++");
                char valid_request[end - start];
                memcpy(valid_request, buffer + start, end - start);

                HttpRequest parsed_request;
                parse_http_request(valid_request, &parsed_request);

                // Process the complete HTTP request
                process_request(&parsed_request);
                printf("++++++++++++++++++++");
            }
            char *end_of_header = strstr(buffer, "\r\n\r\n");
            if (end_of_header != NULL) {
            //while (end_of_header != NULL) {
                // A complete HTTP request is found
                size_t request_len = end_of_header - buffer + 4; // Include the end of headers and the empty line
                char request[request_len + 1]; // +1 for null terminator
                strncpy(request, buffer, request_len);
                request[request_len] = '\0'; // Null-terminate the string

                // Parse the HTTP request string into an HttpRequest structure
                HttpRequest parsed_request;
                parse_http_request(request, &parsed_request);

                // Process the complete HTTP request
                process_request(&parsed_request);

                // Remove the processed data from the buffer

                //memmove(buffer, buffer + request_len, buffer_len);
                //buffer_len -= request_len;

                // Look for the next complete HTTP request in the buffer
                //end_of_header = strstr(buffer, "\r\n\r\n");
            }*/
        //}

/*        char recv_buffer[3000];
        int recv_status = recv(connection_id, recv_buffer, sizeof(recv_buffer) - 1, 0);
        if (recv_status == 0) {
            // Connection closed by the client
            perror("Client disconnected\n");
            break; // Break out of the loop
        } else if(recv_status == -1) {
            perror("Receiving error");
            close(connection_id);
            return -1;
        }*/

        printf("Received message: ");
        for (int i = 0; i < buffer_len; ++i) {
            if (isprint(buffer[i])) {
                putchar(buffer[i]);
            } else {
                printf("\\x%02X", buffer[i] & 0xFF);
            }
        }
        printf("\n");

        //MATCHING OF HTTP PACKET
        char *msg;


/*       strcat(buffer, escaped_recv_buffer);
        buffer_len += data_len;

        memset(recv_buffer, 0, sizeof(recv_buffer));
        printf("BUFFER :::: %s", buffer);*//*
        regmatch_t matches[6];
        if (regexec(&regex, recv_buffer, 6, matches, 0) == 0) {
            // Match found
            printf("HTTP Method: %.*s\n", (int)(matches[1].rm_eo - matches[1].rm_so), recv_buffer + matches[1].rm_so);
            printf("Host: %.*s\n", (int)(matches[3].rm_eo - matches[3].rm_so), recv_buffer + matches[3].rm_so);
            printf("Payload: %.*s\n", (int)(matches[5].rm_eo - matches[5].rm_so), recv_buffer + matches[5].rm_so);

            //msg = "Reply\r\n\r\n";
            msg = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nReply\r\n\r\n";

        } else {
            msg = "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\r\n\r\n";
            printf("No match\n");
        }
*/
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
