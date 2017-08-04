#include "sd_common.h"

#define USE_SELECT

#if defined(USE_SELECT)
#include "sd_select.c"
#elif defined(USE_POOL)
#include "sd_pool.c"
#endif

event_loop *init_loop(int size) {
  event_loop *ev = (event_loop *) malloc (sizeof (event_loop));
  if (ev == NULL)
    return;
  ev->capacity = size;
  ev->add_size = 100;
  ev->reg_fds = (fd_event *) calloc (ev->capacity, sizeof (fd_event));
  if (ev->reg_fds == NULL)
    goto err1;
  ev->happends = (int *) calloc (ev->capacity, sizeof (int));
  if (ev->happends == NULL)
    goto err2;
  if (partial.init (ev) != 0)
    goto err2;
  return ev;
err2:
  if (ev->happends) {
    free (ev->happends);
  }
err1:
  if (ev->reg_fds) {
    free (ev->reg_fds);
  }
  free (ev);
  return NULL;
}

void free_loop(event_loop *evloop) {
  partial.free (evloop);
  free (evloop->happends);
  free (evloop->reg_fds);
  free (evloop);
}

void resize_loop(event_loop *evloop) {
  //ev->reg_fds = (fd_event *) realloc (ev->capacity, sizeof (fd_event));
  return;
}

int add_fdevent(event_loop *evloop, int fd, int mask, READ_FUNC read_func,
  WRITE_FUNC write_func, EXCEP_FUNC excep_func) {
  //if (fd > evloop->
  if (evloop->reg_fds[fd].mask != 0) {
    printf ("add fd event failed!\n");
    return -1;
  }
  fd_event *p = evloop->reg_fds + fd;
  p->mask = mask;
  p->read_func = read_func;
  p->write_func = write_func;
  p->excep_func = excep_func;
  p->message = NULL;
  partial.add(evloop, fd, mask);
  return 0;
}

void del_fdevent(event_loop *evloop, int fd, int event) {
  int del = 0;
  evloop->reg_fds[fd].mask &= ~event;
  if (evloop->reg_fds[fd].mask == 0) {
    free (evloop->reg_fds[fd].message);
    del = 1;
  }
  partial.del(evloop, fd, event, del);
}

void loop_run(event_loop *evloop) {
  while (1) {
    //printf ("loop run here\n");
    int ret = partial.pol(evloop);
    if (ret > 0) {
      deal_event (evloop);
    }
  }
}

void deal_event(event_loop *evloop) {
  int i = 0;
  for (; i < evloop->capacity; ++i) {
    if (evloop->happends[i] != 0) {
      int event = evloop->happends[i];
      if (event & READ_EVENT) {
        evloop->reg_fds[i].read_func(i);
      }
      if ((event & WRIT_EVENT) && evloop->reg_fds[i].message != NULL) {
        evloop->reg_fds[i].write_func(i,evloop->reg_fds[i].message);
        free (evloop->reg_fds[i].message);
        evloop->reg_fds[i].message = NULL;
      }
      if (event & EXCP_EVENT) {
        evloop->reg_fds[i].excep_func(i);
      }
      evloop->happends[i] = 0;
    }
  }
}
