#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/mman.h>


/*
 3 structures within the shared memory that contain
 all the data that the forked programs needed and would use to save the
 return from the encoder
*/

struct proc_datum { //structure containing all the import data to pass to the chiuld threads
	int num; //id of thread
	int sig[12]; //signal from the server
	int w_mat[4]; //code from the server
	unsigned val; //value to pass to the server
	unsigned dest; //destination to pass the value to
	unsigned ret; // raturn value
	int port; //port to socket
	char *addr; //address to server
};




/*
 Takes a pointer to one of proc_datum, opens a socket
 to the server provided in the arguments and one line of the 3 line input
 (destination process and value). It then send s the raw data to the sever
 program and waits for it to return. Afterwards it decodes the values from
 the signals received from the socket and concludes so it can be used in the
 parent program.
*/

void mul_prog(struct proc_datum *datum) { // multi threaded program 
	struct proc_datum *operant = (struct proc_datum *)datum;
	struct sockaddr_in address;
	int client = socket(AF_INET, SOCK_STREAM, 0);

	if (client < 0) {
		printf("Error creating socket!\n");
		exit(-1);
	}

	memset(&address, '0', sizeof(address)); // creating the address struct
	address.sin_family = AF_INET;
	address.sin_port = htons(operant->port);

	// converting adrerss from IP4 to binary
	if (inet_pton(AF_INET, operant->addr, &address.sin_addr) <= 0) { 
		printf("Invalid hostname\n");
		exit(-2);
	}

	free(operant->addr);
	int conn = -1;
	while (conn < 0) { //continuously try to connect
		conn = connect(client, (struct sockaddr *)&address, sizeof(address));
	}

	int message[3];
	message[0] = operant->dest, message[1] = operant->val, message[2] = operant->num; // genrate the message to send {destination_id, value, id}
	printf("Child %d, sending value: %d to child process %d\n", message[2], message[1], message[0]);
	send(client, message, sizeof(int) * 3, 0);
	int v_read;
	v_read = read(client, operant->sig, sizeof(int) * 12); // wait for the signal to aarrive
	v_read = read(client, operant->w_mat, sizeof(int) * 4); // wait for the code to arrive
	int eni[12]; // decrypting message recieved

	/*all of the below is from the worksheet*/
	for (int i = 0; i < 12; ++i) {
		eni[i] = operant->sig[i] * operant->w_mat[i % 4];
	}

	int fl = 0;
	int bin[3];
	
	for (int i = 0; i < 3; ++i) {
		int sum = 0;
		int x = 4;

		while (x--) {
			sum += eni[fl++];
		}

		bin[i] = sum / 4;

		if (bin[i] == -1) {
			bin[i] = 0;
		}
	}
	operant->ret = 4 * bin[0] + 2 * bin[1] + bin[2];
	//printf("RETURN: %d\n", operant->ret);
}


// standard function for shared memory in mman library.
struct proc_datum* memory_share(){
	int prot = PROT_READ | PROT_WRITE;
	int vis = MAP_ANONYMOUS | MAP_SHARED;
	return mmap(NULL, sizeof(struct proc_datum), prot, vis, 0, 0);
}



/*
 creates shgared memory for the 3 structs haves input data, address and port onto it.
 Forks 3 times and wait for data to be received. It then outputs it.
*/
int main(int argc, char **argv) {
	printf("\n");
	struct proc_datum *procs[3];

	if (argc != 3) {
		printf("Usage: %s hostname port_no\n\n", argv[0]);
		return 1;
	}

	for (int i = 0; i < 3; ++i) { //create data structure
		procs[i] = memory_share();
		scanf("%d %d", &procs[i]->dest, &procs[i]->val);
		procs[i]->port = atoi(argv[2]);
		procs[i]->addr = malloc(strlen(argv[1] + 1));
		strcpy(procs[i]->addr, argv[1]);
		procs[i]->num = i + 1;
	}
	int status=0;
	pid_t child_pid[3];
	for (int i = 0; i < 3; ++i) { //create threads with the data structures
		child_pid[i] = fork();
		if(child_pid[i]!=0)
			continue;
		mul_prog(procs[i]);
		return 0;
	}
	for(int i=0; i<3; ++i){
		waitpid(child_pid[i], NULL, 0);
		//printf("Process[%d] ended\n", i+1);
	}

	for (int fl = 0; fl < 3; ++fl) { //output children data
		printf("\nChild %d\n", fl + 1);
		printf("Signal:");

		for (int i = 0; i < 12; ++i) {
			printf("%d ", procs[fl]->sig[i]);
		}

		printf("\nCode: ");
		for (int i = 0; i < 4; ++i) {
			printf("%d ", procs[fl]->w_mat[i]);
		}

		printf("\nReceived value = %d\n\n", procs[fl]->ret);
	}
	//printf("END!\n");
}
