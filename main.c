#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#define PORT 8888
char *readLine();
char *readName();
int copy(char *dest, char *src, int offset, int len);
void *sendMsg(void *arg);
void *recvMsg(void *arg);
int openSocket, connDesc;
char buf[2048];
char connName[2048], selfName[2048];
struct sockaddr_in sockInfo;
int sockInfoSize = sizeof(sockInfo);
pthread_t sendThread, recvThread;
bool nameReceived = false;
void main(int argc, char const *argv[]) {
  openSocket = socket(AF_INET, SOCK_STREAM, 0);
  memset(buf, '\0', 2048);
  if (openSocket < 0) {
    printf("Error creating a socket.");
    exit(1);
  }
  int ch;
  char *cch, *name;
  printf("Enter your choice\n1)Host\n2)Connect\nChoice:");
  cch = readLine();
  ch = atoi(cch);
  free(cch);
  fflush(stdin);
  printf("Enter your name:");
  name = readName();
  fflush(stdin);
  copy(selfName, name, 0, strlen(name));
  if (ch == 1) {
    sockInfo.sin_family = AF_INET;
    sockInfo.sin_port = htons(PORT);
    sockInfo.sin_addr.s_addr = inet_addr("0.0.0.0");
    if (bind(openSocket, (struct sockaddr *)&sockInfo, sockInfoSize) < 0) {
      printf("Error binding to port %d", PORT);
      exit(1);
    }
    if (listen(openSocket, 0) == -1) {
      printf("Error while starting to listen for requests\n");
      close(openSocket);
      exit(2);
    }
    printf("Listening on port-%d\n", PORT);
    connDesc = accept(openSocket, (struct sockaddr *)&sockInfo, &sockInfoSize);
    if (connDesc == -1) {
      printf("Error connecting to the client");
      exit(3);
    }
    printf("Conncted to the client\n");
  } else if (ch == 2) {
    int port;
    char *addr;
    printf("Enter the address of host:");
    addr = readLine();
    if (strcmp(addr, "localhost") == 0) {
      strcpy(addr, "127.0.0.1");
    }
    printf("Enter port number:");
    scanf("%d", &port);
    sockInfo.sin_family = AF_INET;
    sockInfo.sin_port = htons(port);
    sockInfo.sin_addr.s_addr = inet_addr(addr);
    free(addr);
    if (connect(openSocket, (struct sockaddr *)&sockInfo, sockInfoSize) == -1) {
      printf("Error connecting with host\n");
      close(openSocket);
      exit(3);
    }
    printf("Connected to the host\n");
    connDesc = openSocket;
  } else {
    printf("Incorrect choice. Enter 1 or 2 only\n");
    exit(-1);
  }
  pthread_create(&sendThread, NULL, sendMsg, NULL);
  pthread_create(&recvThread, NULL, recvMsg, NULL);
  pthread_join(sendThread, NULL);
  pthread_join(recvThread, NULL);
}
char *readLine() {
  int SIZE = 32, i = 0;
  char *ptr = malloc(sizeof(char) * SIZE);
  char c;
  while ((c = getc(stdin)) != '\n') {
    ptr[i++] = c;
    if (i == SIZE) {
      SIZE = (SIZE << 1);
      ptr = realloc(ptr, sizeof(char) * SIZE);
    }
  }
  ptr[i] = '\0';
  fflush(stdin);
  return ptr;
}
char *readName() {
  int SIZE = 32;
  char *ptr = malloc(sizeof(char) * SIZE);
  char c;
  int i = 0, l = 0;
  fflush(stdin);
  do {
    memset(ptr, SIZE, '\0');
    i = 0;
    while ((c = getc(stdin)) != '\n') {
      ptr[i++] = c;
      if (i == SIZE) {
        SIZE = (SIZE << 1);
        ptr = realloc(ptr, sizeof(char) * SIZE);
      }
    }
    l = strlen(ptr);
    if (l >= 2048) {
      printf("The name cannot be greater than 2KBs\n");
      printf("Enter your name in shorter form:");
    }
  } while (l >= 2048);
  ptr[i] = '\0';
  fflush(stdin);
  return ptr;
}
int copy(char *dest, char *src, int offset, int len) {
  for (int i = 0; i < len; i++) {
    dest[i] = src[offset + i];
    if (dest[i] == '\0') return i + 1;
  }
  return len;
}
void *sendMsg(void *arg) {
  send(connDesc, selfName, strlen(selfName), 0);
  while (1) {
    printf("%s::", selfName);
    char *msg = readLine();
    int len = strlen(msg);
    for (int i = 0; i < len; i += 2048) {
      int l = copy(buf, msg, i, 2048);
      if (send(connDesc, buf, l, 0) == -1) {
        printf("Error while sending the data\n");
        break;
      }
    }
    if (strcmp(msg, "/exit") == 0) {
      free(msg);
      close(connDesc);
      if (openSocket) close(openSocket);
      printf("Connection Closed\n");
      pthread_cancel(recvThread);
      return NULL;
    }
    free(msg);
  }
  return NULL;
}
void *recvMsg(void *arg) {
  while (1) {
    int len;
    while ((len = recv(connDesc, buf, 2048, 0)) == 2048) {
      printf("%s", buf);
    }
    if (len == -1) {
      printf("Error receiving a msg\n");
    } else if (len == 0) {
      close(connDesc);
      if (openSocket) close(openSocket);
      printf("Connection Closed\n");
      pthread_cancel(sendThread);
      return NULL;
    } else {
      if (nameReceived) {
        printf("\r%s::%s\n", connName, buf);
        printf("%s::", selfName);
      } else {
        copy(connName, buf, 0, len);
        nameReceived = true;
      }
    }
  }
  return NULL;
}