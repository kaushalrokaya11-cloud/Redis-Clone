#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <vector>
#include <string>

using namespace std;




const size_t k_max_size = 32<<20;

void msg (const char * msg){
    fprintf(stderr,"%s\n",msg);
}

static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

static int32_t read_full(int fd,uint8_t * buffer, size_t n){ 
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

static int32_t write_full (int fd, uint8_t * buffer, size_t n){
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
static int32_t send_req(int  fd,string &text){
    //writing the query
    uint32_t len = (uint32_t)text.length();
    if(len>k_max_size){
        msg("text is too long");
        return -1;
    }
    vector<uint8_t> write_buffer(4 + len);

    memcpy(write_buffer.data(),&len,4);
    memcpy(&write_buffer[4],text.data(),len);

    ssize_t err = write_full(fd,write_buffer.data(),4+len);
    if(err){

        return -1;
    }
    return 0;
}
static int32_t read_res(int fd){

    //reading the response from the server
    vector<uint8_t> read_buffer(4);
    errno =0;

   int err = read_full(fd,read_buffer.data(),4);

    if(err){
        msg(errno==0?"EOF":"read() error");
        return -1;
    }
    u_int32_t len;
    memcpy(&len,&read_buffer[0],4);
    if(len>k_max_size){
        msg("reply from server is too long");
        return -1;
    }

    errno=0;
    read_buffer.resize(4+len);
    err = read_full(fd,&read_buffer[4],len);
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

    vector<string> req_list  = {
        "request1", "request2", "request3", string(k_max_size,'h'),
        "request4"
    };
    //sending req
    for (auto &str: req_list){

        int32_t err = send_req(fd,str);
        if(err){
            goto DONE;
        }
    }
    //reading response
    for(int i = 0;i< req_list.size();i++){

       int err = read_res(fd);
        if(err){
            goto DONE;
        }
    }
        
    DONE:
    close(fd);
    return 0;

}