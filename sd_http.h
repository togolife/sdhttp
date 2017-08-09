#ifndef SDHTTP_HTTP_H
#define SDHTTP_HTTP_H

typedef struct request_head request_head;
struct request_head {
  int req_func; // 请求方法 0-Get 1-Post
  char *req_uri;// 
  int keep_alive;
  char *req_body;// 请求体
};

typedef int (*HANDLER) (char *req, slice *writ_mes);
typedef struct func_handler func_handler;
struct func_handler {
  char name[100];
  HANDLER handle;
  struct func_handler *next;
};

request_head *init_request_head();
void free_request_head(request_head *p);
func_handler *init_func_handler();
void free_func_handler(func_handler *p);
int reg_handler (func_handler *p, char *name, HANDLER handle);
func_handler *find_func_handler(func_handler *p, char *name);

int http_read_request (int fd, slice *read_mes);
int http_parse (slice *mes, request_head *req);
int http_reply (request_head *req, func_handler *head, slice *writ_mes);

#endif