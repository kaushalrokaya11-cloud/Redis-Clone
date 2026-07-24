#include<stdio.h>
#include<sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main(){
    int fd = socket(AF_INET,SOCK_STREAM,0);
    if(fd<0){
        perror("socket()");
    }
    struct sockaddr_in addr = {};
    addr.sin_family= AF_INET;
    addr.sin_port= htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int rv = connect(fd,(struct sockaddr *)&addr,sizeof(addr));

    if(rv<0){
        perror("connect()");
    }
    char msg [] = "hello from client";
    write(fd,msg,sizeof(msg)-1);

    char read_buffer[64]= {};
    ssize_t n = read(fd, read_buffer,sizeof(read_buffer));
    if(n<0){
        perror("read()");
    }
    printf("Server says: %s\n",read_buffer);

}