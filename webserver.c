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
#include <stdbool.h>

#define BUFFER_SIZE 8192
#define MAX_HEADERS 40
#define MAX_HEADER_SIZE 256
#define MAX_RESOURCE_COUNT 100
#define MAX_PATH_SIZE 256
#define MAX_PATH_CONTENT 100
#define MAX_DYNAMIC_PATH_SIZE 256
#define DYNAMIC_PATH_PREFIX "/dynamic/"

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

typedef struct {
    char path[MAX_PATH_SIZE];
    char content[MAX_PATH_CONTENT];
} Resource;

Resource resources[MAX_RESOURCE_COUNT];
size_t resources_count = 0;

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

void send_message(char msg[], int connection_id) {
    int len = strlen(msg);
    int bytes_sent = send(connection_id, msg, len, 0);
    if (bytes_sent == -1) {
        perror("Sending error");
        //return -1;
    }
    printf("Message sent successfully\n");
    printf("Successful reply!\n\n\n");
}

bool find_resource(const char *path, char *content) {
    for (int i = 0; i < resources_count; ++i) {
        if (strcmp(resources[i].path, path) == 0) {
            //strcpy(content, resources[i].content);
            return true; // Resource found
        }
    }
    return false; // Resource not found
}

void update_resource(const char *path, const char *content) {
    for (int i = 0; i < resources_count; ++i) {
        if (strcmp(resources[i].path, path) == 0) {
            strcpy(resources[i].content, content);
            return; // Resource updated
        }
    }

    // If the resource is not found, add a new resource
    if (resources_count < MAX_RESOURCE_COUNT) {
        strcpy(resources[resources_count].path, path);
        strcpy(resources[resources_count].content, content);
        resources_count++; // Increment the resource count
    }
}

void delete_resource(const char *path) {
    for (int i = 0; i < resources_count; ++i) {
        if (strcmp(resources[i].path, path) == 0) {
            resources[i].path[0] = '\0'; // Empty the path to mark it as deleted
            resources[i].content[0] = '\0'; // Empty the content
            resources_count--;
            return; // Resource deleted
        }
    }
}

int is_dynamic_path(const char *path) {
    // Check if the path starts with "dynamic/"
    return strncmp(path, DYNAMIC_PATH_PREFIX, strlen(DYNAMIC_PATH_PREFIX)) == 0;
}

void process_request(const HttpRequest *request, int connection_id) {
    // Your logic to process the complete and valid HTTP request goes here
/*    printf("Processing request:\nMethod: %s\nPath: %s\nVersion: %s\nHeaders: %s\nPayload: %s\n",
           request->method, request->path, request->version, request->headers, request->payload);*/
    printf("\n\nProcessing request:\nMethod: %s\nPath: %s\nVersion: %s\nHeaders:\n", request->method, request->path, request->version);
    // Print each header
    for (int i = 0; i < MAX_HEADERS && request->headers[i][0][0] != '\0'; i++) {
        printf("  %s: %s\n", request->headers[i][0], request->headers[i][1]);
    }
    printf("Payload: %s\n", request->payload);

    if(strcmp(request->method, "PUT") == 0) {
        if (!is_dynamic_path(request->path)) {
            // Respond with Forbidden (403) for paths outside dynamic/
            char *msg = "HTTP/1.1 403 Forbidden\r\nContent-Length: 9\r\n\r\nForbidden\r\n";
            send_message(msg, connection_id);
            return;
        }
        // Handle PUT request
        update_resource(request->path, request->payload);

        // Respond with "Created" if the resource did not previously exist
        // Respond with "No Content" if the previous content has been overwritten
        char *msg;
        if (strlen(request->payload) > 0) {
            msg = "HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\r\n";
        } else {
            msg = "HTTP/1.1 204 No Content\r\n\r\n";
        }
        send_message(msg, connection_id);
    } else if (strcmp(request->method, "GET") == 0){
        if (is_dynamic_path(request->path)) {

            // Search for the dynamic route in your resources array
            int found = 0;
            for (size_t i = 0; i < resources_count; i++) {
                if (strcmp(request->path, resources[i].path) == 0) {
                    found = 1;
                    // Construct the HTTP response
                    char response[1024];  // Adjust the size as needed
                    snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\nContent-Length: %zu\r\n\r\n%s\r\n", strlen(resources[i].content), resources[i].content);
                    send_message(response, connection_id);
                    break;
                }
            }
            if(!found) {
                char *msg = "HTTP/1.1 404 Not Found\r\nContent-Length: 12\r\n\r\nNot Found\r\n\r\n";
                send_message(msg, connection_id);
            }

        } else {
            // Check if the request path corresponds to a static route
            if (strncmp(request->path, "/static/foo", strlen("/static/foo")) == 0) {
                // Respond with "Foo"
                char *msg = "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nFoo\r\n";
                send_message(msg, connection_id);
            } else if (strncmp(request->path, "/static/bar", strlen("/static/bar")) == 0) {
                // Respond with "Bar"
                char *msg = "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nBar\r\n";
                send_message(msg, connection_id);
            } else if (strncmp(request->path, "/static/baz", strlen("/static/baz")) == 0) {
                // Respond with "Baz"
                char *msg = "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nBaz\r\n";
                send_message(msg, connection_id);
            } else {
                // Respond with "Not Found" for other paths
                char *msg = "HTTP/1.1 404 Not Found\r\nContent-Length: 12\r\n\r\nNot Found\r\n\r\n";
                send_message(msg, connection_id);
            }
        }
    } else if (strcmp(request->method, "DELETE") == 0) {
        // Check if the resource exists before attempting deletion
        if (!find_resource(request->path, NULL)) {
            // Respond with "Not Found" if the resource doesn't exist
            char *msg = "HTTP/1.1 404 Not Found\r\nContent-Length: 12\r\n\r\nNot Found\r\n\r\n";
            send_message(msg, connection_id);
        } else {
            // Handle DELETE request
            delete_resource(request->path);

            // Respond with "No Content" for successful deletion
            char *msg = "HTTP/1.1 204 No Content\r\nContent-Length: 0\r\n\r\n";
            send_message(msg, connection_id);
        }
    }
}

