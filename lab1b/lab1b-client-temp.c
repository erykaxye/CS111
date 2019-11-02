//fuck my life
/*
NAME: Erica Xie
EMAIL: ericaxie@ucla.edu
ID: 404920875
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <poll.h> 
#include <sys/socket.h>
#include <termios.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <assert.h> 
#include "zlib.h"

/* Use this variable to remember original terminal attributes. */

struct termios saved_attributes;

void
reset_input_mode (void)
{
    tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
}

void
set_input_mode (void)
{
    struct termios tattr;

    /* Make sure stdin is a terminal. */
    if (!isatty (STDIN_FILENO)) {
        fprintf (stderr, "Not a terminal.\n");
        exit (EXIT_FAILURE);
    }

    /* Save the terminal attributes so we can restore them later. */
    tcgetattr (STDIN_FILENO, &saved_attributes);
    atexit (reset_input_mode);

    /* Set the funny terminal modes. */
    tcgetattr (STDIN_FILENO, &tattr);
    tattr.c_iflag = ISTRIP; /* only lower 7 bits*/
    tattr.c_oflag = 0; /* no processing */
    tattr.c_lflag = 0; /* no processing */
    tattr.c_cc[VMIN] = 1;
    tattr.c_cc[VTIME] = 0;
    tcsetattr (STDIN_FILENO, TCSANOW, &tattr); //TCSAFLUSH
} 

