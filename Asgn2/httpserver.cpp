// Jessica Pan - jeypan@ucsc.edu
// FALL 2019 - CSE130 Principle of Computer Systems Design
// Asgn2 - Multi-threading HTTP server with logging

#include "queue.h"
#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <cstring>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

using namespace std;

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER;
pthread_mutex_t logmutex = PTHREAD_MUTEX_INITIALIZER;
int offset = 0;
pair<bool, char *> logfile(false, "");

struct B {
    char buf;
};

void *handle_connection(void *p_new_socket);

pair<int, int> parse_Requests(int argc, char *argv[])
{
    int opt;
    int port;
    int threadpoolSize;
    if(argc <= 7) {
        while((opt = getopt(argc, argv, "N:l:")) != -1) {
            switch(opt) {
                case 'N':
                    printf("-N: %s\n", optarg);
                    fflush(stdout);
                    threadpoolSize = atoi(optarg);
                    printf("threadpool size: %d\n", threadpoolSize);
                    fflush(stdout);
                    break;
                case 'l':
                    printf("-l logfile: %s\n", optarg);
                    fflush(stdout);
                    logfile.first = true;
                    logfile.second = optarg;
                    printf("logfile first %d seond %s\n", logfile.first, logfile.second);
                    fflush(stdout);
                    break;
            }
        }
        if(threadpoolSize == 0) { // by default - 4 threads
            threadpoolSize = 4;
        }
        printf("threadpool size after flags: %d\n", threadpoolSize);
        fflush(stdout);

        int nonflags = argc - optind;
        printf("nonflags: %d argc: %d optind: %d\n", nonflags, argc, optind);
        fflush(stdout);
        if(nonflags == 2) { // ./httpserver localhost [port]
            printf("in nonflags\n");
            fflush(stdout);
            port = atoi(argv[optind + 1]);
            printf("%d", port);
            fflush(stdout);
        } else if(nonflags == 1) { // ./httpserver localhost [standard HTTP port]
            port = 80;
        } else {
            warnx(
              "Usage: ./httpserver -N [# of threads] -l [log_filename] [address] [optional port]");
            exit(1);
        }
    } else {
        warnx("Usage: ./httpserver -N [# of threads] -l [log_filename] [address] [optional port]");
        exit(1);
    }
    pair<int, int> port_and_threadpoolSize;
    port_and_threadpoolSize.first = port;
    port_and_threadpoolSize.second = threadpoolSize;

    return port_and_threadpoolSize;
}

void *thread_function(void *arg)
{
    while(1) {
        int *pclient;
        pthread_mutex_lock(&mutex1);
        if((pclient = dequeue()) == NULL) {
            pthread_cond_wait(&condition_var, &mutex1);
            pclient = dequeue();
        }
        pthread_mutex_unlock(&mutex1);
        if(pclient != NULL) {
            // connection made
            handle_connection(pclient);
        }
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    // PARSING GET/PUT REQUEST HEADER & GET PORT #
    pair<int, int> port_and_threads = parse_Requests(argc, &(*argv));
    int PORT = port_and_threads.first;
    int threadpool_size = port_and_threads.second;

    pthread_t threadpool[threadpool_size];

    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("In socket");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    if(strcmp(argv[optind], "localhost") == 0) {
        address.sin_addr.s_addr = inet_addr("127.0.0.1");
    } else {
        address.sin_addr.s_addr = inet_addr(argv[optind]);
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
        printf("Waiting for Connections.....");
        fflush(stdout);

        for(int i = 0; i < threadpool_size; i++) {
            pthread_create(&threadpool[i], NULL, thread_function, NULL);
        }

        // Waiting for connection
        if((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen))
           < 0) {
            perror("In accept");
            exit(EXIT_FAILURE);
        }
        printf("CONNECTED\n");
        fflush(stdout);

        int *pclient = (int *)malloc(sizeof(int));
        *pclient = new_socket;
        pthread_mutex_lock(&mutex1);
        enqueue(pclient);
        pthread_cond_signal(&condition_var);
        pthread_mutex_unlock(&mutex1);
        /*
                printf("pclient val: %d", *pclient);
                fflush(stdout);
                printf("pthread id in654 main: %d", pthread_self());
                fflush(stdout);
                printf("End of connection.....\n\n");
                fflush(stdout);
          */
    }
    return 0;
}

