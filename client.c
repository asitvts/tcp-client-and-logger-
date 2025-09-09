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


int client_fd;
struct sockaddr_in server_addr;

#define BUF 256
#define PORT 2000

#define FirstByte 0
#define NORMAL_CLIENT 0
#define NAME_SIZE 20


char name[NAME_SIZE];


int main(){
	
	client_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(client_fd == -1){
		printf("error making socket\n");
		return 1;
	}
	
	
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
	server_addr.sin_port = htons(PORT);
	
	
	
	int connect_ret = connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(connect_ret == -1){
		printf("error connecting\n");
		close(client_fd);
		return 2;
	}
	
	printf("connection made..\n");
	
	
	// lets get clients name
	printf("give your name: ");
	fgets(name, sizeof(name), stdin);
	name[strcspn(name, "\n")] = '\0';
	
	printf("\nnow you can start talking\n");
	
	
	fd_set readfds;
	
	int max = STDIN_FILENO > client_fd ? STDIN_FILENO :  client_fd;
	
	while(1){
		FD_ZERO(&readfds);
		FD_SET( STDIN_FILENO,&readfds);
		FD_SET(client_fd,&readfds);
		
		int select_ret = select(max+1, &readfds, NULL, NULL, NULL);
		
		if(select_ret == -1){
			printf("error in select...continuing\n");
			continue;
		}
		
		if(FD_ISSET(STDIN_FILENO,&readfds)){
		
			char* string = NULL;
			size_t size= 0;
			ssize_t bytes_read = getline(&string, &size, stdin);
			if(bytes_read<0){
				printf("error reading from stdin continuing\n");
				free(string);
				string=NULL;
				continue;
			}
			
			string[strcspn(string,"\n")]='\0';
			
			char buf[BUF]={0};
			
			buf[FirstByte]=NORMAL_CLIENT;
			
			strncpy(&buf[1], name, NAME_SIZE);
			
			strncpy(&buf[1 + NAME_SIZE], string, BUF - 1 - NAME_SIZE);

			
			
			int send_ret=send(client_fd, buf, BUF, 0);
			if(send_ret==-1){
				printf("error sending the message.. try again...\n");
				free(string);
				string=NULL;
				continue;
			}
			
			if(strcmp(string, "close")==0){
				printf("close detected.. closing\n");
				break;
			}
		}
		
		if(FD_ISSET(client_fd, &readfds)){
			char buf[BUF]={0};
			
			
			int bytes_read = recv(client_fd, buf, BUF, 0);
			if (bytes_read <= 0) {
			    if (bytes_read == 0) {
				   printf("Server closed connection.\n");
				   break;
			    } 
			    else {
				   perror("recv");
				   continue;
			    }
			}
			
			buf[bytes_read]='\0';
			
			printf("msg : %s\n", buf);
		
		}
	
	}
	
	
	close(client_fd);
	printf("client closed\n");
	
	


	return 0;
}















