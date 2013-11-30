#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>


static struct option opts[] = {
  { "dup", 1, NULL, 'd' },
  { "open", 1, NULL, 'o' },
  { "close", 1, NULL, 'c' },
  { "report", 1, NULL, 'r' },
  { NULL, 0, NULL, 0 }
};

static int on_opt_dup(char* arg, int report_fd);
static int on_opt_open(char* arg, int report_fd);
static int on_opt_close(char* arg, int report_fd);
static int on_opt_report(char* arg);

static void report(int fd, const char* context, const char* token, int err);


int main(int argc, char* argv[]) {
  char c;
  char **nonopts;
  int report_fd;
  int fd_flags;

  report_fd = -1;
  while ((c = getopt_long(argc, argv, "+d:o:c:r:", opts, NULL)) != -1) {
    switch (c) {
    case 'd':
      if (on_opt_dup(optarg, report_fd) == -1)
        return -1;
      break;
    case 'o':
      if (on_opt_open(optarg, report_fd) == -1)
        return -1;
      break;
    case 'c':
      if (on_opt_close(optarg, report_fd) == -1)
        return -1;
      break;
    case 'r':
      report_fd = on_opt_report(optarg);
      if (report_fd == -1)
        return -1;
      break;
    default:
      return -1;
    }
  }
  nonopts = argv + optind;
  if (*nonopts == NULL) {
    report(report_fd, "main", "getopt_long", 0);
    return -1;
  }
  if (report_fd != -1) {
    fd_flags = fcntl(report_fd, F_GETFD);
    if (fd_flags == -1
        || fcntl(report_fd, F_SETFD, fd_flags | FD_CLOEXEC) == -1) {
      report(report_fd, "main", "fcntl", errno);
      return -1;
    }
  }
  execvp(nonopts[0], nonopts);
  report(report_fd, "main", "exec", errno);
  return -1;
}

static int on_opt_dup(char* arg, int report_fd) {
  int fd1, fd2;
  char* endptr;

  if (*arg == '\0') {
    report(report_fd, "dup", "strtol", 0);
    return -1;
  }

  fd1 = strtol(arg, &endptr, 0);
  if (*endptr != ',') {
    report(report_fd, "dup", "strtol", 0);
    return -1;
  }
  ++endptr;
  fd2 = strtol(endptr, &endptr, 0);
  if (*endptr != '\0'){
    report(report_fd, "dup", "strtol", 0);
    return -1;
  }

  if (fd1 == fd2)
    return 0;

  if (dup2(fd1, fd2) == -1) {
    report(report_fd, "dup", "dup2", errno);
    return -1;
  }

  if (close(fd1) == -1) {
    report(report_fd, "dup", "close", errno);
    return -1;
  }

  return 0;
}

static int on_opt_open(char* arg, int report_fd) {
  int fd, flags;
  mode_t mode;
  char* endptr;
  int open_fd;

  if (*arg == '\0') {
    report(report_fd, "open", "strtol", 0);
    return -1;
  }

  fd = strtol(arg, &endptr, 0);
  if (*endptr != ',') {
    report(report_fd, "open", "strtol", 0);
    return -1;
  }
  ++endptr;
  flags = strtol(endptr, &endptr, 0);
  if (*endptr != ',') {
    report(report_fd, "open", "strtol", 0);
    return -1;
  }
  ++endptr;
  mode = strtol(endptr, &endptr, 0);
  if (*endptr != ',') {
    report(report_fd, "open", "strtol", 0);
    return -1;
  }
  ++endptr;

  open_fd = open(endptr, flags, mode);
  if (open_fd == -1) {
    report(report_fd, "open", "open", errno);
    return -1;
  }

  if (open_fd != fd) {
    int r;

    r = dup2(open_fd, fd);
    if (r == -1) {
      report(report_fd, "open", "dup2", errno);
      close(open_fd);
      return -1;
    }
    if (close(open_fd) == -1) {
      report(report_fd, "open", "close", errno);
      return -1;
    }
  }
  return 0;
}

static int on_opt_close(char* arg, int report_fd) {
  int fd;
  char* endptr;

  if (*arg == '\0') {
    report(report_fd, "close", "strtol", 0);
    return -1;
  }

  fd = strtol(arg, &endptr, 0);
  if (*endptr != '\0'){
    report(report_fd, "close", "strtol", 0);
    return -1;
  }

  if (close(fd) == -1) {
    report(report_fd, "close", "close", errno);
    return -1;
  }

  return 0;
}

static int on_opt_report(char* arg) {
  int fd;
  char* endptr;

  if (*arg == '\0')
    return -1;

  fd = strtol(arg, &endptr, 10);
  if (*endptr != '\0')
    return -1;

  return fd;
}

static void report(int fd, const char* context, const char* func, int err) {
  char buf[16];
  int n;
  if (fd == -1)
    return;

  write(fd, context, strlen(context));
  write(fd, ":", 1);
  write(fd, func, strlen(func));
  write(fd, ":", 1);
  n = sprintf(buf, "%d", err);
  write(fd, buf, n);
}
