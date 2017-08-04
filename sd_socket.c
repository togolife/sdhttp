#include "sd_common.h"

int CreateSocket(void) {
  int fd = socket (AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror ("create socket failed!");
  }
  return fd;
}

int setnonblock(int fd, int type) {
  int flags = fcntl(fd,F_GETFL,0);
  if (flags < 0) {
    perror ("get socket flags failed!");
    return -1;
  }
  if (type == 1) {
    flags |= O_NONBLOCK;
  } else {
    flags &= ~O_NONBLOCK;
  }
  if (fcntl(fd,F_SETFL, flags) < 0) {
    printf ("set socket O_NONBLOCK failed!");
    return -1;
  }
  return 0;
}

int CreateNonBlockSocket(void) {
  int fd = socket (AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror ("create socket failed!");
  } else {
    if (setnonblock(fd, 1) == -1) {
      close(fd);
      fd = -1;
    }
  }
  return fd;
}

int BindAndListen(int sock, int port) {
  int keepAlive = 1;
  setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepAlive, sizeof(keepAlive));
  int reuseaddr = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void*)&reuseaddr, sizeof(reuseaddr));
  struct sockaddr_in serveraddr;
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_port = htons(port);
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind (sock, (struct sockaddr *)&serveraddr, sizeof (serveraddr)) < 0) {
    perror ("bind socket failed!");
    return -1;
  }
  if (listen (sock, 1024) < 0) {
    perror ("listen socket failed!");
    return -1;
  }
  return 0;
}

