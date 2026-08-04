#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>


int32_t request_handler(int * fd);
const size_t k_max_size = 2048;// size_t is 64 bit and holds unsigned integer 

void msg (const char * msg){// const needed for string literals
    fprintf(stderr,"%s\n",msg);//stderr is a I/O stream which is recognized by the process,by convention, with fd as 2. 
}


static int32_t read_full(int fd,char * buffer, size_t n){ //static functions cannot be used in any other files except where they are defined 
    while(n>0){

        ssize_t rd = read (fd,buffer,n);
        if(rd<=0){

            return -1;
        }
        assert((size_t)rd <= n);
        n -= (size_t)rd;
        buffer+= rd;

    }
    return 0;
}

static int32_t write_full (int fd, char * buffer, size_t n){
    while(n>0){
        
        ssize_t wd = write(fd, buffer, n);
        if(wd<=0){
            return -1;
        }
        assert((size_t)wd<= n);
        n -= (size_t)wd;
        buffer += wd;
    }
    return 0;
}


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
    /*SOL_SOCKET used for generic socket layer.
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

    rv = listen(fd,SOMAXCONN); //2nd argument is the size of accept/pending connection queue
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
        while(true){
            
            int32_t err = request_handler(&newfd);
            if(err){
                break;
            }
        
        }
        close(newfd);
    }
    return 0;
    
}
int32_t request_handler(int * fd){
    int newfd = *fd;
    char read_buffer[k_max_size + 4];

    errno = 0;
    int32_t err = read_full(newfd,read_buffer,4);
    if(err){
        msg(errno==0?"EOF":"read() error");
        return -1;
    }
    uint32_t len= 0;
    memcpy(&len,read_buffer,4);

    if(len>k_max_size){
        msg("too long");
        return -1;
    }
    errno = 0;
    err = read_full(newfd,&read_buffer[4],len); 

    if(err){
        msg(errno==0?"EOF":"read() error");
        return -1;
    }

    printf("Client send: %.*s\n",len,&read_buffer[4]);

    char reply[] = "Hello from server";
    len = (int32_t)strlen(reply);
    char write_buffer[4 + len];
    memcpy(&write_buffer[0],&len,4);
    memcpy(&write_buffer[4],reply,len);

    return write_full(newfd,write_buffer,len+4);


}