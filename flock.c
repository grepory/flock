/* $Id: flock.c,v 1.8 2005/07/09 21:57:00 hpa Exp $ */
/* ----------------------------------------------------------------------- *
 *   
 *   Copyright 2003-2005 H. Peter Anvin - All Rights Reserved
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, Inc., 53 Temple Place Ste 330,
 *   Bostom MA 02111-1307, USA; either version 2 of the License, or
 *   (at your option) any later version; incorporated herein by reference.
 *
 * ----------------------------------------------------------------------- */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <paths.h>
#include <sysexits.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/wait.h>

static const struct option long_options[] = {
  { "shared",       0, NULL, 's' },
  { "exclusive",    0, NULL, 'x' },
  { "unlock",       0, NULL, 'u' },
  { "nonblocking",  0, NULL, 'n' },
  { "nb",           0, NULL, 'n' },
  { "timeout",      1, NULL, 'w' },
  { "wait",         1, NULL, 'w' },
  { "close",        0, NULL, 'o' },
  { "help",         0, NULL, 'h' },
  { "version",      0, NULL, 'V' },
  { 0, 0, 0, 0 }
};

const char *program;

static void usage(int ex)
{
  fprintf(stderr,
	  "flock version " VERSION "\n"
	  "Usage: %s [-seun][-w #] fd#\n"
	  "       %s [-sen][-w #] file [-c] command...\n"
	  "  -s  --shared     Get a shared lock\n"
	  "  -e  --exclusive  Get an exclusive lock\n"
	  "  -u  --unlock     Remove a lock\n"
	  "  -n  --nonblock   Fail rather than wait\n"
	  "  -w  --timeout    Wait for a limited amount of time\n"
	  "  -o  --close      Close file descriptor before running command\n"
	  "  -c  --command    Run a single command string through the shell\n"
	  "  -h  --help       Display this text\n"
	  "  -V  --version    Display version\n",
	  program, program);
  exit(ex);
}


static sig_atomic_t timeout_expired = 0;

static void timeout_handler(int sig)
{
  (void)sig;

  timeout_expired = 1;
}


static char * strtotimeval(const char *str, struct timeval *tv)
{
  char *s;
  long fs;			/* Fractional seconds */
  int i;
  
  tv->tv_sec = strtol(str, &s, 10);
  fs = 0;
  
  if ( *s == '.' ) {
    s++;
    
    for ( i = 0 ; i < 6 ; i++ ) {
      if ( !isdigit(*s) )
	break;

      fs *= 10;
      fs += *s++ - '0';
    }

    for ( ; i < 6; i++ )
      fs *= 10;

    while ( isdigit(*s) )
      s++;
  }

  tv->tv_usec = fs;
  return s;
}