int handle_logging_offset(int newoffset)
{
    int prev = offset;
    offset += newoffset;
    printf("offset at: %d\n", prev);
    fflush(stdout);

    return prev;
}

string failed_request_log(string method, string fname, string protocol, string status)
{
    return ("FAIL: " + method + " " + fname + " " + protocol + " --- response " + status
            + "\n========\n");
}

string successful_get_log(string method, string fname)
{
    return (method + " " + fname + " length 0\n========\n");
}

string successful_put_log(string method, string fname, string len)
{
    return (method + " " + fname + " length " + len + "\n");
}

/*
void test_putheader(string header, int o, int fd)
{
    char buffer[header.length()];
    strcpy(buffer, header.c_str());
    if(logfile.first) {
        printf(" put header: %s\n", buffer);
        fflush(stdout);
        int k = pwrite(fd, &buffer, sizeof(buffer), o);
        printf("pwrite returns: %d\n", k);
        fflush(stdout);
    }
}
*/

void handle_header_logging(string header, int o, int fd)
{
    char buffer[header.length() + 1];
    strcpy(buffer, header.c_str());
    if(logfile.first) {
        printf("header: %s", buffer);
        fflush(stdout);
        int k = pwrite(fd, &buffer, strlen(buffer), o);
        printf("pwrite returns: %d\n", k);
        fflush(stdout);
    }
}

int calculatetoReserve(int lines, int buffSize)
{
    return ((8 * lines) + lines + 9 + (buffSize * 3));
}

void handle_log_putdata(string header,
  char *datta /* vector<B> &datta*/,
  int count,
  int line,
  int off,
  int filedesc)
{
    int tempOffset = off;
    char head[header.length()];
    strcpy(head, header.c_str());
    char end[] = "========\n";
    /*    char buff[datta.size()];

    printf("%d\n", sizeof(buff));
    fflush(stdout);
    */

    char *buff = datta;
    /*printf("data in put log: %s\n", datta);
    fflush(stdout);

    printf("data in put log: %s\n", buff);
    fflush(stdout);

    printf("whats in header: %s", head);
    fflush(stdout);
*/
    int t;
    t = pwrite(filedesc, &head, sizeof(head), tempOffset);
    tempOffset += t;
    printf("pwrite wrote bytes = %d\n", t);
    fflush(stdout);

    char linechars[61];
    int previous = 0;
    int padding = 0;
    for(int b = 0; b < count; ++b) {
        if(b != 0 && ((b % 20) == 0)) { // full line of chars
                                        // PADDING

            printf("padding value in ifstmt = %d\n", previous);
            fflush(stdout);
            string temp = to_string(previous);
            temp = string(8 - temp.length(), '0').append(temp);
            char pad[8];
            strcpy(pad, temp.c_str());

            printf("padding: %s\n", pad);

            char line[69];
            sprintf(line, "%s%s%c", pad, linechars, '\n');

            printf("linechars: %s\n", linechars);
            printf("data line w/ padding: %s\n", line);

            t = pwrite(filedesc, line, sizeof(line), tempOffset);
            tempOffset += t;
            printf("pwrite wrote bytes = %d\ncurrent global offset = %d\ncurrent local offset = %d",
              t,
              offset,
              tempOffset);
            fflush(stdout);
            previous = padding;
        }

        printf("char to hex: %c\n", buff[b]);
        fflush(stdout);

        padding += 1;
        sprintf(linechars + 3 * (b % 20), " %02x", buff[b]);

        printf("padding value = %d\n", padding);
        fflush(stdout);
        printf("index at %d linechar index: %d\n", b, linechars + 3 * b);
        fflush(stdout);

        if(b == count - 1) {
            string temp = to_string(previous);
            temp = string(8 - temp.length(), '0').append(temp);
            char pad[8];
            strcpy(pad, temp.c_str());

            printf("padding <20 area: %s\n", pad);
            fflush(stdout);

            int leftover = ((count % 20) * 3) + 8 + 1;
            char line[leftover];
            printf("size of pad = %d", sizeof(pad));
            fflush(stdout);

            sprintf(line, "%s%s%c", pad, linechars, '\n');
            /*
                        printf("line after padding = %s", line);
                        fflush(stdout);
                        printf("cound = %d", count);
                        fflush(stdout);
                        printf("leftover = %d\n", leftover);
                        fflush(stdout);
                        printf("mod val = %d\nsizeof buff = %d\n", (b % 20), sizeof(buff));
                        fflush(stdout);
                        printf("linechars: %s", linechars);
                        fflush(stdout);
            */
            t = pwrite(filedesc, line, sizeof(line), tempOffset);
            tempOffset += t;
            printf("pwrite wrote bytes = %d", t);
            fflush(stdout);
        }
    }
    // tempOffset += pwrite(filedesc, buff, count, tempOffset);

    tempOffset += pwrite(filedesc, end, sizeof(end), tempOffset);
}

