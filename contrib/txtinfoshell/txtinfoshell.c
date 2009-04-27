/*
 * txtinfoshell.c
 *
 *  Created on: 27.04.2009
 *      Author: rogge
 */

/*
 ** client.c -- a stream socket client demo
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define DEFAULT_TXTINFO_PORT 2006
#define BLOCKSIZE 2048

int running;

/*
 * signalHandler
 *
 * This handler stops the mainloop when the programm is send a signal
 * to stop
 */
static void
signalHandler(int signo __attribute__ ((unused)))
{
  running = 0;
}

int
main(int argc, char *argv[]) {
  int sockfd;
  char buf[BLOCKSIZE];
  struct sockaddr_in localSocket;
  fd_set readSet;
  int port = DEFAULT_TXTINFO_PORT;

  /* parse parameter */
  switch (argc) {
    case 1:
      break;
    case 2:
      if ((strcmp(argv[1], "-h") != 0) && (strcmp(argv[1], "--help") != 0)) {
        port = atoi(argv[1]);
        break;
      }
    default:
      fprintf(stderr, "Usage: txtinfo-sh [port]\n");
      return 0;
  }

  memset(&localSocket, 0, sizeof(localSocket));
  localSocket.sin_family = AF_INET;
  localSocket.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  localSocket.sin_port = htons(port);

  if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Cannot open socket to connect to olsrd");
    return 0;
  }

  if (connect(sockfd, (struct sockaddr *)&localSocket, sizeof(localSocket)) == -1) {
    close(sockfd);
    perror("Cannot connect to olsrd");
    return 0;
  }

  running = 1;

  /* activate signal handling */
  signal(SIGABRT, &signalHandler);
  signal(SIGTERM, &signalHandler);
  signal(SIGQUIT, &signalHandler);
  signal(SIGINT, &signalHandler);

  while (running) {
    int result;
    struct timeval tv;

    FD_ZERO(&readSet);

    FD_SET(STDIN_FILENO, &readSet);
    FD_SET(sockfd, &readSet);

    tv.tv_sec = 1;
    tv.tv_usec = 0;

    result = select(sockfd+1, &readSet, NULL, NULL, &tv);
    if (result < 0) {
      perror("select");
      break;
    }

    if (result > 0) {
      if (FD_ISSET(STDIN_FILENO, &readSet)) {
        /* stdin/out need read/write */
        ssize_t len = 0;
        ssize_t s = 0;

        len = read(STDIN_FILENO, buf, sizeof(buf));

        while (s < len) {
          /* sockets use recv/send */
          ssize_t w = send(sockfd, &buf[s], len - s, 0);
          if (w <= 0) {
            running = 0;
            perror("socket-send");
            break;
          }
          s += w;
        }
      }
      if (FD_ISSET(sockfd, &readSet)) {
        /* sockets use recv/send */
        ssize_t len = 0;
        ssize_t s = 0;

        len = recv(sockfd, buf, sizeof(buf), 0);
        if (len == 0) {
          // socket closed
          running = 0;
          break;
        }

        while (s < len) {
          /* stdin/out need read/write */
          ssize_t w = write(STDOUT_FILENO, &buf[s], len - s);
          if (w <= 0) {
            running = 0;
            perror("stdout-send");
            break;
          }
          s += w;
        }
      }
    }
  }
  close(sockfd);
  return 0;
}
