/*
NAME: Erica Xie
EMAIL: ericaxie@ucla.edu
ID: 404920875 
*/

#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h> 
#include <math.h> 
#include <signal.h>
#include <mraa.h> 
#include <time.h> 
#include <sys/resource.h> 
#include <poll.h> 
#include <netdb.h> 
#include <sys/socket.h> 
#include <openssl/ssl.h> 
#include <openssl/err.h>

int run = 1; //run flag 
int l = 0; //log flag
int fd1 = 1; 
char buf[2048]; 
float temp = 0; 
time_t curtime; 
struct tm *info; 
int hour = 0; 
int min = 0; 
int sec = 0; 
int sockfd = 0; 
SSL *ssl_client = 0; 

void interrupt() {

    /* find current time */
    time(&curtime); 
    info = localtime(&curtime); 
    hour = info->tm_hour; 
    min = info->tm_min; 
    sec = info->tm_sec; 

    sprintf(buf, "%.2d:%.2d:%.2d SHUTDOWN\n", hour, min, sec);
    if(l) {
        if(write(fd1, &buf, strlen(buf)) < 0) {
            fprintf(stderr, "Write error: %s\n", strerror(errno)); 
            fflush(stderr); 
            exit(1);
        }
    }
    if(SSL_write(ssl_client, &buf, strlen(buf)) < 0) {
        fprintf(stderr, "Write error: %s\n", strerror(errno));
        fflush(stderr); 
        exit(1);
    }

    memset(buf, 0, 2048);
    run = 0; 

}

