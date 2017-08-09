#ifndef SDHTTP_STRING_H
#define SDHTTP_STRING_H

typedef struct slice slice;
struct slice {
  char *str;
  int len;
  int capacity;
};

slice *init_slice();
void free_slice(slice *p);
int append_slice(slice *p, char *str, int len);


#endif