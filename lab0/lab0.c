/*
NAME: Erica Xie
EMAIL: ericaxie@ucla.edu
ID: 404720875
*/

#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h> 

void handler(int sig) {
  fprintf(stderr, "Segmentation Fault Caught: %d\n", sig);
  exit(4);
}

int main(int argc, char *argv[])
{

  /* declare all variables */
  char buf[1], *ptr;
  int n, opt, s, c, fd0, fd1;
  static struct option long_options[] =
    {
      {"input", required_argument, NULL, 'i'},
      {"output", required_argument, NULL, 'o'},
      {"segfault", no_argument, NULL, 's'},
      {"catch", no_argument, NULL, 'c'},
    }; 
  
  /* initialize variables */
  n = 1;
  s = 0;
  c = 0;
  fd0 = 0;
  fd1 = 1; 
  
  /* check optional arguments */
  if(argc > 1) {
    while((opt = getopt_long(argc, argv, "iosc:", long_options, NULL)) != -1) {
      switch(opt){
      case 'i':
        fd0 = open(optarg, O_RDONLY);
	if (fd0 >= 0) {
	  close(0);
	  dup2(fd0, 0);
	} 
	else {
	  fprintf(stderr, "Cannot open file: %s given by --input\n%s\n", optarg, strerror(errno));
	  exit(2);
	}
	break; 
      case 'o':
	fd1 = creat(optarg, 0644);
	if (fd1 >= 0) {
	  close(1); 
	  dup2(fd1, 1);
	}
	else {
	  fprintf(stderr, "Cannot create file: %s given by --output\n%s\n", optarg, strerror(errno));
	  exit(3);
	}
	break;
      case 's':
	s = 1;
	break; 
      case 'c':
	c = 1;
	break; 
      default:
	fprintf(stderr, "Invalid Argument\n");
	exit(1);
	break; 
      }
    }
  }
  
  /* register the signal handler */
  if(c) {
    signal(SIGSEGV, handler);
  }

  /* make segmentation fault */
  if(s) {
    ptr = NULL;
    *ptr = 'o';
  }

  while(read(fd0, buf, n) > 0) {
    if(write(fd1, buf, n) < 0) {
      fprintf(stderr, "File error: %s\n", strerror(errno));
    }
  } 
  exit(0);
}
