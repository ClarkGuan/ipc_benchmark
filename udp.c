#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

double getdetlatimeofday(struct timeval *begin, struct timeval *end) {
  return (end->tv_sec + end->tv_usec * 1.0 / 1000000) -
         (begin->tv_sec + begin->tv_usec * 1.0 / 1000000);
}

//#define USEC 1000000

//ssize_t recv_timed(int fd, void *buf, size_t n, int flags, long us) {
//  struct timeval dst = {.tv_sec = us / USEC, .tv_usec = us % USEC};
//  struct timeval now;
//  if (gettimeofday(&now, NULL) == -1) {
//    perror("gettimeofday");
//    exit(1);
//  }
//  dst.tv_sec += now.tv_sec;
//  dst.tv_usec += now.tv_usec;
//  if (setitimer(ITIMER_REAL, (const struct itimerval *) &dst, NULL) == -1) {
//    perror("setitimer");
//    exit(1);
//  }
//  ssize_t ret = recv(fd, buf, n, flags);
//  if (ret == -1 && errno == EINTR) {
//    struct timeval after;
//    if (gettimeofday(&after, NULL) == -1) {
//      perror("gettimeofday");
//      exit(1);
//    }
//    long spent = (after.tv_sec - now.tv_sec) * USEC + (after.tv_usec - now.tv_usec);
//    if (spent >= us) {
//      errno = ETIMEDOUT;
//    }
//  }
//
//  return ret;
//}

int main(int argc, char *argv[]) {
  int fd, yes;
  int i, size, count, sum;
  ssize_t n;
  char *buf;
  struct timeval begin, end;
  struct sockaddr_in in;

  if (argc != 3) {
    printf("usage: ./udp <size> <count>\n");
    return 1;
  }
//  signal(SIGALRM, SIG_IGN);

  size = atoi(argv[1]);
  count = atoi(argv[2]);
  buf = malloc(size);

  memset(&in, 0, sizeof(in));
  if (fork() == 0) { // parent
    fd = socket(AF_INET, SOCK_DGRAM, 0);

    in.sin_family = AF_INET;
    in.sin_port = htons(15000);
    inet_pton(AF_INET, "127.0.0.1", &in.sin_addr);

    yes = 1;
    struct timeval timeout = { .tv_sec = 3 };
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    if (bind(fd, (struct sockaddr *) &in, sizeof(in)) == -1) {
      perror("bind");
      return 1;
    }

    sum = 0;
    for (i = 0; i < count; i++) {
      n = recv(fd, buf, size, 0);
      if (n == 0) {
        break;
      }
      if (n == -1) {
        perror("recv");
        return 1;
      }
      sum += n;
    }
    if (sum != count * size) {
      fprintf(stderr, "sum error: %d != %d\n", sum, count * size);
      return 1;
    }
    close(fd);
  } else {
    sleep(1);

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    in.sin_family = AF_INET;
    in.sin_port = htons(15000);
    inet_pton(AF_INET, "127.0.0.1", &in.sin_addr);

    gettimeofday(&begin, NULL);

    for (i = 0; i < count; i++) {
      if (sendto(fd, buf, size, 0, (struct sockaddr *) &in, sizeof(in)) != size) {
        perror("sendto");
        return 1;
      }
    }

    gettimeofday(&end, NULL);

    double tm = getdetlatimeofday(&begin, &end);
    printf("%.0fMB/s %.0fmsg/s\n",
           count * size * 1.0 / (tm * 1024 * 1024),
           count * 1.0 / tm);
  }

  return 0;
}
