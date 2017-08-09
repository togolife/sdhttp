#include "sd_common.h"

event_loop *mainloop = NULL;

int sock_read_handler (int fd) {
  printf ("in sock_read_handler fd:%d\n", fd);
  if (mainloop->reg_fds[fd].read_mes == NULL) {
    mainloop->reg_fds[fd].read_mes = init_slice();
    if (mainloop->reg_fds[fd].read_mes == NULL) {
      printf ("sock_read_handler init slice failed!\n");
      return -1;
    }
  }
  slice *read_mes = mainloop->reg_fds[fd].read_mes;
  int ret = http_read_request(fd, read_mes);
  if (mainloop->reg_fds[fd].addition == NULL) {
    mainloop->reg_fds[fd].addition = init_request_head();
    if (mainloop->reg_fds[fd].addition == NULL) {
      printf ("sock_read_handler malloc failed!\n");
      return -1;
    }
  }
  printf ("read message: %s\n", read_mes->str);
  request_head *req_head = mainloop->reg_fds[fd].addition;
  int ret2 = http_parse (read_mes, req_head);
  if (ret2 == 1) {
    if (mainloop->reg_fds[fd].writ_mes == NULL) {
      mainloop->reg_fds[fd].writ_mes = init_slice();
      if (mainloop->reg_fds[fd].writ_mes == NULL) {
        printf ("sock_read_handler init slice failed!\n");
        return -1;
      }
    }
    slice *writ_mes = mainloop->reg_fds[fd].writ_mes;
    http_reply (req_head, NULL, writ_mes);
    free_request_head (req_head);
    mainloop->reg_fds[fd].addition = NULL;
    free_slice (mainloop->reg_fds[fd].read_mes);
    mainloop->reg_fds[fd].read_mes = NULL;
  }
  if (ret == 1) {
    del_fdevent(mainloop, fd, READ_EVENT);
    if (ret2 != 1) {
      del_fdevent(mainloop, fd, WRIT_EVENT);
      close (fd);
    }
  }
  return 0;
}

int sock_write_handler (int fd, slice *data) {
  printf ("in sock_write_handler fd:%d\n", fd);
  int left = data->len;
  int pos = 0;
  while (left > 0) {
    int ret = write (fd, data->str + pos, left);
    if (ret == 0) {
      break;
    }
    if (ret < 0) {
      //
    }
    left -= ret;
    pos += ret;
  }
  printf ("write message: %s\n", data->str);
  if ((mainloop->reg_fds[fd].mask & READ_EVENT) == 0) {
    del_fdevent(mainloop, fd, WRIT_EVENT);
    close (fd);
  }
  return 0;
}

int accept_handler (int fd) {
  //printf ("in accept_handler\n");
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
