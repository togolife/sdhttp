#include "sd_common.h"

request_head *init_request_head() {
  request_head *p = (request_head *) malloc (sizeof (request_head));
  if (p == NULL) {
    return NULL;
  }
  p->req_func = 0;
  p->keep_alive = 0;
  p->req_uri = p->req_body = NULL;
  return p;
}

void free_request_head(request_head *p) {
  free (p->req_uri);
  free (p->req_body);
  free (p);
}

func_handler *init_func_handler() {
  func_handler *p = (func_handler *) malloc (sizeof (func_handler));
  if (p == NULL) {
    return p;
  }
  strcpy (p->name, "HEADNODE");
  p->handle = NULL;
  p->next = NULL;
  return p;
}

void free_func_handler(func_handler *p) {
  while (p) {
    func_handler *q = p->next;
    free (p);
    p = q;
  }
}

int reg_handler (func_handler *p, char *name, HANDLER handle) {
  func_handler *q = find_func_handler (p, name);
  if (q == NULL) {
    q = (func_handler *) malloc (sizeof (func_handler));
    if (q == NULL) {
      return -1;
    }
    strcpy (q->name,name);
    q->handle = handle;
    q->next = p->next;
    p->next = q;
  } else {
    q->handle = handle;
  }
  return 0;
}

func_handler *find_func_handler(func_handler *p, char *name) {
  while (p && strcmp (p->name, name) != 0) {
    p = p->next;
  }
  return p;
}

int http_read_request (int fd, slice *read_mes) {
  char line[1024];
  int ret = 0, readlen = 0;
  while (1) {
    ret = read (fd, line, 1024);
    if (ret < 0) {
      if (errno != EWOULDBLOCK && errno != EINTR) {
        printf ("read http request failed!\n");
        return -1; // 发生错误
      }
      break;
    }
    if (ret == 0) {
      return 1; // 表示client关闭
    }
    append_slice (read_mes, line, ret);
  }
  return 0; // 本次读取结束
}

// http解析请求
/* http请求格式
请求行 空格分隔   请求方法 请求URL http协议与版本
请求头 冒号  参数:值
请求体 等号  参数=值&参数=值
*/
int http_parse (slice *mes, request_head *req) {
  if (mes == NULL || mes->str == NULL) {
    return 0;
  }
  char *end = strstr (mes->str, "\r\n\r\n");
  if (end == NULL) {
    return 0;
  }
  char *cnt_l = strstr (mes->str, "Content-Length:");
  if (cnt_l != NULL) {
    cnt_l += 15;
    int len = 0;
    while (*cnt_l != '\r' && *cnt_l != '\n') {
      if (*cnt_l >= '0' && *cnt_l <= '9')
        len = len * 10 + *cnt_l - '0';
      ++cnt_l;
    }
    if (strlen (end) - 4 != len) {
      return 0;
    }
    req->req_body = (char *) malloc (sizeof (char) * (len + 1));
    if (req->req_body == NULL) {
      printf ("http parse failed! save request body failed!\n");
      return -1;
    }
    strcpy (req->req_body, end+4);
  }
  char *p = mes->str;
  int len = 0;
  // http请求方式
  if (strncmp (p, "GET ", 4) == 0) {
    req->req_func = 0;
    len += 4;
  } else if (strncmp (p, "POST ", 5) == 0) {
    req->req_func = 1;
    len += 5;
  } else {
    req->req_func = 0; // Default 0
    while (p[len] != ' ') ++len;
    ++len;
  }
  p += len;
  len = 0;
  while (p[len] != ' ') ++len;
  req->req_uri = (char *) malloc (sizeof (char) * len);
  if (req->req_uri == NULL) {
    printf ("http parse failed!\n");
    return -1;
  }
  strncpy (req->req_uri, p, len);
  req->req_uri[len] = 0;
  // 判断keep alive  这个再优化
  req->keep_alive = 0;
  return 1;
}

static int http_reply_head_line (slice *writ_mes, int status) {
  append_slice (writ_mes, "HTTP/1.1 ", 9);
  switch (status) {
    case 200:
      append_slice (writ_mes, "200 OK", 6);
      break;
    case 404:
      append_slice (writ_mes, "404 Not Found", 13);
      break;
    default:
      append_slice (writ_mes, "500 Internal Server Error", 25);
      break;
  }
  append_slice (writ_mes, "\r\n", 2);
  return 0;
}

static int http_reply_head_s (slice *writ_mes, char *name, char *value) {
  append_slice (writ_mes, name, strlen(name));
  append_slice (writ_mes, ":", 1);
  append_slice (writ_mes, value, strlen(value));
  append_slice (writ_mes, "\r\n", 2);
  return 0;
}

static int http_reply_head_i (slice *writ_mes, char *name, int value) {
  append_slice (writ_mes, name, strlen(name));
  append_slice (writ_mes, ":", 1);
  char tmp[100];
  sprintf (tmp, "%d", value);
  append_slice (writ_mes, tmp, strlen(tmp));
  append_slice (writ_mes, "\r\n", 2);
  return 0;
}

// http 应答
/* 约定处理规则
Get请求方式：用来获取路径文件，如果有参数则不处理
Post请求方式：用来处理参数情况
*/
int http_reply (request_head *req, func_handler *head, slice *writ_mes) {
  char *root = "~/tmp/sdlib/"; // 文件主路径  这个再优化，可写成读取配置文件或其他形式
  if (req->req_func == 0) {
    slice *get_uri = init_slice();
    int status = 200;
    do {
      if (get_uri == NULL) {
        status = 500;
        break;
      }
      append_slice (get_uri, root, strlen(root));
      if (strcmp (req->req_uri, "/") == 0) {
        append_slice (get_uri, "index.html", 10);
      } else {
        append_slice (get_uri, req->req_uri, strlen(req->req_uri));
      }
      int fd = open (get_uri->str, O_RDONLY);
      if (fd < 0) {
        printf ("get file %s not exists!\n", get_uri->str);
        status = 404;
      } else {
        get_uri->len = 0;
        char line[1024];
        char *head = "<html><head><title>Http学习</title></head><body><pre>";
        append_slice (get_uri, head, strlen(head));
        while (1) {
          int ret = read (fd, line, 1024);
          if (ret == 0)
            break;
          append_slice (get_uri, line, ret);
        }
        append_slice (get_uri, "</pre></body></html>", 20);
        close (fd);
      }
    } while (0);
    if (status == 200) {
      http_reply_head_line (writ_mes, 200);
      char tmp[100];
      snprintf (tmp, 100, "Content-Length: %d", get_uri->len);
      append_slice (writ_mes, tmp, strlen(tmp));
      append_slice (writ_mes, "\r\n", 2);
      append_slice (writ_mes, "\r\n", 2);
      append_slice (writ_mes, get_uri->str, get_uri->len);
    } else {
      http_reply_head_line (writ_mes, status);
      char tmp[100];
      if (status == 404) {
        snprintf (tmp, 100, "404 Not Found the file!");
      } else if (status == 500) {
        snprintf (tmp, 100, "500 Internal Deal Failed!");
      }
      http_reply_head_i (writ_mes, "Content-Length", strlen(tmp));
      append_slice (writ_mes, "\r\n", 2);
      append_slice (writ_mes, tmp, strlen(tmp));
    }
    free_slice (get_uri);
  } else { // Post方式
    //
    
  }
  return 0;
}