void *handle_connection(void *p_new_socket)
{
    printf("begin handle_connection\n");
    fflush(stdout);
    int new_socket = *((int *)p_new_socket);
    free(p_new_socket);
    char buffer[4000];
    char *read_buf[4000];
    int i = 0;
    int status = 0;
    int logfiledisc;
    if(logfile.first) {
        logfiledisc = open(logfile.second, O_RDWR | O_CREAT, 0666);
    }
    read(new_socket, buffer, sizeof(buffer));

    /*  printf("inside handle_connection\n");
        fflush(stdout);
        printf("What's in the buffer: %s\n", buffer);
        fflush(stdout);
        printf("pthread id: %d", pthread_self());
        fflush(stdout);
    */
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
        status = 500;
        char isrErr[] = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
        send(new_socket, isrErr, strlen(isrErr), 0);
        if(logfile.first) {
            string log = failed_request_log(
              Headers[Method].values, Headers[Filename].values, Headers[Protocol].values, "500");

            int off;
            pthread_mutex_lock(&logmutex);
            off = handle_logging_offset(log.length());
            pthread_mutex_unlock(&logmutex);

            // HANDLE_PWRITE
            handle_header_logging(log, off, logfiledisc);
        }
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
                 || (fname[k] >= '0' && fname[k] <= '9') || (fname[k] == '-') || (fname[k] == '_'))
               && (fname[k] != '.')) {
                validFname = true;
            } else {
                validFname = false;
            }
        }
    }
    // 400 (BAD REQUEST) ERROR
    if(validFname == false && status != 500) {
        char notTs[] = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
        send(new_socket, notTs, strlen(notTs), 0);
        if(logfile.first) {
            string log = failed_request_log(
              Headers[Method].values, Headers[Filename].values, Headers[Protocol].values, "400");

            int off;
            pthread_mutex_lock(&logmutex);
            off = handle_logging_offset(log.length());
            pthread_mutex_unlock(&logmutex);

            // HANDLE_PWRITE
            handle_header_logging(log, off, logfiledisc);
        }
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
                    if(logfile.first) {
                        string log = failed_request_log(Headers[Method].values,
                          Headers[Filename].values,
                          Headers[Protocol].values,
                          "403");

                        int off;
                        pthread_mutex_lock(&logmutex);
                        off = handle_logging_offset(log.length());
                        pthread_mutex_unlock(&logmutex);

                        // HANDLE_PWRITE
                        handle_header_logging(log, off, logfiledisc);
                    }
                }
            } else {
                // 404 FILE NOT FOUND
                char getErr[] = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
                send(new_socket, getErr, strlen(getErr), 0);
                if(logfile.first) {
                    string log = failed_request_log(Headers[Method].values,
                      Headers[Filename].values,
                      Headers[Protocol].values,
                      "404");

                    int off;
                    pthread_mutex_lock(&logmutex);
                    off = handle_logging_offset(log.length());
                    pthread_mutex_unlock(&logmutex);

                    // HANDLE_PWRITE
                    handle_header_logging(log, off, logfiledisc);
                }
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
            if(bytes_read == 0) {
                char resp[] = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
                send(new_socket, resp, strlen(resp), 0);
            }
            if(logfile.first) {
                string log = successful_get_log(Headers[Method].values, Headers[Filename].values);

                int off;
                pthread_mutex_lock(&logmutex);
                off = handle_logging_offset(log.length());
                pthread_mutex_unlock(&logmutex);

                // HANDLE_PWRITE
                handle_header_logging(log, off, logfiledisc);
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

                    if(logfile.first) {
                        string log = failed_request_log(Headers[Method].values,
                          Headers[Filename].values,
                          Headers[Protocol].values,
                          "403");

                        int off;
                        pthread_mutex_lock(&logmutex);
                        off = handle_logging_offset(log.length());
                        pthread_mutex_unlock(&logmutex);

                        // HANDLE_PWRITE
                        handle_header_logging(log, off, logfiledisc);
                    }

                } else {
                    // Testing with curl(1) --> prevents file from reaching here
                    // 400 BAD REQUEST
                    char putInval[] = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
                    send(new_socket, putInval, strlen(putInval), 0);
                    if(logfile.first) {
                        string log = failed_request_log(Headers[Method].values,
                          Headers[Filename].values,
                          Headers[Protocol].values,
                          "400");

                        int off;
                        pthread_mutex_lock(&logmutex);
                        off = handle_logging_offset(log.length());
                        pthread_mutex_unlock(&logmutex);

                        // HANDLE_PWRITE
                        handle_header_logging(log, off, logfiledisc);
                    }
                }
            }
        } else {
            struct stat file;
            if(strcmp("None", Headers[ContentLen].values) == 0) { // no content length header
                char datta[20];
                long rdata;
                int totalsize = 0;

                char resp[] = "HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\r\n";
                send(new_socket, resp, strlen(resp), 0);

                while((rdata = read(new_socket, datta, sizeof(datta))) >= 1) {
                    totalsize += rdata;
                    write(pfd, datta, rdata);
                }

            } else { // with content length header

                char datta;
                long rdata = 0;

                char l[4];
                sprintf(l, "%s", Headers[ContentLen].values);

                char resp[] = "HTTP/1.1 201 Created\r\nContent-Length: ";
                strcat(resp, l);
                strcat(resp, "\r\n\r\n");
                send(new_socket, resp, strlen(resp), 0);

                int len = atoi(Headers[ContentLen].values);
                char *buf = (char *)malloc(len * sizeof(char));

                int k = 0;
                while(atoi(Headers[ContentLen].values) != rdata) {
                    int temp = read(new_socket, &datta, 1);
                    write(pfd, &datta, temp);
                    printf("data char: %s index: %d\n", &datta, rdata);
                    fflush(stdout);
                    buf[k] = datta;
                    ++rdata;
                    ++k;
                }

                printf("all da data frm client:");
                for(int i = 0; i < len; ++i) {
                    printf("%c", buf[i]);
                    fflush(stdout);
                }

                if(logfile.first) {
                    string log = successful_put_log(
                      Headers[Method].values, Headers[Filename].values, Headers[ContentLen].values);

                    int temp = atoi(Headers[ContentLen].values) / 20;
                    int tem = atoi(Headers[ContentLen].values) % 20;
                    int lines = tem > 0 ? (temp + 1) : temp;
                    printf("length: %d, lines: %d", atoi(Headers[ContentLen].values), lines);
                    fflush(stdout);
                    int reservedBytes = calculatetoReserve(lines, atoi(Headers[ContentLen].values));
                    printf(
                      "number of reserved bytes for data before added to header: %d header: %d",
                      reservedBytes,
                      log.length());
                    int off;
                    pthread_mutex_lock(&logmutex);
                    off = handle_logging_offset(log.length() + reservedBytes);
                    pthread_mutex_unlock(&logmutex);
                    // test_putheader(log, off, logfiledisc);

                    // HANDLE_PWRITE
                    handle_log_putdata(log, buf, len, lines, off, logfiledisc);
                }
                free(buf);
            }
        }
        memset(patty, 0, sizeof(patty));
        filename = NULL;
        close(pfd);
    }

    close(logfiledisc);
    close(new_socket);
    return NULL;
}
