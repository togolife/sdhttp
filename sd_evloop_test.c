#include "sd_common.h"

event_loop *mainloop = NULL;

void set_write_data (int fd, char *data, int len) {
  char *p = (char *) malloc (sizeof (char) * (len+1));
  if (p == NULL) {
    printf ("set write data failed!\n");
    return;
  }
  memcpy (p, data, (len + 1) * sizeof(char));
  free (mainloop->reg_fds[fd].message);
  mainloop->reg_fds[fd].message = p;
  return;
}

int sock_read_handler (int fd) {
  printf ("in sock_read_handler fd:%d\n", fd);
  char line[1024];
  int readlen = 0;
  while (1) {
    int ret = read (fd, line, 1023);
    if (ret < 0) {
      if (errno != EWOULDBLOCK) {
        printf ("read client failed!\n");
      }
      break;
    }
    if (ret == 0) {
      if (readlen == 0) {
        del_fdevent(mainloop, fd, READ_EVENT);
        del_fdevent(mainloop, fd, WRIT_EVENT);
        close (fd);
        return 1;
      } else {
        del_fdevent(mainloop, fd, READ_EVENT);
      }
    }
    readlen += ret;
    line[ret] = 0;
    printf ("%s\n", line);
  }
  set_write_data(fd,line,readlen);
  return 0;
}

int sock_write_handler (int fd, char *data) {
  printf ("in sock_write_handler fd:%d\n", fd);
  int left = strlen(data);
  int pos = 0;
  while (left > 0) {
    int ret = write (fd, data + pos, left);
    if (ret == 0) {
      break;
    }
    if (ret < 0) {
      //
    }
    left -= ret;
    pos += ret;
  }
  if ((mainloop->reg_fds[fd].mask & READ_EVENT) == 0) {
    del_fdevent(mainloop, fd, WRIT_EVENT);
    close (fd);
  }
  return 0;
}

int accept_handler (int fd) {
  printf ("in accept_handler\n");
  while (1) {
    struct sockaddr_in cliaddr;
    int len = sizeof (cliaddr);
    int ret = accept (fd, (struct sockaddr *)&cliaddr, &len);
    if (ret < 0) {
      if (errno == EWOULDBLOCK || errno == EPROTO || errno == ECONNABORTED || errno == EINTR)
        break;
      else {
        perror ("accept failed!\n");
        return -1;
      }
    }
    printf ("receive a client: %s\n", inet_ntoa(cliaddr.sin_addr));
    setnonblock(ret, 1);
    add_fdevent (mainloop, ret, READ_EVENT|WRIT_EVENT, sock_read_handler, sock_write_handler, NULL);
  }
  return 0;
}

int main (void) {
  int sock = CreateNonBlockSocket();
  if (sock < 0) {
    printf ("CreateNonBlockSocket failed!\n");
    return -1;
  }
  if (BindAndListen (sock, 9876) != 0) {
    printf ("BindAndListen failed!\n");
    return -1;
  }
  event_loop *evloop = init_loop(1024);
  if (evloop == NULL) {
    printf ("create event loop failed!\n");
    return -1;
  }
  mainloop = evloop;
  add_fdevent(evloop, sock, READ_EVENT, accept_handler, NULL, NULL);
  loop_run(evloop);
  return 0;
}
