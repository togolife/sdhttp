#ifndef SDHTTP_SOCKET_H
#define SDHTTP_SOCKET_H

int setnonblock(int fd, int type);
int CreateSocket(void);
int CreateNonBlockSocket(void);
int BindAndListen(int sock, int port);

#endif