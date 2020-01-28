/*
 *Jessica Pan - jeypan@ucsc.edu
 *FALL 2019 - CSE 130 Principles of Computer System Design
 *Asgn0 - dog: mock version of cat command
 */
////////////////////////////////////////////////////////////////////

#include <err.h>
#include <fcntl.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    size_t nbytes, bytes_read;
    char buffer[32000];
    nbytes = sizeof(buffer);

    if(argc > 1) {
        for(int k = 1; k < argc; ++k) {
            if(strcmp(argv[k], "-") == 0 || strcmp(argv[k], "--") == 0) {
                while((bytes_read = read(STDIN_FILENO, buffer, nbytes)) >= 1) {
                    write(STDOUT_FILENO, buffer, bytes_read);
                }
            } else {
                const char *path = argv[k];
                struct stat s;
                if(stat(path, &s) == 0) {
                    if(s.st_mode & S_IFDIR) { // if argv[k] is a directory
                        warnx("%s: Is a directory", argv[k]);
                    } else if(s.st_mode & S_IFREG) { // if argv[k] is a file

                        int fd = open(argv[k], O_RDONLY);
                        if(fd != -1) {
                            while((bytes_read = read(fd, buffer, nbytes)) >= 1) {
                                write(STDOUT_FILENO, buffer, bytes_read);
                            }
                        }
                        close(fd);
                    } else { // argv[k] isn't a file or a directory
                        warn("%s", argv[k]);
                    }
                } else { // catches flags/or nonexistent files '--f'
                    warn("%s", argv[k]);
                }
            }
        }
    } else { // argc == 1
        while((bytes_read = read(STDIN_FILENO, buffer, nbytes)) >= 1) {
            write(STDOUT_FILENO, buffer, bytes_read);
        }
    }

    return 0;
}
