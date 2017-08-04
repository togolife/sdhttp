struct select_data {
  int maxfd;
  fd_set rset,wset,eset;
  fd_set _rset,_wset,_eset;
};

static int select_init (event_loop *evloop) {
  evloop->addition = malloc (sizeof (struct select_data));
  if (evloop->addition == NULL) {
    return -1;
  }
  struct select_data *p = (struct select_data *) evloop->addition;
  p->maxfd = 0;
  FD_ZERO(&(p->rset));
  FD_ZERO(&(p->wset));
  FD_ZERO(&(p->eset));
  return 0;
}

static void select_free (event_loop *evloop) {
  free (evloop->addition);
}

static int select_add (event_loop *evloop, int fd, int event) {
  struct select_data *p = (struct select_data *) evloop->addition;
  if (event & READ_EVENT) {
    FD_SET(fd, &(p->rset));
  }
  if (event & WRIT_EVENT) {
    FD_SET(fd, &(p->wset));
  }
  if (event & EXCP_EVENT) {
    FD_SET(fd, &(p->eset));
  }
  if (fd > p->maxfd)
    p->maxfd = fd;
  return 0;
}

static int select_del (event_loop *evloop, int fd, int event, int del_flg) {
  struct select_data *p = (struct select_data *) evloop->addition;
  if (event & READ_EVENT) {
    FD_CLR(fd, &(p->rset));
  }
  if (event & WRIT_EVENT) {
    FD_CLR(fd, &(p->wset));
  }
  if (event & EXCP_EVENT) {
    FD_CLR(fd, &(p->eset));
  }
  if (del_flg == 1 && p->maxfd == fd) {
    int i = p->maxfd;
    while (i >= 0) {
      if (evloop->reg_fds[i].mask != 0) {
        p->maxfd = i;
        break;
      }
      --i;
    }
    if (i < 0) {
      p->maxfd = 0;
    }
  }
  return 0;
}

static int select_pool (event_loop *evloop) {
  struct select_data *p = (struct select_data *) evloop->addition;
  memcpy (&(p->_rset), &(p->rset), sizeof(p->rset));
  memcpy (&(p->_wset), &(p->wset), sizeof(p->wset));
  memcpy (&(p->_eset), &(p->eset), sizeof(p->eset));
  struct timeval ts = {2,0};
  int ret = select (p->maxfd + 1, &(p->_rset), &(p->_wset), &(p->_eset), &ts);
  if (ret < 0 && errno != EINTR) {
    perror ("select failed!");
    return -1;
  }
  //printf ("select return %d maxfd %d\n", ret, p->maxfd);
  int num = 0;
  if (ret > 0) {
    int i = 0;
    for (;i < p->maxfd + 1; ++i) {
      int mask = 0;
      if (FD_ISSET(i, &(p->_rset))) {
        mask |= READ_EVENT;
      }
      if (FD_ISSET(i, &(p->_wset))) {
        mask |= WRIT_EVENT;
      }
      if (FD_ISSET(i, &(p->_eset))) {
        mask |= EXCP_EVENT;
      }
      if (mask != 0) {
        ++num;
        evloop->happends[i] = mask;
      }
    }
  }
  //printf ("num return %d \n", num);
  return num;
}

struct partial partial = {
  select_init,
  select_add,
  select_del,
  select_pool,
  select_free
};

