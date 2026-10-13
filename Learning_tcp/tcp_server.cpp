#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include<vector>
#include<poll.h> 
#include<stdlib.h> 
#include<fcntl.h> 
#include<iostream>

using namespace std;

struct Conn{
    int fd = -1;
    bool want_write = false;
    bool want_read = false;
    bool want_close = false;

    vector<uint8_t> incoming;
    vector<uint8_t> outgoing;
};

const size_t k_max_size = 2048;// size_t is 64 bit and holds unsigned integer 

void msg (const char * msg){// const needed for string literals
    fprintf(stderr,"%s\n",msg);//stderr is a I/O stream which is recognized by the process,by convention, with fd as 2. 
}
static void die(const char *msg) {
    fprintf(stderr, "[%d] %s\n", errno, msg);
    abort();
}
static void fd_set_nb(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

static void buf_append(vector<uint8_t> &buf, uint8_t * data, size_t len){
    buf.insert(buf.end(),data, data+len);
}
static void buf_consume(vector<uint8_t> &buf, size_t n  ){
    buf.erase(buf.begin(), buf.begin()+n);
}
static bool try_one_request(Conn * conn){
    if(conn->incoming.size()<4){
        return false;//not enough bytes to even read
                    // the length of the message
    }
    uint32_t len = 0;
    memcpy(&len,conn->incoming.data(),4);

    if(len>k_max_size){
        conn->want_close= true;// msg too large
        return false;
    }
    if(conn->incoming.size()< 4 + len){
        return false;// one complete request not arrived
    }
   

    //Processing the request:
    uint8_t request[64 * 1024]; 
    memcpy(request, &conn->incoming[4],len );
    cout<<"The incoming request is: ";
    for(int i = 0 ; i < (int)len; i ++ ){
        cout<< (char)request[i];
    }
    cout<<endl;

    //generating response 

    //this is an simple echo server
    //-following message framing-
    buf_append(conn->outgoing,(uint8_t*) &len,4);
    buf_append(conn->outgoing, request, len);

    buf_consume(conn->incoming, 4+len);//removing parsed message from
                                        //incoming buffer

    return true;//success

}

static Conn* handle_accept(int fd){
 

    //Acceptence logic 
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        int newfd = accept(fd,(struct sockaddr *)&client_addr, &addrlen);

        if(newfd<0){return nullptr;}

        fd_set_nb(newfd);

        struct Conn* conn = new Conn();
        conn->fd = newfd;
        conn->want_read = true;
        return conn;

}

static void handle_read(Conn* conn){
    uint8_t buf[64 * 1024];
    ssize_t rv = read (conn->fd,buf,64*1024);

    if(rv<=0){
        conn->want_close= true;
        return;
    }
    buf_append(conn->incoming, buf,(size_t) rv);

    while(try_one_request(conn)){};

    if(conn->outgoing.size()>0){//response is ready
        conn->want_read= false;
        conn->want_write = true;

    }

}

static void handle_write(Conn*conn){
    assert(conn->outgoing.size()>0);
    //indicates some bug if handle_write() is called
    //but outgoing buffer is empty 

    ssize_t rv = write(conn->fd, conn->outgoing.data(),conn->outgoing.size());
    if(rv<0){
        conn->want_close= true;
        return;

    }
    buf_consume(conn->outgoing, (size_t)rv);

     if(conn->outgoing.size()==0){//response is sent
        conn->want_read= true;
        conn->want_write = false;

    }
     
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

    int rv = bind(fd,(const struct sockaddr*)&addr,sizeof(addr));//copies the data from struct sockaddr_in addr to socket object

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
    vector<struct Conn* > fd2conn;
    vector<struct pollfd > poll_args;

    while(true){
        
        poll_args.clear();
        struct pollfd pfd = {fd,POLLIN,0};
        poll_args.push_back(pfd);

        for(Conn *conn:fd2conn){
            if(!conn){
                continue;
            }
            pfd = { conn->fd, POLLERR, 0};
            if(conn->want_read){
                pfd.events|= POLLIN;
            }
            if(conn->want_write){
                pfd.events|=POLLOUT;
            }
            poll_args.push_back(pfd);
        }
        int p = poll(poll_args.data(),poll_args.size(),-1);
        if(p<0 && errno == EINTR){
            continue;

        }
        if(p<0){
            die("poll");
        }

        if(poll_args[0].revents){
            if(Conn * conn = handle_accept(fd)){
                if(conn->fd>=(int)fd2conn.size()){
                    fd2conn.resize(conn->fd+1);
                }
                fd2conn[conn->fd]= conn;
            }
        }
        for(int i = 1;i<(int)poll_args.size();i++){

            uint32_t ready = poll_args[i].revents;
            Conn * conn = fd2conn[poll_args[i].fd];
            if(ready & POLLIN){

                handle_read(conn);
            }
            if(ready & POLLOUT){
                handle_write(conn);

            }
            //termination logic

            if(ready & POLLERR || conn->want_close){
                close(conn->fd);
                fd2conn[conn->fd]= nullptr;
                delete conn;
            }

        }     
        
    }
    return 0;
    
}
