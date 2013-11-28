#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "words.h"

void go(void);

int main(void) {
  const int n = 30000;
  for (int i = 0; i < n; ++i) {
    go();
  }
}

void go(void) {
  pid_t pid;
  int ch_stdin[2], ch_stdout[2], report[2];
  int i;
  ssize_t r;
  char buf[1024];
  if (pipe(ch_stdin) == -1 || pipe(ch_stdout) == -1 || pipe(report) == -1) {
    perror("pipe");
    exit(-1);
  }

  switch ((pid = fork())) {
    case 0:
      if (dup2(ch_stdin[0], 0) == -1
          || close(ch_stdin[1]) == -1
          || close(ch_stdout[0]) == -1
          || dup2(ch_stdout[1], 1) == -1
          || close(report[0]) == -1
          || fcntl(report[1], F_SETFD, FD_CLOEXEC) == -1
          || execlp("sort", "sort", NULL) == -1) {
        write(report[1], "welp something went wrong", 25);
      }
      _exit(-1);
    case -1:
      perror("fork");
      exit(-1);
    default:
      break;
  }
  close(ch_stdin[0]);
  close(ch_stdout[1]);
  close(report[1]);

  r = read(report[0], buf, sizeof buf);
  if (r == -1) {
    perror("read(report)");
    exit(-1);
  }
  if (r > 0) {
    fprintf(stderr, "child reported error: %.*s\n", (int) r, buf);
    exit(-1);
  }
  close(report[0]);
  r = write(ch_stdin[1], words, sizeof words);
  if (r == -1) {
    perror("write");
    exit(-1);
  }
  if (r < sizeof words) {
    fprintf(stderr, "write returned early\n");
    exit(-1);
  }
  close(ch_stdin[1]);
  r = read(ch_stdout[0], buf, sizeof buf);
  if (r == -1) {
    perror("read");
    exit(-1);
  }
  if (r < sizeof words) {
    fprintf(stderr, "read returned early\n");
    exit(-1);
  }

  close(ch_stdout[0]);
  waitpid(pid, &i, 0);
  if (WIFEXITED(i)) {
    if (WEXITSTATUS(i)) {
      fprintf(stderr, "child exit status: %d\n", (int) WEXITSTATUS(i));
      exit(-1);
    }

    return;
  } else if (WIFSIGNALED(i)) {
    fprintf(stderr, "child signaled: %d\n", (int) WIFSIGNALED(i));
    exit(-1);
  } else {
    fprintf(stderr, "child exit????: %d\n", (int) i);
    exit(-1);
  }
}
