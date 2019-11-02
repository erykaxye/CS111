//what 
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
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h> 
#include <sys/types.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h> 
#include <assert.h> 
#include "zlib.h"

pid_t child;

void sigpipehandler() {
  exit(0);
}

void shutdown_handler(void) {
    int status;
    if(waitpid(child, &status, 0) == -1) {
        fprintf(stderr, "Error on waitpid\n");
        exit(1);
    }
    int a = (status & 0x007f);
    int b = ((status & 0xff00) >> 8);
    fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", a, b); 
}

int main(int argc, char **argv) {

    /* declare all variables */ 
    char temp, buf[1024], *execvp_argv[1], compbuf[1024], decompbuf[1024];
    int opt, c, sockfd, newsockfd, portno, i, k, ret, end, input, n, port_given, to_child_pipe[2], from_child_pipe[2]; 
    socklen_t clilen; 
    z_stream sockettoshell, shelltosocket;  
    struct sockaddr_in serv_addr, cli_addr; 
    struct pollfd fds[2];

    static struct option long_options[] =
    {
        {"port", required_argument, NULL, 'p'},
        {"compress", no_argument, NULL, 'c'},
        {0, 0, 0, 0}
    };

    /* initialize variables */
    c = 0; 
    end = 0;
    port_given = 0;

    /* check optional arguments */
    while((opt = getopt_long(argc, argv, "pc:", long_options, NULL)) != -1) {
        switch(opt){
        case 'p':
            portno = atoi(optarg); 
            port_given = 1; 
            break;
        case 'c':
            c = 1;
            shelltosocket.zalloc = Z_NULL; 
            shelltosocket.zfree = Z_NULL; 
            shelltosocket.opaque = Z_NULL;
            if(deflateInit(&shelltosocket, Z_DEFAULT_COMPRESSION) < 0) {
                fprintf(stderr, "Cannot initiailze compression");
                exit(1);
            }
            sockettoshell.zalloc = Z_NULL; 
            sockettoshell.zfree = Z_NULL; 
            sockettoshell.opaque = Z_NULL;
            if(inflateInit(&sockettoshell) < 0) {
                fprintf(stderr, "Cannot initiailze compression");
                exit(1);
            }
            break;
        default:
            fprintf(stderr, "Invalid Argument\n");
            exit(1);
        }
    }

    signal(SIGPIPE, sigpipehandler); 

    if(!port_given) { 
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        exit(1);
    }

    /* create a socket point */
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 

    if (sockfd < 0) {
        perror("Error opening socket\n");
        exit(1);
    }

    /* initialize socket structure */
    memset((char *) &serv_addr, 0, 1000);  
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_addr.s_addr = INADDR_ANY;    
    serv_addr.sin_port = htons(portno); 

    /* bind host address using bind() */
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error binding host address");
        exit(1);
    }

    listen(sockfd, 5); 
    clilen = sizeof(cli_addr); 

    /* accept connection from the client */
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
	
    if (newsockfd < 0) {
        perror("ERROR on accept");
        exit(1);
    }

    /* if connected, start shell*/
    if(pipe(to_child_pipe) < 0) {
      fprintf(stderr, "Could not create pipe 1\n");
      exit(1);
    }
    if(pipe(from_child_pipe) < 0) {
      fprintf(stderr, "Could not create pipe 2\n");
      exit(1);
    }

    child = fork();

    atexit(shutdown_handler);

    /* parent process */
    if (child > 0) {
      if(close(to_child_pipe[0]) < 0) { 
        fprintf(stderr, "Close error: %s\n", strerror(errno));
        exit(1);
      }
      if(close(from_child_pipe[1]) < 0) { 
        fprintf(stderr, "Close error: %s\n", strerror(errno));
        exit(1);
      }

      fds[0].fd = newsockfd;
      fds[1].fd = from_child_pipe[0]; 
      fds[0].events = POLLIN | POLLHUP | POLLERR; 
      fds[1].events = POLLIN | POLLHUP | POLLERR; 
      //check for sigpipe in both, send sigint to the shell from server 

      while(1) {
        ret = poll(fds, 2, 0);
        if(ret > 0) {
          /* check each of the 3 events */ 
          for(k = 0; k < 2; k++) {
            if (fds[k].revents & POLLIN) {
              /* reading from socket */
              if(k == 0) {
                memset(buf, 0, 1024); 
                input = read (newsockfd, buf, 1024); 
                if(input < 0) {
                  fprintf(stderr, "Read error: %s\n", strerror(errno));
                  exit(1);
                } 
                //decompress what you read in and send to shell 
                if(c) {
                    memset(decompbuf, 0, 1024); 
                    sockettoshell.avail_in = input; 
                    sockettoshell.next_in = (Bytef *)buf; 
                    sockettoshell.avail_out = (uInt)sizeof(decompbuf); 
                    sockettoshell.next_out = (Bytef *)decompbuf; 
                    do {
                        inflate(&sockettoshell, Z_SYNC_FLUSH); 
                    } while(sockettoshell.avail_in > 0); 
                    //memcpy(buf, decompbuf, 1024);   
                    input = (int)strlen(decompbuf) - sockettoshell.avail_out; 
                    for (i = 0; i < input; i++) {
                        /* C-d */
                        if (decompbuf[i] == '\004') {
                            end = 1;
                            if(close(to_child_pipe[1]) < 0) { 
                            fprintf(stderr, "Close error: %s\n", strerror(errno));
                            exit(1);
                            }
                            break;
                        }
                        /* C-c */
                        else if (decompbuf[i] == '\003') {
                            if(kill(child, SIGINT) != 0) {
                            fprintf(stderr, "Could not kill program\n");
                            exit(1);
                            }
                        }
                        else {
                            temp = decompbuf[i];
                            if(write(to_child_pipe[1], &temp, 1) < 0) { 
                            fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                            exit(1); 
                            }
                        }
                    }

                }
                else {
                    for (i = 0; i < input; i++) {
                        /* C-d */
                        if (buf[i] == '\004') {
                            end = 1;
                            if(close(to_child_pipe[1]) < 0) { 
                            fprintf(stderr, "Close error: %s\n", strerror(errno));
                            exit(1);
                            }
                            break;
                        }
                        /* C-c */
                        else if (buf[i] == '\003') {
                            if(kill(child, SIGINT) != 0) {
                            fprintf(stderr, "Could not kill program\n");
                            exit(1);
                            }
                        }
                        else {
                            temp = buf[i];
                            if(write(to_child_pipe[1], &temp, 1) < 0) { 
                            fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                            exit(1); 
                            }
                        }
                    }
                }
              }
              /* reading from child pipe (shell) */
              else {
                memset(buf, 0, 1024); 
                input = read (from_child_pipe[0], buf, 1024);
                if(input < 0) {
                  fprintf(stderr, "Read error: %s\n", strerror(errno));
                  exit(1);
                }
                for (i = 0; i < input; i++) {
                  /* C-d */
                  if (buf[i] == '\004') {
                    end = 1;
                    break;
                  }
                  /* <lf> */
                  else if (buf[i] == '\n') {
                    if(!c) {
                        temp = '\r';
                        write(newsockfd, &temp, 1);
                        temp = '\n';
                        write(newsockfd, &temp, 1);
                    }
                  }
                  else { 
                    if(!c) {
                        temp = buf[i];
                        write(newsockfd, &temp, 1);
                    }
                  }
                }
                if(c) {
                    //compress and write to newsockfd 
                    memset(compbuf, 0, 1024); 
                    shelltosocket.avail_in = input; 
                    shelltosocket.next_in = (Bytef *)buf; 
                    shelltosocket.avail_out = (uInt)sizeof(compbuf); 
                    shelltosocket.next_out = (Bytef *)compbuf; 
                    do {
                        deflate(&shelltosocket, Z_SYNC_FLUSH); 
                    } while(shelltosocket.avail_in > 0); 
                    input = (int)sizeof(compbuf) - shelltosocket.avail_out; 
                    for(n = 0; n < input; n++) {
                        temp = compbuf[n];
                        write(newsockfd, &temp, 1); 
                    }
                    input = 0; 
                }
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
        if(end) 
          break;
      }
    }
    /* child process */
    else if (child == 0) {
      if(close(to_child_pipe[1]) < 0) { 
        fprintf(stderr, "Close error: %s\n", strerror(errno));
        exit(1);
      }
      if(close(from_child_pipe[0]) < 0) { 
        fprintf(stderr, "Close error: %s\n", strerror(errno));
        exit(1);
      }
      dup2(to_child_pipe[0], 0);
      if(close(to_child_pipe[0]) < 0) { 
        fprintf(stderr, "Close error: %s\n", strerror(errno));
        exit(1);
      }
      dup2(from_child_pipe[1], 1);
      dup2(from_child_pipe[1], 2);
      if(close(from_child_pipe[1]) < 0) { 
        fprintf(stderr, "Close error: %s\n", strerror(errno));
        exit(1);
      }

      execvp_argv[0] = NULL;
      if(execvp("/bin/bash", execvp_argv) == -1) {
        fprintf(stderr, "Error opening shell\n");
        exit(1);
      }
    }
    /* fork failed */
    else {
      fprintf(stderr, "Fork failed\n");
      exit(1);
    }

    close(sockfd); 
    close(newsockfd); 
    if(c) {
        deflateEnd(&stdintosocket); 
        inflateEnd(&sockettostdout); 
    } 

    return EXIT_SUCCESS;
}

