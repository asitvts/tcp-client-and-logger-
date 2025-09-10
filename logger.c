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


int logger_fd;
struct sockaddr_in server_addr;

#define BUF 256
#define PORT 2000

#define FirstByte 0
#define LOGGER 2
#define NAME_SIZE 20
#define MSG_SIZE 235




int main(int argc, char **argv){

	time_t now;
	struct tm* local;
	char time_buf[80];
	
	logger_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(logger_fd == -1){
		printf("error making socket\n");
		return 1;
	}
	
	
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
	server_addr.sin_port = htons(PORT);
	
	
	
	int connect_ret = connect(logger_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(connect_ret == -1){
		printf("error connecting\n");
		close(logger_fd);
		return 2;
	}
	
	printf("connection made..\n");
	
	
	
	char buf[BUF]={0};
	buf[FirstByte]=LOGGER;
	int bytes_sent= send(logger_fd, buf, BUF, 0);
	
	if(bytes_sent<0){
		printf("error sending logger identity\n");
		close(logger_fd);
		return 3;
	}
	
	FILE* fptr = NULL;
	fptr = fopen(argv[1], "a");
	if(fptr==NULL){
		printf("error opening the file.. exiting\n");
		return 4;
	}
	
	
	
	
	while(1){
		
		// logger will now only receive
		
		
		memset(buf, 0, BUF);
		
		int bytes_recv=recv(logger_fd, buf, BUF, 0);
		
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
		
		char name[NAME_SIZE+1]={0};
		strncpy(name, &buf[1], NAME_SIZE);
		name[NAME_SIZE]='\0';
		
		
		char msg[MSG_SIZE+1]={0};
		strncpy(msg, &buf[21],MSG_SIZE);
		msg[MSG_SIZE]='\0';
		
		
		
		printf("msg : ' %s ' by : ' %s ' at time : ' %s '\n", msg,name,time_buf);		// show on stdout
		
		fprintf(fptr, "%s    %s :  %s\n", time_buf,name,msg);		// write in file
	
		fflush(fptr); 		// this prevents data loss on crash
	}
	
	
	fclose(fptr);
	close(logger_fd);
	printf("logger closed\n");
	
	


	return 0;
}















