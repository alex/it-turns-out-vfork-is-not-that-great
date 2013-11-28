#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "words.h"

void go(void);

char* bad_strdup_printf(const char* fmt, ...);

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
  char *argv[9];
  char buf[1024];
  if (pipe(ch_stdin) == -1 || pipe(ch_stdout) == -1 || pipe(report) == -1) {
    perror("pipe");
    exit(-1);
  }
  argv[0] = "./helper";
  argv[1] = bad_strdup_printf("-r%d", report[1]);
  argv[2] = bad_strdup_printf("-c%d", ch_stdin[1]);
  argv[3] = bad_strdup_printf("-c%d", ch_stdout[0]);
  argv[4] = bad_strdup_printf("-c%d", report[0]);
  argv[5] = bad_strdup_printf("-d%d,0", ch_stdin[0]);
  argv[6] = bad_strdup_printf("-d%d,1", ch_stdout[1]);
  argv[7] = "/usr/bin/sort";
  argv[8] = NULL;

  switch ((pid = vfork())) {
    case 0:
      execv("./helper", argv);
      _exit(-1);
    case -1:
      perror("vfork");
      exit(-1);
    default:
      break;
  }
  close(ch_stdin[0]);
  close(ch_stdout[1]);
  close(report[1]);

  for (i = 1; i < 7; ++i) {
    free(argv[i]);
  }

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

char* bad_strdup_printf(const char* fmt, ...) {
  va_list ap;
  int size;
  char* buf;
  va_start(ap, fmt);
  size = vsnprintf(NULL, 0, fmt, ap) + 1;
  if (size == -1) {
    perror("vsnprintf");
    exit(-1);
  }
  va_end(ap);
  buf = malloc(size);
  va_start(ap, fmt);
  vsnprintf(buf, size, fmt, ap);
  va_end(ap);
  return buf;
}
