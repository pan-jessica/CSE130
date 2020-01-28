// Jessica Pan - jeypan@ucsc.edu
// FALL 2019 - CSE130 Principle of Computer Systems Design
// Asgn1 - HTTP server

#include <arpa/inet.h>
#include <cstring>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

using namespace std;

int main(int argc, char const *argv[])
{
    int port;
    if(argc == 3) { // ./httpserver localhost [port]
        sscanf(argv[2], "%d", &port);
    } else if(argc == 2) { // ./httpserver localhost [standard HTTP port]
        port = 80;
    } else {
        warnx("Usage: ./httpserver [address] [optional port]");
        exit(1);
    }

    int server_fd, new_socket;
    long valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("In socket");
        exit(EXIT_FAILURE);
    }

#define PORT port

    address.sin_family = AF_INET;
    if(strcmp(argv[1], "localhost") == 0) {
        address.sin_addr.s_addr = inet_addr("127.0.0.1");
    } else {
        address.sin_addr.s_addr = inet_addr(argv[1]);
    }
    address.sin_port = htons(PORT);

    // BINDING WITH SOCKET
    if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("In bind");
        exit(EXIT_FAILURE);
    }

    // LISTEN FOR SOCKET
    if(listen(server_fd, 10) < 0) {
        perror("In listen");
        exit(EXIT_FAILURE);
    }

    // CREATING CONNECTION
    while(1) {
        // Waiting for connection
        if((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen))
           < 0) {
            perror("In accept");
            exit(EXIT_FAILURE);
        }

        char buffer[4000];
        char *read_buf[4000];
        int i = 0;
        valread = read(new_socket, buffer, sizeof(buffer));

        char *token = strtok(buffer, "\r\n");
        while(token != NULL) {
            read_buf[i++] = token;
            token = strtok(NULL, "\r\n");
        }

        enum HeaderKey { Method, Filename, Protocol, Host, Useragent, Accept, ContentLen, Expect };

        struct S {
            char *values;
        };
        vector<S> Headers;

        char *tok = strtok(read_buf[Method], " ");
        while(tok != NULL) { // Method
            S s;
            if(strcmp(tok, "GET") == 0 || strcmp(tok, "PUT") == 0) {
                s.values = tok;
                Headers.push_back(s);
            } else if(strcmp(tok, "HTTP/1.1") != 0) { // filename or path
                s.values = basename(tok);
                Headers.push_back(s);
            } else { // Protocol
                s.values = tok;
                Headers.push_back(s);
            }
            tok = strtok(NULL, " ");
        }

        //-------------------------------------GET HEADER--------------------
        if(strcmp(Headers[Method].values, "GET") == 0) {
            S s;
            char *t = strtok(read_buf[1], " "); // HOST
            while(t != NULL) {
                if(strcmp(t, "Host:") != 0) {
                    s.values = t;
                    Headers.push_back(s);
                }
                t = strtok(NULL, " ");
            }

            t = strtok(read_buf[2], " "); // USER-AGENT
            while(t != NULL) {
                if(strcmp(t, "User-Agent:") != 0) {
                    s.values = t;
                    Headers.push_back(s);
                }
                t = strtok(NULL, " ");
            }

            t = strtok(read_buf[3], " "); // ACCEPT
            while(t != NULL) {
                if(strcmp(t, "Accept:") != 0) {
                    s.values = t;
                    Headers.push_back(s);
                }
                t = strtok(NULL, " ");
            }

        } else if(strcmp(Headers[Method].values, "PUT") == 0) {
            //////// -----------------PUT HEADER ---------------------
            S s;
            for(i = 1; read_buf[i] != NULL; ++i) {
                char *t = strtok(read_buf[i], " ");
                while(t != NULL) {
                    if((i == 1) && (strcmp(t, "Host:") != 0)) {
                        s.values = t;
                        Headers.push_back(s);
                    }
                    if((i == 2) && (strcmp(t, "User-Agent:") != 0)) {
                        s.values = t;
                        Headers.push_back(s);
                    }
                    if((i == 3) && (strcmp(t, "Accept:") != 0)) {
                        s.values = t;
                        Headers.push_back(s);
                    }
                    if((i == 4) && (strcmp(t, "Expect:") == 0)) {
                        char eh[] = "None";
                        s.values = eh;
                        Headers.push_back(s);
                    } else if((i == 4) && (strcmp(t, "Content-Length:") != 0)) {
                        s.values = t;
                        Headers.push_back(s);
                    } else if((i == 5) && (strcmp(t, "Expect:") != 0)) {
                        s.values = t;
                        Headers.push_back(s);
                    }
                    t = strtok(NULL, " ");
                }
            }
        } else {
            // 500 INTERNAL SERVER ERROR
            char isrErr[] = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
            send(new_socket, isrErr, strlen(isrErr), 0);
        }

        /// CHECKING FILENAME FOR EXACTLY 27 CHARACTERS
        bool validFname = false;
        char *fname = Headers[Filename].values;
        if(strlen(fname) != 27) {
            // 400 (BAD REQUEST) ERROR
            validFname = false;
        } else {
            for(auto k = 0; fname[k] != '\0'; ++k) {
                if(((fname[k] >= 'A' && fname[k] <= 'Z') || (fname[k] >= 'a' && fname[k] <= 'z')
                     || (fname[k] >= '0' && fname[k] <= '9') || (fname[k] == '-')
                     || (fname[k] == '_'))
                   && (fname[k] != '.')) {
                    validFname = true;
                } else {
                    validFname = false;
                }
            }
        }
        // 400 (BAD REQUEST) ERROR
        if(validFname == false) {
            char notTs[] = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
            send(new_socket, notTs, strlen(notTs), 0);
        }
        //-----------------------------------HANDLE GET-----------------------------------------

        if((validFname == true) && (strcmp(Headers[Method].values, "GET") == 0)) {
            size_t nbytes, bytes_read;
            char buffy[4000];
            nbytes = sizeof(buffy);

            char *filepath = Headers[Filename].values;
            int pfd;
            if((pfd = open(filepath, O_RDONLY, 0)) == -1) {
                struct stat s;
                if(stat(filepath, &s) == 0) {
                    mode_t k = s.st_mode;
                    if((k & S_IRUSR) == 0) {
                        char getErr[] = "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n";
                        send(new_socket, getErr, strlen(getErr), 0);
                    }
                } else {
                    // 404 FILE NOT FOUND
                    char getErr[] = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
                    send(new_socket, getErr, strlen(getErr), 0);
                }
            } else {
                while((bytes_read = read(pfd, buffy, nbytes)) >= 1) {
                    char l[4];
                    sprintf(l, "%zu", bytes_read);
                    char resp[] = "HTTP/1.1 200 OK\r\nContent-Length: ";
                    strcat(resp, l);
                    strcat(resp, "\r\n\r\n");
                    send(new_socket, resp, strlen(resp), 0);
                    write(new_socket, buffy, bytes_read);
                }
            }

            close(pfd);
        }

        //-----------------------------------HANDLE PUT-----------------------------------------
        if(validFname && (strcmp(Headers[Method].values, "PUT") == 0)) {
            char patty[29];
            strcat(patty, Headers[Filename].values);
            const char *filename = patty;

            int pfd;
            if((pfd = open(patty, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1) {
                struct stat s;
                if(stat(patty, &s) == 0) {
                    mode_t k = s.st_mode;
                    if((k & S_IRUSR) == 0) {
                        char getErr[] = "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n";
                        send(new_socket, getErr, strlen(getErr), 0);
                    } else {
                        // Testing with curl(1) --> prevents file from reaching here
                        // 400 BAD REQUEST
                        char putInval[] = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
                        send(new_socket, putInval, strlen(putInval), 0);
                    }
                }
            } else {
                char datta[32000];
                long rdata;
                if(strcmp("None", Headers[ContentLen].values) == 0) {
                    char resp[] = "HTTP/1.1 201 Created\r\nContent-Length: 6\r\n\r\n";
                    send(new_socket, resp, strlen(resp), 0);
                    while((rdata = read(new_socket, datta, sizeof(datta))) >= 1) {
                        write(pfd, datta, rdata);
                    }

                } else {
                    if((rdata = read(new_socket, datta, sizeof(datta))) >= 1) {
                        write(pfd, datta, rdata);
                    }

                    char l[4];
                    sprintf(l, "%s", Headers[ContentLen].values);
                    char resp[] = "HTTP/1.1 201 Created\r\nContent-Length: ";
                    strcat(resp, l);
                    strcat(resp, "\r\n\r\n");
                    send(new_socket, resp, strlen(resp), 0);
                }
            }
            memset(patty, 0, sizeof(patty));
            filename = NULL;
            close(pfd);
        }
        close(new_socket);
    }
    return 0;
}
