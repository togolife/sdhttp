#ifndef SDHTTP_EVLOOP_H
#define SDHTTP_EVLOOP_H

#define NONE_EVENT 0x00
#define READ_EVENT 0x01
#define WRIT_EVENT 0x02
#define EXCP_EVENT 0x04

typedef int (*READ_FUNC) (int fd);
typedef int (*WRITE_FUNC) (int fd, char *mes);
typedef int (*EXCEP_FUNC) (int fd);

typedef struct fd_event fd_event;
struct fd_event {
  int mask;
  READ_FUNC read_func;
  WRITE_FUNC write_func;
  EXCEP_FUNC excep_func;
  char *message;
};

typedef struct event_loop event_loop;
struct event_loop {
  fd_event *reg_fds; // 序号作为fd
  int capacity; // reg_fds容量
  int size;     // 当前数量
  int add_size; // 累加数量
  int *happends;
  void *addition; // select/pool 等特殊化使用
};

struct partial {
  int (*init)(event_loop *evloop); // 初始化
  int (*add) (event_loop *evloop, int fd, int event); // 添加事件
  int (*del) (event_loop *evloop, int fd, int event, int flag); // 删除事件
  int (*pol) (event_loop *evloop); // 等待事件
  void (*free) (event_loop *evloop);// 释放
};

event_loop *init_loop(int size);
void free_loop(event_loop *evloop);
void resize_loop(event_loop *evloop);
int add_fdevent(event_loop *evloop, int fd, int mask, int (*read_func) (int fd),
  int (*write_func) (int fd, char *mes),
  int (*exception_func) (int fd));
void del_fdevent(event_loop *evloop, int fd, int event);
void loop_run(event_loop *evloop);
void deal_event(event_loop *evloop);

#endif