int main(int argc, char *argv[])
{
  struct itimerval timeout, old_timer;
  int have_timeout = 0;
  int type = LOCK_EX;
  int block = 0;
  int fd = -1;
  int opt, ix;
  int do_close = 0;
  int err;
  int status;
  char *eon;
  char **cmd_argv = NULL, *sh_c_argv[4];
  const char *filename = NULL;
  struct sigaction sa, old_sa;

  program = argv[0];

  if ( argc < 2 )
    usage(EX_USAGE);
  
  memset(&timeout, 0, sizeof timeout);

  while ( (opt = getopt_long(argc, argv, "+sexnouw:h?", long_options, &ix)) != EOF ) {
    switch(opt) {
    case 's':
      type = LOCK_SH;
      break;
    case 'e':
    case 'x':
      type = LOCK_EX;
      break;
    case 'u':
      type = LOCK_UN;
      break;
    case 'o':
      do_close = 1;
      break;
    case 'n':
      block = LOCK_NB;
      break;
    case 'w':
      have_timeout = 1;
      eon = strtotimeval(optarg, &timeout.it_value);
      if ( *eon )
	usage(EX_USAGE);
      break;
    case 'V':
      fprintf(stderr, "flock " VERSION "\n");
      exit(0);
    default:
      usage((optopt != 'h' && optopt != '?') ? EX_USAGE : 0);
      break;
    }
  }

  if ( argc > optind+1 ) {
    /* Run command */

    if ( !strcmp(argv[optind+1], "-c") ||
	 !strcmp(argv[optind+1], "--command") ) {

      if ( argc != optind+3 ) {
	fprintf(stderr, "%s: %s requires exactly one command argument\n",
		program, argv[optind+1]);
	exit(EX_USAGE);
      }

      cmd_argv = sh_c_argv;

      cmd_argv[0] = getenv("SHELL");
      if ( !cmd_argv[0] || !*cmd_argv[0] )
	cmd_argv[0] = _PATH_BSHELL;

      cmd_argv[1] = "-c";
      cmd_argv[2] = argv[optind+2];
      cmd_argv[3] = 0;
    } else {
      cmd_argv = &argv[optind+1];
    }

    filename = argv[optind];
    fd = open(filename, O_RDONLY|O_CREAT, 0666);

    if ( fd < 0 ) {
      err = errno;
      fprintf(stderr, "%s: cannot open lock file %s: %s\n",
	      program, argv[optind], strerror(err));
      exit((err == ENOMEM||err == EMFILE||err == ENFILE) ? EX_OSERR :
	   (err == EROFS||err == ENOSPC) ? EX_CANTCREAT :
	   EX_NOINPUT);
    }

  } else {
    /* Use provided file descriptor */

    fd = (int)strtol(argv[optind], &eon, 10);
    if ( *eon || !argv[optind] ) {
      fprintf(stderr, "%s: bad number: %s\n", program, argv[optind]);
      exit(EX_USAGE);
    }

  }

  if ( have_timeout ) {
    memset(&sa, 0, sizeof sa);

    sa.sa_handler = timeout_handler;
    sa.sa_flags   = SA_RESETHAND;
    sigaction(SIGALRM, &sa, &old_sa);

    setitimer(ITIMER_REAL, &timeout, &old_timer);
  }

  while ( flock(fd, type|block) ) {
    switch( (err = errno) ) {
    case EWOULDBLOCK:		/* -n option set and failed to lock */
      exit(1);
    case EINTR:			/* Signal received */
      if ( timeout_expired )
	return 1;		/* -w option set and failed to lock */
      continue;			/* otherwise try again */
    default:			/* Other errors */
      if ( filename )
	fprintf(stderr, "%s: %s: %s\n", program, filename, strerror(err));
      else
	fprintf(stderr, "%s: %d: %s\n", program, fd, strerror(err));
      exit((err == ENOLCK||err == ENOMEM) ? EX_OSERR : EX_DATAERR);
    }
  }

  if ( have_timeout ) {
    setitimer(ITIMER_REAL, &old_timer, NULL); /* Cancel itimer */
    sigaction(SIGALRM, &old_sa, NULL); /* Cancel signal handler */
  }  

  status = 0;
  
  if ( cmd_argv ) {
    pid_t w, f;

    f = fork();

    if ( f < 0 ) {
      err = errno;
      fprintf(stderr, "%s: fork: %s\n", program, strerror(err));
      exit(EX_OSERR);
    } else if ( f == 0 ) {
      if ( do_close )
	close(fd);
      err = errno;
      execvp(cmd_argv[0], cmd_argv);
      /* execvp() failed */
      fprintf(stderr, "%s: %s: %s\n", program, cmd_argv[0], strerror(err));
      _exit((err == ENOMEM) ? EX_OSERR: EX_UNAVAILABLE);
    } else {
      do {
	w = waitpid(f, &status, 0);
      } while ( w != f );

      if ( WIFEXITED(status) )
	status = WEXITSTATUS(status);
      else if ( WIFSIGNALED(status) )
	status = WTERMSIG(status) + 128;
      else
	status = EX_OSERR;	/* WTF? */
    }
  }

  return status;
}

