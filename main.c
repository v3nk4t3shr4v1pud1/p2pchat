#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<unistd.h>
#define PORT 8888
char* readLine();
int copy(char *dest,char *src,int offset,int len);
void* sendMsg(void* arg);
void* recvMsg(void* arg);
int openSocket,connDesc;
char buf[2048];
struct sockaddr_in sockInfo;
int sockInfoSize=sizeof(sockInfo);
pthread_t sendThread,recvThread;
void main(int argc, char const *argv[]){
    openSocket=socket(AF_INET,SOCK_STREAM,0);
    memset(buf,'\0',2048);
    if(openSocket<0){
        printf("Error creating a socket.");
        exit(1);
    }
    int ch;
    char *cch;
    printf("Enter your choice\n1)Host\n2)Connect\nChoice:");
    cch=readLine();
    ch=atoi(cch);
    free(cch);
    fflush(stdin);
    if(ch==1){
        sockInfo.sin_family=AF_INET;
        sockInfo.sin_port=htons(PORT);
        sockInfo.sin_addr.s_addr=inet_addr("0.0.0.0");
        if(bind(openSocket,(struct sockaddr*)&sockInfo,sockInfoSize)<0){
            printf("Error binding to port %d",PORT);
            exit(1);
        }
        if(listen(openSocket,0)==-1){
            printf("Error while starting to listen for requests\n");
            close(openSocket);
            exit(2);
        }
        printf("Listening on port-%d\n",PORT);
        connDesc=accept(openSocket,(struct sockaddr*)&sockInfo,&sockInfoSize);
        if(connDesc==-1){
            printf("Error connecting to the client");
            exit(3);
        }
        printf("Conncted to the client\n");
    }else if(ch==2){
        int port;
        char *addr;
        printf("Enter the address of host:");
        addr=readLine();
        printf("Enter port number:");
        scanf("%d",&port);
        sockInfo.sin_family=AF_INET;
        sockInfo.sin_port=htons(port);
        sockInfo.sin_addr.s_addr=inet_addr(addr);
        free(addr);
        if(connect(openSocket,(struct sockaddr*)&sockInfo,sockInfoSize)==-1){
            printf("Error connecting with host\n");
            close(openSocket);
            exit(3);
        }
        printf("Connected to the host\n");
        connDesc=openSocket;
    }else{
        printf("Incorrect choice. Enter 1 or 2 only\n");
        exit(-1);
    }
    pthread_create(&sendThread,NULL,sendMsg,NULL);
    pthread_create(&recvThread,NULL,recvMsg,NULL);
    pthread_join(sendThread,NULL);
    pthread_join(recvThread,NULL);
}
char* readLine(){
    int SIZE=32,i=0;
    char *ptr=malloc(sizeof(char)*SIZE);
    char c;
    while((c=getc(stdin))!='\n'){
        ptr[i++]=c;
        if(i==SIZE){
            SIZE=(SIZE<<1);
            ptr=realloc(ptr,sizeof(char)*SIZE);
        }
    }
    ptr[i]='\0';
    fflush(stdin);
    return ptr;
}
int copy(char *dest,char *src,int offset,int len){
    for(int i=0;i<len;i++){
        dest[i]=src[offset+i];
        if(dest[i]=='\0')return i+1;
    }
    return len;
}
void* sendMsg(void *arg){
    while(1){
        char *msg=readLine();
        int len=strlen(msg);
        for(int i=0;i<len;i+=2048){
            int l=copy(buf,msg,i,2048);
            if(send(connDesc,buf,l,0)==-1){
                printf("Error while sending the data\n");
                break;
            }
        }
        if(strcmp(msg,"/exit")==0){
            free(msg);
            close(connDesc);
            if(openSocket)close(openSocket);
            printf("Connection Closed\n");
            pthread_cancel(recvThread);
            return NULL;
        }
        free(msg);
    }
    return NULL;
}
void* recvMsg(void *arg){
    while(1){
        int len;
        while((len=recv(connDesc,buf,2048,0))==2048){
            printf("%s",buf);
        }
        if(len==-1){
            printf("Error receiving a msg\n");
        }else if(len==0){
            close(connDesc);
            if(openSocket)close(openSocket);
            printf("Connection Closed\n");
            pthread_cancel(sendThread);
            return NULL;
        }else{
            printf("%s\n",buf);
        }
    }
    return NULL;
}