int main(int argc, char **argv) {

    fprintf(stderr, "hello0");

    /* declare all variables */
    char temp, buf[1024], tolog[1024], logbuf[1024], compbuf[1024], decompbuf[1024];
    int opt, l, c, sockfd, portno, i, k, ret, input, port_given, fd1, count, n; 
    int retu;
    z_stream stdintosocket, sockettostdout; 
    struct sockaddr_in serv_addr; 
    struct hostent *server; 
    struct pollfd fds[2];

    struct rlimit lim = {
        10000,  /* Soft limit */
        10000  /* Hard limit (ceiling for rlim_cur) */
    }; 

    static struct option long_options[] =
    {
        {"port", required_argument, NULL, 'p'},
        {"log", required_argument, NULL, 'l'},
        {"compress", no_argument, NULL, 'c'},
        {0, 0, 0, 0}
    };

    /* initialize variables */
    l = 0; 
    c = 0; 
    n = 0; 
    count = 0;
    port_given = 0; 

    /* check optional arguments */ 
    while((opt = getopt_long(argc, argv, "plc:", long_options, NULL)) != -1) {
        switch(opt){
            case 'p':
                portno = atoi(optarg); 
                port_given = 1; 
                break;
            case 'l':
                l = 1;
                setrlimit(RLIMIT_FSIZE, &lim);
                fd1 = creat(optarg, 0644); 
                if (fd1 < 0) {
                    fprintf(stderr, "Cannot create file: %s given by --output\n%s\n", optarg, strerror(errno));
                    exit(1);
                }
                break;
            case 'c':
                c = 1;
                stdintosocket.zalloc = Z_NULL; 
                stdintosocket.zfree = Z_NULL; 
                stdintosocket.opaque = Z_NULL; 
                if(deflateInit(&stdintosocket, Z_DEFAULT_COMPRESSION) < 0) {
                    fprintf(stderr, "Cannot initializing compression");
                    exit(1);
                }
                sockettostdout.zalloc = Z_NULL; 
                sockettostdout.zfree = Z_NULL; 
                sockettostdout.opaque = Z_NULL; 
                if(inflateInit(&sockettostdout) < 0) {
                    fprintf(stderr, "Cannot initializing compression");
                    exit(1);
                }
                break;
            default:
                fprintf(stderr, "Invalid Argument\n");
                exit(1);
        }
    }

    if(!port_given) { 
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        exit(1);
    }

    set_input_mode(); 

    /* create a socket point */
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 

    if (sockfd < 0) {
        perror("Error opening socket\n");
        exit(1);
    }

    server = gethostbyname("localhost"); 

    if (server == NULL) {
        fprintf(stderr, "No such host\n"); 
        exit(1); 
    } 

    memset((char *) &serv_addr, 0, 1000); 
    serv_addr.sin_family = AF_INET; 
    memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
    serv_addr.sin_port = htons(portno); 

    /* connecting to server */ 
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error connecting to server"); 
        exit(1);
    } 

    fds[0].fd = 0; 
    fds[1].fd = sockfd; 
    fds[0].events = POLLIN | POLLHUP | POLLERR; 
    fds[1].events = POLLIN | POLLHUP | POLLERR; 
    //check for sigpipe in both, send sigint to the shell from server 

    while(1) {
        ret = poll(fds, 2, 0);
        if(ret > 0) {
          /* check each of the 3 events */ 
          for(k = 0; k < 2; k++) {
            if (fds[k].revents & POLLIN) {
              /* reading from stdin */
              if(k == 0) {
                count = 0; 
                memset(buf, 0, 1024);
                memset(logbuf, 0, 1024);
                memset(compbuf, 0, 1024); 
                input = read (0, buf, 1024); 
                if(input < 0) {
                  fprintf(stderr, "Read error: %s\n", strerror(errno));
                  exit(1);
                }
                for (i = 0; i < input; i++) {
                    /* C-d or C-c */
                    if(buf[i] == '\004' || buf[i] == '\003') { 
                        temp = buf[i];
                        if(!c) {
                            if(write(sockfd, &temp, 1) < 0) { 
                                fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                                exit(1); 
                            } 
                        }
                        logbuf[count] = buf[i]; 
                        count++;
                        //compressing 
                        if(c) {
                            stdintosocket.avail_in = (uInt)input; 
                            stdintosocket.next_in = (Bytef *)logbuf; 
                            stdintosocket.avail_out = (uInt)sizeof(compbuf); 
                            stdintosocket.next_out = (Bytef *)compbuf; 
                            do {
                                deflate(&stdintosocket, Z_SYNC_FLUSH); 
                            } while(stdintosocket.avail_in > 0); 
                            count = (int)(sizeof(compbuf) - stdintosocket.avail_out); //stdintosocket.total_out; //not sure if number is total bytes out?
                            memcpy(logbuf, compbuf, 1024); 
                            for(n = 0; n < count; n++) {
                                temp = compbuf[n]; 
                                write(sockfd, &temp, 1); 
                            } 
                        }
                        //SENT %d bytes: %s\n
                        if(l) {
                            sprintf(tolog, "SENT %d bytes: ", count);
                            if(write(fd1, &tolog, strlen(tolog)) < 0) {
                                fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                                exit(1);
                            }  
                            if(write(fd1, &logbuf, count) < 0) {
                                fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                                exit(1);
                            }
                            sprintf(tolog, "\n"); 
                            if(write(fd1, &tolog, strlen(tolog)) < 0) {
                                fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                                exit(1);
                            } 
                        }
                        count = 0;
                        exit(0);  
                    }
                    /* <lf> or <cr> */
                    else if (buf[i] == '\n' || buf[i] == '\r') {
                        temp = '\r';
                        if(write(1, &temp, 1) < 0) { 
                            fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                            exit(1); 
                        } 
                        temp = '\n';
                        if(write(1, &temp, 1) < 0) { 
                            fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                            exit(1); 
                        }
                        if(!c) {
                            if(write(sockfd, &temp, 1) < 0) { 
                                fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                                exit(1); 
                            }
                        }
                        logbuf[count] = '\n'; 
                        count++;
                    }
                    /* write character to socket & stdout */ 
                    else { 
                        fprintf(stderr, "hello5");
                        temp = buf[i];
                        fprintf(stderr, "hello6");
                        if(write(1, &temp, 1) < 0) { 
                            fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                            exit(1); 
                        }
                        if(!c) {
                            if(write(sockfd, &temp, 1) < 0) { 
                                fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                                exit(1); 
                            }
                        }
                        fprintf(stderr, "hello7");
                        logbuf[count] = buf[i]; 
                        count++;
                        fprintf(stderr, "hello8");
                    }
                }
                // printf("hello");
                for(n = 0; n < input; n++) {
                    fprintf(stderr, "hello9");
                    temp = logbuf[n]; 
                    fprintf(stderr, "%s", &temp);
                    fprintf(stderr, "%d", count);
                    fprintf(stderr, "hello10");
                }
                //compressing 
                if(c) {
                    fprintf(stderr, "hello11");
                    stdintosocket.avail_in = (uInt)input; 
                    stdintosocket.next_in = (Bytef *)logbuf; 
                    stdintosocket.avail_out = (uInt)sizeof(compbuf); 
                    stdintosocket.next_out = (Bytef *)compbuf; 
                    fprintf(stderr, "hello12");
                    do {
                        retu = deflate(&stdintosocket, Z_SYNC_FLUSH);
                        if (retu != Z_OK && retu != Z_STREAM_END) {
                            fprintf(stderr, "%s", stdintosocket.msg);
                            exit(1);
                        }
                   
                    } while(stdintosocket.avail_in > 0); 
                    fprintf(stderr, "hello13");
                    count = (int)(sizeof(compbuf) - stdintosocket.avail_out); 
                    fprintf(stderr, "hello14");
                    for(n= 0; n < count; n++) {
                        temp = compbuf[n];
                        write(sockfd, &temp, 1); 
                    } 
                    fprintf(stderr, "hello15");   
                    memcpy(logbuf, compbuf, 1024);    
                }
                //SENT %d bytes: %s\n
                if(l) {
                    fprintf(stderr, "hello17");
                    sprintf(tolog, "SENT %d bytes: ", count);
                    if(write(fd1, &tolog, strlen(tolog)) < 0) {
                        fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                        exit(1);
                    }  
                    if(write(fd1, &logbuf, count) < 0) {
                        fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                        exit(1);
                    }
                    sprintf(tolog, "\n"); 
                    if(write(fd1, &tolog, strlen(tolog)) < 0) {
                        fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                        exit(1);
                    } 
                    fprintf(stderr, "hello18");
                }
                count = 0; 
              }
              /* reading from socket */
              else {
                count = 0; 
                memset(buf, 0, 1024);
                memset(logbuf, 0, 1024);
                memset(decompbuf, 0, 1024); 
                input = read (sockfd, buf, 1024); 
                if(input < 0) {
                  fprintf(stderr, "Read error: %s\n", strerror(errno));
                  exit(1);
                } 
                //RECEIVED %d bytes: %s\n
                if(l) {
                    sprintf(tolog, "RECEIVED %d bytes: ", input);
                    if(write(fd1, &tolog, strlen(tolog)) < 0) {
                        fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                        exit(1);
                    } 
                    if(write(fd1, &buf, input) < 0) {
                        fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                        exit(1);
                    } 
                    sprintf(tolog, "\n");
                    if(write(fd1, &tolog, strlen(tolog)) < 0) {
                        fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                        exit(1);
                    } 
                }
                //decompressing 
                if(c) {
                    sockettostdout.avail_in = (uInt)input; 
                    sockettostdout.next_in = (Bytef *)buf; 
                    sockettostdout.avail_out = (uInt)sizeof(decompbuf); 
                    sockettostdout.next_out = (Bytef *)decompbuf; 
                    do {
                        inflate(&sockettostdout, Z_SYNC_FLUSH); 
                    } while(sockettostdout.avail_in > 0); 
                    //memcpy(buf, decompbuf, 1024);  
                    input = (int)(sizeof(decompbuf) - sockettostdout.avail_out); 
                    for (i = 0; i < input; i++) {
                        /* <lf> */
                        if (decompbuf[i] == '\n') {
                            temp = '\r';
                            write(1, &temp, 1);
                            temp = '\n';
                            write(1, &temp, 1);
                        } 
                        /* write character to stdout */
                        else {
                            temp = decompbuf[i];
                            write(1, &temp, 1);
                        }
                    }
                }
                else {
                    for (i = 0; i < input; i++) {
                        /* <lf> */
                        if (buf[i] == '\n') {
                            temp = '\r';
                            write(1, &temp, 1);
                            temp = '\n';
                            write(1, &temp, 1);
                        } 
                        /* write character to stdout */
                        else {
                            temp = buf[i];
                            write(1, &temp, 1);
                        }
                    }
                }
                count = 0; 
              }
            }
            if (fds[k].revents & POLLHUP) {
              fprintf(stderr, "POLLHUP\n");
              exit(0);
            }
            if (fds[k].revents & POLLERR) {
              fprintf(stderr, "POLLERR\n");
              exit(1);
            }
          }
        }
        else if (ret == 0) {
          continue;
        }
        else {
          fprintf(stderr, "Poll command failed\n");
          exit(1);
        }
    }
    if(c) {
        deflateEnd(&stdintosocket); 
        inflateEnd(&sockettostdout); 
    }
}