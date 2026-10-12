#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>



const size_t k_max_size = 2048;

void msg (const char * msg){
    fprintf(stderr,"%s\n",msg);
}

static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

static int32_t read_full(int fd,char * buffer, size_t n){ 
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
static int32_t query(int * fd, const char * text){
    //writing the query
    uint32_t len = (uint32_t)strlen(text);
    if(len>k_max_size){
        msg("text is too long");
        return -1;
    }
    char write_buffer[4 + k_max_size];

    memcpy(write_buffer,&len,4);


    memcpy(&write_buffer[4],text,len);
    ssize_t err = write_full(*fd,write_buffer,4+len);
    if(err){

        return -1;
    }

    //reading the response from teh server
    char read_buffer[4+ k_max_size];
    errno =0;

    err = read_full(*fd,read_buffer,4);

    if(err){
        msg(errno==0?"EOF":"read() error");
        return -1;
    }

    memcpy(&len,&read_buffer[0],4);
    if(len>k_max_size){
        msg("reply from server is too long");
        return -1;
    }

    errno=0;
    err = read_full(*fd,&read_buffer[4],len);
    if(err){
        msg(errno==0?"EOF":"read() error");
        return -1;
    }


    printf("Server responded: %.*s\n",len,&read_buffer[4]);
    return 0;
}

int main(){
    int fd = socket(AF_INET,SOCK_STREAM,0);
    if(fd<0){
        perror("socket()");
        return -1;
    }
    struct sockaddr_in addr = {};
    addr.sin_family= AF_INET;
    addr.sin_port= htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int rv = connect(fd,(struct sockaddr *)&addr,sizeof(addr));

    if(rv<0){
        perror("connect()");
        return -1;
    }

    int32_t err = query(&fd,"hello! From Kaushal");
    if(err){
        goto DONE;
    }
    err = query(&fd,"hello! From Mohit");
    if(err){
        goto DONE;
    }

    DONE:
    close(fd);
    return 0;

}