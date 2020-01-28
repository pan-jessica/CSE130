// Jessica Pan - jeypan@ucsc.edu
// FALL 2019 - CSE130 Principle of Computer Systems Design
// Asgn3 - HTTP server with caching and logging

#include <arpa/inet.h>
#include <cstring>
#include <deque>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <libgen.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

using namespace std;
int offset = 0;
pair<bool, char *> logfile(false, "");
deque<pair<string, string>> Cache;

void handle_connection(int new_socket, bool caching);

pair<int, bool> parse_Requests(int argc, char *argv[])
{
    int opt;
    int port;
    bool cache = false;
    if(argc <= 6) {
        while((opt = getopt(argc, argv, "cl:")) != -1) {
            switch(opt) {
                case 'c':
                    cache = true;
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
    pair<int, bool> port_and_cache;
    port_and_cache.first = port;
    port_and_cache.second = cache;

    return port_and_cache;
}

int main(int argc, char *argv[])
{
    pair<int, bool> port_and_cache = parse_Requests(argc, &(*argv));
    int PORT = port_and_cache.first;
    bool cache = port_and_cache.second;

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

        // Waiting for connection
        if((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen))
           < 0) {
            perror("In accept");
            exit(EXIT_FAILURE);
        }

        printf("CONNECTED\n");

        handle_connection(new_socket, cache);
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

string successful_get_log(string method, string fname, bool cachingdone, bool cached)
{
    if(cachingdone) {
        if(cached) {
            return (method + " " + fname + " length 0 [was in cache]\n========\n");
        } else {
            return (method + " " + fname + " length 0 [was not in cache]\n========\n");
        }
    }
    return (method + " " + fname + " length 0\n========\n");
}

string successful_put_log(string method, string fname, string len, bool cachingdone, bool cached)
{
    if(cachingdone) {
        if(cached) {
            return (method + " " + fname + " length " + len + " [was in cache]\n");
        } else {
            return (method + " " + fname + " length " + len + " [was not in cache]\n");
        }
    }
    return (method + " " + fname + " length " + len + "\n");
}

void handle_header_logging(string header, int o, int fd)
{
    char buffer[header.length() + 1];
    strcpy(buffer, header.c_str());
    if(logfile.first) {
        printf("header: %s", buffer);
        int k = pwrite(fd, &buffer, strlen(buffer), o);
        printf("pwrite returns: %d\n", k);
    }
}

int calculatetoReserve(int lines, int buffSize)
{
    return ((8 * lines) + lines + 9 + (buffSize * 3));
}

void handle_log_putdata(string header, char *datta, int count, int off, int filedesc)
{
    int tempOffset = off;
    char head[header.length()];
    strcpy(head, header.c_str());
    char end[] = "========\n";

    char *buff = datta;
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
        printf("index at %d linechar index: %s\n", b, linechars + 3 * b);
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
            printf("size of pad = %lu", sizeof(pad));
            fflush(stdout);

            sprintf(line, "%s%s%c", pad, linechars, '\n');

            t = pwrite(filedesc, line, sizeof(line), tempOffset);
            tempOffset += t;
            printf("pwrite wrote bytes = %d", t);
            fflush(stdout);
        }
    }

    tempOffset += pwrite(filedesc, end, strlen(end), tempOffset);
}

void evictfileInCache()
{
    /// Write to disk (server directory) before removing from cache
    string fname = Cache.front().first;
    string temp = Cache.front().second;
    int size = temp.length();
    string fdata(Cache.front().second);
    int fd;
    if((fd = open(fname.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666)) != -1) {
        write(fd, fdata.c_str(), fdata.size());
    }
    close(fd);
    /// remove file from cache
    Cache.pop_front();
    printf("evicted file from cache\n");
}

void handle_connection(int new_socket, bool caching)
{
    printf("begin handle_connection");
    char buffer[4000];
    char *read_buf[4000];
    int i = 0;
    int status = 0;
    int logfiledisc;
    if(logfile.first) { // logging file
        logfiledisc = open(logfile.second, O_WRONLY | O_CREAT, 0666);
    }

    read(new_socket, buffer, sizeof(buffer));

    char *token = strtok(buffer, "\r\n");
    while(token != NULL) {
        read_buf[i++] = token;
        token = strtok(NULL, "\r\n");
    }

    enum HeaderKey { Method, Filename, Protocol, Host, Useragent, Accept, ContentLen, Expect };

    struct S {
        char *values;
    };
    vector<S> Headers; // Vector for parsing header

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

            int off = handle_logging_offset(log.length());

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

            int off = handle_logging_offset(log.length());

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

                        int off = handle_logging_offset(log.length());

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

                    int off = handle_logging_offset(log.length());

                    // HANDLE_PWRITE
                    handle_header_logging(log, off, logfiledisc);
                }
            }
        } else {
            bool inCache = false;
            if((Cache.size() > 0) && caching) {
                cout << "files in cache: " << Cache.size() << endl;
                string DATA;
                for(int n = 0; n != Cache.size(); ++n) {
                    string fname(Headers[Filename].values);
                    string cachename = Cache.at(n).first;
                    cout << "fname: " << fname << "cachename" << cachename << endl;
                    if(fname == cachename) {
                        inCache = true;
                        DATA = Cache.at(n).second;
                        break;
                    }
                }
                cout << "DATAAAAAAA" << DATA << endl;

                int n = DATA.length();
                char dataArray[n + 1];
                strcpy(dataArray, DATA.c_str());
                char resp[] = "HTTP/1.1 200 OK\r\nContent-Length: ";

                int restNum = to_string(n).length() + 4;
                char rest[restNum];
                strcpy(rest, to_string(n).c_str());
                strcat(rest, "\r\n\r\n");
                strcat(resp, rest);
                send(new_socket, resp, strlen(resp), 0);
                write(new_socket, dataArray, strlen(dataArray));
            }

            if(inCache == false) {
                size_t total_bytes = 0;
                string total_data = "";
                cout << "bytes_read = " << bytes_read << " nbytes = " << nbytes << endl;

                while((bytes_read = read(pfd, buffy, nbytes)) >= 1) {
                    total_bytes += bytes_read;
                    string temp(buffy);
                    total_data += temp;

                    char l[4];
                    sprintf(l, "%zu", bytes_read);
                    char resp[] = "HTTP/1.1 200 OK\r\nContent-Length: ";
                    strcat(resp, l);
                    strcat(resp, "\r\n\r\n");
                    send(new_socket, resp, strlen(resp), 0);
                    write(new_socket, buffy, bytes_read);
                }

                if(caching) {
                    cout << "file not in cache...place data in cache and return to user\n";
                    pair<string, string> new_file = { Headers[Filename].values, total_data };
                    cout << "in new_file pair: first: " << new_file.first
                         << "second: " << new_file.second << endl;
                    if(Cache.size() < 4) {
                        printf("pushing new file into cache\n");
                        printf("files in cache rn: %d", Cache.size());
                        Cache.push_back(new_file);
                    } else { // Cache is full!!!
                        printf("pushing new file when cache is full\n");
                        evictfileInCache();
                        Cache.push_back(new_file);
                    }

                    cout << "files in cache after pushing: " << Cache.size() << endl;
                    cout << "wat's in cache(last/most recent): \nfilename: "
                         << Cache.at(Cache.size() - 1).first
                         << " filedata: " << Cache.at(Cache.size() - 1).second << endl;
                }

                if(total_bytes == 0) {
                    char resp[] = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
                    send(new_socket, resp, strlen(resp), 0);
                }
            }
            // LOGGING
            if(logfile.first) {
                string log = successful_get_log(
                  Headers[Method].values, Headers[Filename].values, caching, inCache);

                int off = handle_logging_offset(log.length());

                // HANDLE_PWRITE
                handle_header_logging(log, off, logfiledisc);
            }
        }

        close(pfd);
    }

    //-----------------------------------HANDLE PUT-----------------------------------------
    if(validFname && (strcmp(Headers[Method].values, "PUT") == 0)) {
        char patty[29];
        sprintf(patty, "%s", Headers[Filename].values);
        const char *filename = patty;
        bool inDisk = false;
        if(access(fname, F_OK) != -1) {
            inDisk = true;
        }

        int pfd;
        if((pfd = open(patty, O_RDWR | O_CREAT | O_TRUNC, 0666)) == -1) {
            printf("chekcing for 403 and 400\n");
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

                        int off = handle_logging_offset(log.length());

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

                        int off = handle_logging_offset(log.length());

                        // HANDLE_PWRITE
                        handle_header_logging(log, off, logfiledisc);
                    }
                }
            }
        } else {
            if(strcmp("None", Headers[ContentLen].values) == 0) {
                char datta[20];
                long rdata;
                char resp[] = "HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\r\n";
                send(new_socket, resp, strlen(resp), 0);

                while((rdata = read(new_socket, datta, sizeof(datta))) >= 1) {
                    write(pfd, datta, rdata);
                }

            } else {
                if(inDisk) {
                    printf("file is in disk");
                } else {
                    printf("file not in disk!!!");
                }
                char datta;
                long rdata = 0;

                char l[4];
                sprintf(l, "%s", Headers[ContentLen].values);
                if(!inDisk) {
                    char resp[] = "HTTP/1.1 201 Created\r\nContent-Length: ";
                    strcat(resp, l);
                    strcat(resp, "\r\n\r\n");
                    send(new_socket, resp, strlen(resp), 0);
                } else {
                    char resp[] = "HTTP/1.1 200 OK\r\nContent-Length: ";
                    strcat(resp, l);
                    strcat(resp, "\r\n\r\n");
                    send(new_socket, resp, strlen(resp), 0);
                }

                int len = atoi(Headers[ContentLen].values);
                char *buf = (char *)malloc(len * sizeof(char));
                while(atoi(Headers[ContentLen].values) != rdata) {
                    int temp = read(new_socket, &datta, 1);
                    write(pfd, &datta, temp);
                    // printf("data char: %s index: %ld\n", &datta, rdata);
                    buf[rdata] = datta;
                    ++rdata;
                }

                string DATA = "";
                printf("all da data frm client:");
                for(i = 0; i < len; ++i) {
                    char temp = buf[i];
                    DATA += temp;
                    printf("%c", buf[i]);
                }

                cout << "DATA initialized: " << DATA << endl;

                bool inCache = false;
                if(caching) {
                    int n;
                    if(Cache.size() > 0) {
                        cout << "files in cache: " << Cache.size() << endl;
                        for(n = 0; n != Cache.size(); ++n) {
                            string fname(Headers[Filename].values);
                            string cachename = Cache.at(n).first;
                            cout << "fname: " << fname << "cachename" << cachename << endl;
                            if(fname == cachename) {
                                inCache = true;
                                break;
                            }
                        }
                    }
                    cout << "DATA after check cache" << DATA << endl;

                    if(inCache) {
                        cout << "file being read from client: " << Headers[Filename].values << endl;
                        cout << "file in cache and overwriting data at " << Cache.at(n).first
                             << " in cache" << endl;
                        Cache.at(n).second = DATA;
                        cout << "newdata from cache for file " << Cache.at(n).second << endl;
                    } else { // FILE NOT IN CACHE
                        pair<string, string> new_file = { Headers[Filename].values, DATA };
                        if(Cache.size() < 4) {
                            printf("pushing new file into cache\n");
                            printf("files in cache rn: %d (<= 4)\n", Cache.size());
                            Cache.push_back(new_file);
                        } else {
                            printf("evicting file from cache && pushing new file to cache\n");
                            printf("files in cache rn: %d (== 4)\n", Cache.size());
                            evictfileInCache();
                            Cache.push_back(new_file);
                        }
                    }
                    cout << "files in cache after fixing up: " << Cache.size() << endl;
                }

                if(logfile.first) {
                    string log = successful_put_log(Headers[Method].values,
                      Headers[Filename].values,
                      Headers[ContentLen].values,
                      caching,
                      inCache);

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
                    int off = handle_logging_offset(log.length() + reservedBytes);

                    // HANDLE_PWRITE
                    handle_log_putdata(log, buf, len, off, logfiledisc);
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
}