// Function to check if "Content-Length" is present in the headers
int has_content_length_header(char headers[MAX_HEADERS][2][MAX_HEADER_SIZE], int header_count) {
    for (int i = 0; i < header_count; i++) {
        if (strcmp(headers[i][0], "Content-Length") == 0) {
            return 1; // Found "Content-Length" header
        }
    }
    return 0; // "Content-Length" not found
}

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
        // Invalid request: no start line
        return -1;
    }

    if(sscanf(line, "%s %s %s", request->method, request->path, request->version) != 3) {
        return -1;
    }

    // Check if method, URI, and HTTP version are not empty
    if (request->method[0] == '\0' || request->path[0] == '\0' || request->version[0] == '\0') {
        // Invalid request: empty method, URI, or HTTP version
        return -1;
    }

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

/*    // Parse payload based on Content-Length header
    for (int i = 0; i < header_count; i++) {
        if (strcmp(request->headers[i][0], "Content-Length") == 0) {
            sscanf(request->headers[i][1], "%zu", &(request->content_length));
            break;
        }
    }*/
    // Parse payload based on Content-Length header
    for (int i = 0; i < header_count; i++) {
        if (strcmp(request->headers[i][0], "Content-Length") == 0) {
            size_t content_length;
            sscanf(request->headers[i][1], "%zu", &content_length);

            while (isspace(*payload_start)) {
                payload_start++;
            }

            // Find the start of the payload
/*            char *payload_pos = strstr(payload_start, "\r\n\r\n");
            if (payload_pos != NULL) {
                payload_pos += 4;  // Move past the "\r\n\r\n"*/

                // Check if the content length matches the actual payload length
                if (strlen(payload_start) >= content_length) {
                    strncpy(request->payload, payload_start, content_length);
                    request->payload[content_length] = '\0';  // Null-terminate the payload
                } else {
                    // Invalid request: payload length doesn't match Content-Length header
                    return -1;
                }
            //}
            break;
        }
    }

/*    // Skip leading whitespace characters before the payload
    while (isspace(*payload_start)) {
        payload_start++;
    }

    // Copy payload if present
    if (request->content_length > 0) {
        strncpy(request->payload, line, request->content_length);
    }

    // Check for a missing Content-Length header for non-zero content length
    if (request->content_length > 0 && has_content_length_header(request->headers, header_count) == 0) {
        // Invalid request: non-zero content length with no Content-Length header
        return -1;
    }*/

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

        while(1) {
            assert(buffer_len < BUFFER_SIZE);
            ssize_t  amt = read(connection_id, buffer + buffer_len, sizeof(buffer) - buffer_len);
            if(amt <= 0) {
                perror("Reading failed\n");
                break;
            }

            print_msg_buffer(buffer, buffer_len);

            buffer_len += amt;
            char *e = strstr(buffer, "\r\n\r\n");
            if(e) {
                // Parse HTTP request
                HttpRequest parsed_request;
                int parse_status = parse_http_request(buffer, &parsed_request);
                if(parse_status < 0) {
                    msg = "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\r\n\r\n";
                    send_message(msg, connection_id);
                } else if (strcmp(parsed_request.method, "GET") == 0 || strcmp(parsed_request.method, "POST") == 0 || strcmp(parsed_request.method, "PUT") == 0 || strcmp(parsed_request.method, "DELETE") == 0) {
                    // Handle the request based on the parsed information
                    printf("PROCCCEESSISNGS");
                    process_request(&parsed_request, connection_id);
                } else {
                    msg = "HTTP/1.1 501 Not Implemented\r\nContent-Length: 17\r\n\r\nNot Implemented\r\n\r\n";
                    send_message(msg, connection_id);
                }

                memset(buffer, '\0', BUFFER_SIZE);
                buffer_len = 0;
            }
        }
        close(connection_id);
    }

    // Close the server socket
    close(socket_id);
    return 0;
}
