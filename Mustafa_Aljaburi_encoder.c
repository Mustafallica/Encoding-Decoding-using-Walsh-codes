#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <unistd.h>
#define LOCALHOST "127.0.0.1"


// way to keep a socket plug and input values in one place.
typedef struct d_link{ // struct to make values manipulation easier
	int plug;
	int in_v[2];
}d_link;


// function to change decimal numbers into polar binary
void dec_to_pbin(int dec, int* pbin){
	int res[3];
	int fl=2;
	while(fl>=0){
		res[fl]=dec%2;
		dec/=2;
		fl--;
	}
	for(int i=0; i<3; ++i){
		if(res[i]==0){
			pbin[i]=-1;
		} else {
			pbin[i]=1;
		}}
}


/*
 it first opens the plug for connection, then waits for 3 connections
 to try and join. Afterwards it takes these sockets one by one takes the input
 data in from all the sockets. It compiles the Walsh signal code and secets a
 walsh de-coder matrix and send them back through the plug. Afterwards
 the whole program concludes.
*/
int main(int argc, char **argv){
	printf("\n");
	int port;
	if(argc!=2){
		printf("Usage: %s port_no\n", argv[0]);
		return 1;
	}

	port = atoi(argv[1]);
	int server_fd;
	int dummy=1;
	struct sockaddr_in address; 
	server_fd = socket(AF_INET, SOCK_STREAM, 0); // creating sockets

	if(server_fd==0){
		printf("Failed creating socket!\n");
		return -1;
	}

	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &dummy, sizeof(dummy)); //no check since if fail bind error will find out
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( port );

    if(bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0){ // binding socket to port
    	printf("Error binding socket to port!\n");
    	return -2;
    }

    listen(server_fd, 3); // no check since if fail the accept error will find out
    int child_con[3];
    d_link ret_values[3];
    int x = 3; // accepts and puts values into struct

    while(x--){
    	int i=3-(x+1);
    	int len=sizeof(address);
    	child_con[i]=accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&len);
    	if(child_con[i]<0){
    		printf("Error connecting a socket!\n");
    		return -3;
    	}

    	int inp_val[3];
    	int v_read;
    	v_read=read(child_con[i], inp_val, sizeof(int)*3);

    	//printf("Recieved %d and %d from child %d\n", inp_val[0], inp_val[1], inp_val[2]);
    	ret_values[inp_val[2]-1].in_v[0]=inp_val[0];
    	ret_values[inp_val[2]-1].in_v[1]=inp_val[1];
    	ret_values[inp_val[2]-1].plug=child_con[i];
    	//printf("Here is the message from child %d: Value = %d, Destination = %d\n", i+1, ret_values[i].in_v[1], ret_values[i].in_v[0]);
    }

    int w_one[] = {-1, 1, -1 ,1};
    int w_two[] = {-1, -1, 1, 1};
    int w_tri[] = {-1, 1, 1, -1};
    int sig_ret[12] = {0};

    /*all of this is the encoding from the worksheet*/
    for(int i=0; i<3; ++i) {
    	printf("Here is the message from child %d: Value = %d, Destination = %d\n", i+1, ret_values[i].in_v[1], ret_values[i].in_v[0]);
    	int *op;
    	switch(i) {
    		case 0:
    			op=w_one; break;
    		case 1:
    			op=w_two; break;
    		case 2:
    			op=w_tri; break;
    	}

    	int pbin[3];
    	dec_to_pbin(ret_values[i].in_v[1], pbin);
    	int sig[12] = {0};
    	int fl=0;

    	for(int j=0; j<3; ++j){
    		for(int q=0; q<4; ++q){
    			sig[fl]=op[q]*pbin[j];
    			++fl;
    		}
    	}
    	for(int j=0; j<12; ++j){
    		sig_ret[j]+=sig[j];
    	}
    }
    //returning values to client
    for(int i=0; i<3; ++i){
    	int *op;
    	switch(i){
    		case 0:
    			op=w_one; break;
    		case 1:
    			op=w_two; break;
    		case 2:
    			op=w_tri; break;
    	}

    	int dest=ret_values[i].in_v[0]-1;
    	send(ret_values[dest].plug, sig_ret, sizeof(int)*12, 0);
    	send(ret_values[dest].plug, op, sizeof(int)*4, 0);
    }
    printf("\n");
}
