#include "sd_common.h"

slice *init_slice() {
  slice *p = (slice *) malloc (sizeof (slice));
  if (p == NULL) {
    return NULL;
  }
  p->str = NULL;
  p->len = 0;
  p->capacity = 0;
  return p;
}

void free_slice(slice *p) {
  free (p->str);
  free (p);
}

static int resize_slice (slice *p, int len) {
  int capacity = p->capacity;
  while (capacity < p->len + len) {
    capacity += 1024;
  }
  char *tmp = (char *) realloc (p->str, capacity * sizeof(char));
  if (tmp == NULL) {
    return -1;
  }
  p->capacity = capacity;
  p->str = tmp;
  return 0;
}

int append_slice(slice *p, char *str, int len) {
  if (p->len + len > p->capacity) {
    if (resize_slice (p, len) < 0)
      return -1;
  }
  strncpy (p->str + p->len, str, len);
  p->str[p->len + len] = 0;
  p->len += len;
  return 0;
}

