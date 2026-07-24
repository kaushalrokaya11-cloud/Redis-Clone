#include<stdio.h>
#include<sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

void work(int * fd);

int main(){
    int fd = socket(AF_INET,SOCK_STREAM,0);

    if(fd<0){
        perror("socket()");
        return -1;
    }
    else{
        printf("Socket created\n");
    }


    int val = 1;
    int status = setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&val,sizeof(val)); 
    /*SOL_SOCKET used for generic socket layer(independent to protocals).
    SO_REUSEADDR is used to ignore the cooldown due to TIME_WAIT and bind the new server anyway(prevents error: port.address alreay in use)
    val is the bool value to switch on/off SO_REUSEADDR*/ 
    if(status<0){
        perror("setsocketopt()");
        return -1;
    }


    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(0);

    int rv = bind(fd,(const struct sockaddr*)&addr,sizeof(addr));

    if(rv){
        perror("bind");
        return -1;
    }

    rv = listen(fd,SOMAXCONN); //2nd argument is teh size of accept/pending connection queue
    if(rv<0){
        perror("listen()");
        return -1;
    }
    else{
        printf("Server is running\n");
    }
    while(true){
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        int newfd = accept(fd,(struct sockaddr *)&client_addr, &addrlen);

        if(newfd<0){
            continue;
        }
        work(&newfd);
        close(newfd);
    }

    
}
void work(int * fd){
    int newfd = *fd;
    char read_buffer[64]= {};
    ssize_t n = read(newfd,read_buffer,sizeof(read_buffer)-1);
    if(n<0){
        perror("Work: read error");
        return ;
    }
    printf("Client send: %s\n",read_buffer);
    char write_buffer[]="Hello from server!";

    write(newfd,write_buffer,sizeof(write_buffer)-1);

}