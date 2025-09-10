#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <sys/select.h>
#include <time.h>


int stat_fd;
struct sockaddr_in server_addr;

#define BUF 256
#define PORT 2000

#define FirstByte 0
#define NAME_SIZE 20
#define MSG_SIZE 235
#define STATUS_MANAGER 1


void sigint_handler(int signum){

	if(signum==SIGINT){
		printf("terminating\n");
		close(stat_fd);
		exit(0);
	}
}

int main(int argc, char **argv){

	struct sigaction sa;
	sa.sa_handler= sigint_handler;
	sa.sa_flags=0;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGINT, &sa, 0);

	time_t now;
	struct tm* local;
	char time_buf[80];
	
	stat_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(stat_fd == -1){
		printf("error making socket\n");
		return 1;
	}
	
	
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
	server_addr.sin_port = htons(PORT);
	
	
	
	int connect_ret = connect(stat_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(connect_ret == -1){
		printf("error connecting\n");
		close(stat_fd);
		return 2;
	}
	
	printf("connection made..\n");
	
	
	
	char buf[BUF]={0};
	buf[FirstByte]=STATUS_MANAGER;
	int bytes_sent= send(stat_fd, buf, BUF, 0);
	
	if(bytes_sent<0){
		printf("error sending status manager identity\n");
		close(stat_fd);
		return 3;
	}
	
	FILE* fptr = NULL;
	fptr = fopen(argv[1], "a");
	if(fptr==NULL){
		printf("error opening the file.. exiting\n");
		return 4;
	}
	
	
	
	
	while(1){
		
		// status manager will now only receive
		
		
		memset(buf, 0, BUF);
		
		int bytes_recv=recv(stat_fd, buf, BUF, 0);
		
		if(bytes_recv<0){
			printf("error receiving logs from server... continuing\n");
			continue;
		}
		
		else if(bytes_recv==0){
			printf("unconditional termination of server\nquitting\n");
			break;
		}
		
		time(&now);
		
		local= localtime(&now);
		
		strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", local);
		
		buf[bytes_recv]='\0';
		
		
		//char conns[2]={0};
		//strncpy(&conns[0],buf,1);
		//conns[1]='\0';		// although there is no need for it
		
		
		unsigned int total = (unsigned char)buf[0];
		printf("total active connections : %u\n", total);
		fprintf(fptr, "%s    total active connections: %u \n", time_buf, total);

		
		
		//printf("total active connections : %s\n", conns);		// show on stdout
		
		//fprintf(fptr, "%s    total active connections: %s \n", time_buf,conns);		// write in file
	
		fflush(fptr); 		// this prevents data loss on crash
	}
	
	
	fclose(fptr);
	close(stat_fd);
	printf("status manager closed\n");
	
	


	return 0;
}