int main(int argc, char *argv[])
{
    /* declare all variables */
    char ibuf[2048], cbuf[2048], pbuf[2048], c, *host; 
    int a, n, k, opt, p, f, input, output, ret, count, stopped;
    int port, id, index;  
    float r; 
    struct sockaddr_in serv_addr; 
    struct hostent *server;
    mraa_aio_context temp_sensor; 
    struct pollfd fds[1];

    static struct option long_options[] =
    {
        {"period", required_argument, NULL, 'p'},
        {"scale", required_argument, NULL, 's'},
        {"log", required_argument, NULL, 'l'},
        {"id", required_argument, NULL, 'i'},
        {"host", required_argument, NULL, 'h'},
        {0, 0, 0, 0}
    }; 
  
    /* initialize variables */
    a = 0; 
    n = 0; 
    p = 1; 
    k = 0;
    f = 1; 
    r = 0; 
    count = 0; 
    stopped = 0; 
    port = 0; 
    id = 0; 
    index = 0; 
    host = NULL; 
  
    /* check optional arguments */
    if(argc > 1) {
        while((opt = getopt_long(argc, argv, "pslih:", long_options, NULL)) != -1) {
            switch(opt) {
            case 'p':
                p = atoi(optarg); 
                break; 
            case 's':
                if(strcmp("F", optarg) == 0) 
                    f = 1; 
                else if(strcmp("C", optarg) == 0) 
                    f = 0; 
                else {
                    fprintf(stderr, "Invalid Input to Scale\n");
                    fflush(stderr); 
                    exit(1);
                }
                break;
            case 'l':
                l = 1; 
                output = creat(optarg, 0644); 
                if (output < 0) {
                    fprintf(stderr, "Cannot create file: %s given by --output\n%s\n", optarg, strerror(errno));
                    fflush(stderr);
                    exit(1);
                }
                fd1 = open(optarg, O_WRONLY);
                if (fd1 < 0) {
                    fprintf(stderr, "Cannot open file: %s given by --log\n%s\n", optarg, strerror(errno));
                    fflush(stderr); 
                    exit(1);
                }
                break;
            case 'i': 
                id = atoi(optarg); 
                break;
            case 'h': 
                host = optarg; 
                break;
            default:
                fprintf(stderr, "Invalid Argument\n");
                fflush(stderr); 
                exit(1);
                break; 
            }
        }
    }

    /* check arguments for port */
    for (index = optind; index < argc; index++) {
        port = atoi(argv[index]);
    }

    if(host == NULL || port == 0 || id == 0 || l == 0) {
        fprintf(stderr, "Missing Argument(s)\n");
        fflush(stderr); 
        exit(1); 
    }

    /* initialize temp */
    temp_sensor = mraa_aio_init(1); 
    if (temp_sensor == NULL) { 
        fprintf(stderr, "Failed to initialize AIO\n");
        mraa_deinit();
        fflush(stderr); 
        exit(2);
    }  

    SSL_library_init(); 
    SSL_load_error_strings(); 
    OpenSSL_add_all_algorithms(); 

    SSL_CTX *ssl_context = SSL_CTX_new(TLSv1_client_method()); 
    if(ssl_context == NULL) {
        fprintf(stderr, "Error creating context\n"); 
        fflush(stderr); 
        exit(2);
    }

    ssl_client = SSL_new(ssl_context); 
    if (ssl_client == NULL) {
        fprintf(stderr, "Error creating client\n"); 
        fflush(stderr); 
        exit(2);
    } 

    /* create socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if(sockfd == -1) {
        fprintf(stderr, "Failed to initialize socket\n");
        fflush(stderr); 
        exit(2);
    }

    server = gethostbyname(host); 
    if (server == NULL) {
        fprintf(stderr, "No such host\n"); 
        exit(2); 
    }

    memset((char *) &serv_addr, 0, 1000);
    serv_addr.sin_family = AF_INET; 
    memcpy((char *) &serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
    serv_addr.sin_port = htons(port); 

    if(!SSL_set_fd(ssl_client, sockfd)){
        fprintf(stderr, "Error setting sockfd\n"); 
        fflush(stderr); 
        exit(2);
    }

    /* connecting to server */
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "Error connecting to server\n"); 
        fflush(stderr); 
        exit(2);
    } 

    if(SSL_connect(ssl_client) != 1) {
        fprintf(stderr, "Error connecting to client\n"); 
        fflush(stderr); 
        exit(2);
    }

    fds[0].fd = sockfd; 
    fds[0].events = POLLIN | POLLHUP | POLLERR; 

    sprintf(buf, "ID=%d\n", id); 
    if(SSL_write(ssl_client, &buf, strlen(buf)) < 0) {
        fprintf(stderr, "Write error: %s\n", strerror(errno)); 
        fflush(stderr); 
        exit(2);
    }
    if(l) {
        if(write(fd1, &buf, strlen(buf)) < 0) {
            fprintf(stderr, "Write error: %s\n", strerror(errno)); 
            fflush(stderr); 
            exit(2);
        }
    }
    memset(buf, 0, 2048);

    while(run) {
        ret = poll(fds, 1, 0);
        if(ret > 0) {
            /* check each of the 3 events */ 
            if (fds[0].revents & POLLIN) { 
                memset(ibuf, 0, 2048);
                input = SSL_read(ssl_client, ibuf, 2048); 
                if(input < 0) {
                    fprintf(stderr, "Read error: %s\n", strerror(errno));
                    fflush(stderr);
                    exit(2);
                }
                for (n = 0; n < input; n++) {
                    c = ibuf[n]; 
                    cbuf[count] = c; 
                    count++; 
                    /* <lf> */
                    if (c == '\n') { 
                        /* find which command and execute */
                        if(strncmp("SCALE=F\n", cbuf, 8) == 0) 
                            f = 1; 
                        else if(strncmp("SCALE=C\n", cbuf, 8) == 0) 
                            f = 0; 
                        else if(strncmp("PERIOD=", cbuf, 7) == 0) {  
                            int i = 0; 
                            if(count > 8) {
                                for(k = 7; k < count; k++) { 
                                    if(cbuf[k] != '\n') {
                                        pbuf[i] = cbuf[k]; 
                                        i++;
                                    } 
                                } 
                                int new_period = atoi(pbuf); 
                                if(new_period > 0)
                                    p = new_period;  
                            } 
                        }
                        else if(strncmp("STOP\n", cbuf, 5) == 0) 
                            stopped = 1; 
                        else if(strncmp("START\n", cbuf, 6) == 0) 
                            stopped = 0; 
                        else if(strncmp("OFF\n", cbuf, 4) == 0) 
                            interrupt(); 
                        /* write command to log */
                        if(l) {
                            if(write(fd1, cbuf, count) < 0) {
                                fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                                fflush(stderr); 
                                exit(2);
                            }
                        }
                        /* reset for next command */ 
                        memset(cbuf, 0, 2048);
                        count = 0; 
                    }
                }
            }
            if (fds[0].revents & POLLHUP) {
              fprintf(stderr, "POLLHUP\n");
              fflush(stderr);
              break; 
            }
            if (fds[0].revents & POLLERR) {
              fprintf(stderr, "POLLERR\n");
              fflush(stderr);
              exit(2);
            }
        } 
        else if (ret < 0) {
          fprintf(stderr, "Poll command failed\n");
          fflush(stderr);
          exit(2);
        }

        /* find current time */
        time(&curtime); 
        info = localtime(&curtime); 
        hour = info->tm_hour; 
        min = info->tm_min; 
        sec = info->tm_sec; 

        /* find temp */
        a = mraa_aio_read(temp_sensor); 
        r = 1023.0/a - 1.0; 
        r = 100000 * r; 
        temp = 1.0/(log(r/100000)/4275 + 1/298.15) - 273.15; //celsius 
        if(f) { //fahrenheit 
            temp = temp*9/5 + 32; 
        }

        /* report */ 
        if(run && !stopped) {
            sprintf(buf, "%.2d:%.2d:%.2d %0.1f\n", hour, min, sec, temp);
            if(l) {
                if(write(fd1, &buf, strlen(buf)) < 0) {
                    fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                    fflush(stderr); 
                    exit(2);
                }
            }
            if(SSL_write(ssl_client, &buf, strlen(buf)) < 0) {
                fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                fflush(stderr); 
                exit(2);
            }
        }

        memset(buf, 0, 2048);
        sleep(p);

    } 

    if(mraa_aio_close(temp_sensor) != MRAA_SUCCESS) {
        fprintf(stderr, "Error closing temp_sensor\n"); 
        fflush(stderr); 
        exit(2);
    }

    close(sockfd); 
    SSL_shutdown(ssl_client); 
    SSL_free(ssl_client); 
    exit(0); 
    
}

/*

Links: 
https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html#Example-of-Getopt 
https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/ 

*/
