#include <mqueue.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

double
getdetlatimeofday(struct timeval *begin, struct timeval *end)
{
  return (end->tv_sec + end->tv_usec * 1.0 / 1000000) -
         (begin->tv_sec + begin->tv_usec * 1.0 / 1000000);
}

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("usage: ./mq <size> <count>\n");
    return 1;
  }

  const char *path = "/mq_test";
  int size = atoi(argv[1]);
  int count = atoi(argv[2]);
  char *buf = malloc(size);
  int sum = 0;
  ssize_t n = 0;
  struct timeval begin, end;
  struct mq_attr attr;

  mqd_t mq = mq_open(path, O_CREAT | O_RDWR, 0777, NULL);
  if (mq == -1) {
    perror("mq_open");
    return 1;
  }

  if (mq_getattr(mq, &attr) == -1) {
    perror("mq_getattr");
    return 1;
  }

  attr.mq_maxmsg = 10000;
  if (mq_setattr(mq, &attr, NULL) == -1) {
    perror("mq_setattr");
    return 1;
  }

  long msg_size = attr.mq_msgsize;
  for (int i = 0; i < attr.mq_curmsgs; i++) {
    if (mq_receive(mq, buf, msg_size, 0) == -1) {
      perror("mq_receive");
      return 1;
    }
  }

  if (fork() == 0) {
    for (int i = 0; i < count; i++) {
      if ((n = mq_receive(mq, buf, msg_size, 0)) == -1) {
        perror("mq_receive");
        return 1;
      }
      sum += n;
    }

    if (sum != count * size) {
      fprintf(stderr, "sum error: %d != %d\n", sum, count * size);
      return 1;
    }

    mq_close(mq);
  } else {
    sleep(1);

    gettimeofday(&begin, NULL);
    for (int i = 0; i < count; i++) {
      if (mq_send(mq, buf, size, 0) == -1) {
        perror("mq_send");
        return 1;
      }
    }
    gettimeofday(&end, NULL);

    double tm = getdetlatimeofday(&begin, &end);
    printf("%.0fMB/s %.0fmsg/s\n",
           count * size * 1.0 / (tm * 1024 * 1024),
           count * 1.0 / tm);

    mq_close(mq);
    mq_unlink(path);
  }

  return 0;
}