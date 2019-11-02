/*
NAME: Erica Xie
EMAIL: ericaxie@ucla.edu
ID: 404920875
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <getopt.h>
#include <sys/types.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h> 
#include <errno.h> 

/* Use this variable to remember original terminal attributes. */

struct termios saved_attributes;
pid_t child;

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
  if (!isatty (STDIN_FILENO))
    {
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

void shutdown_handler(void) {
  int status; 
  if(waitpid(child, &status, 0) == -1) {
    fprintf(stderr, "Error on waitpid\n");
    exit(1);
  }
  int a = (status & 0x007f);
  int b = ((status & 0xff00) >> 8);
  fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d", a, b);
}

int main(int argc, char **argv) {

  /* declare all variables */
  char buf[256], temp, *execvp_argv[1];
  int n, opt, i, k, input, end, s, ret, to_child_pipe[2], from_child_pipe[2];

  static struct option long_options[] =
    {
      {"shell", no_argument, NULL, 's'},
      {0, 0, 0, 0}
    };

  struct pollfd fds[2];

  /* initialize variables */
  n = 256;
  end = 0;
  s = 0;

  /* check optional arguments */
  while((opt = getopt_long(argc, argv, "s:", long_options, NULL)) != -1) {
    switch(opt){
    case 's':
      s = 1;
      break;
    default:
      fprintf(stderr, "Invalid Argument\n");
      exit(1);
    }
  }

  set_input_mode();

  if(s) {
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

      fds[0].fd = 0;
      fds[1].fd = from_child_pipe[0];
      fds[0].events = POLLIN | POLLHUP | POLLERR;
      fds[1].events = POLLIN | POLLHUP | POLLERR;

      while(1) {
        ret = poll(fds, 2, 0);
        if(ret > 0) {
          /* check each of the 3 events */ 
          for(k = 0; k < 2; k++) {
            if (fds[k].revents & POLLIN) {
              /* reading from stdin */
              if(k == 0) {
                input = read (0, buf, n); 
                if(input < 0) {
                  fprintf(stderr, "Read error: %s\n", strerror(errno));
                  exit(1);
                }
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
                    if(write(to_child_pipe[1], &temp, 1) < 0) { 
                      fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                      exit(1); 
                    }
                  }
                  else {
                    temp = buf[i];
                    if(write(1, &temp, 1) < 0) { 
                      fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                      exit(1); 
                    }
                    if(write(to_child_pipe[1], &temp, 1) < 0) { 
                      fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                      exit(1); 
                    }
                  }
                }
              }
              /* reading from child pipe */
              else {
                input = read (from_child_pipe[0], buf, n);
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
                    temp = '\r';
                    write(1, &temp, 1);
                    temp = '\n';
                    write(1, &temp, 1);
                  }
                  else {
                    temp = buf[i];
                    write(1, &temp, 1);
                  }
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
  }

  else {
    while (1) {
      input = read (0, buf, n);
      if(input < 0) {
        fprintf(stderr, "Close error: %s\n", strerror(errno));
        exit(1);
      }
      for (i = 0; i < input; i++) {
	       /* C-d */
	       if (buf[i] == '\004') {
	          end = 1;
            break;
	       }
	       /* <cr> or <lf> */
	       else if (buf[i] == '\n' || buf[i] == '\r') {
	          temp = '\r';
	          write(1, &temp, 1);
	          temp = '\n';
	          write(1, &temp, 1);
	       }
	       else {
	          temp = buf[i];
	          write(1, &temp, 1);
	       }
      }
      if(end)
	     break;
    }
  }

  return EXIT_SUCCESS;

}
