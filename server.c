#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

// macros

#define PORT 2000
#define MAX_CLIENTS 10
#define BACKLOG MAX_CLIENTS*2
#define BUF 256

#define FirstByte 0

#define NORMAL_CLIENT 0
#define LOGGER 2

#define NAME_SIZE 20
#define MSG_SIZE 235
#define LOGGER_SIZE 5



// global variables

int server_fd;
struct sockaddr_in server_addr;




// sync tools

pthread_mutex_t mutex;
pthread_cond_t cond;





// ctrl+c handler

void sigint_handler(int signum){
	if(signum==SIGINT){
		printf("\nterminating server\n");
		pthread_mutex_destroy(&mutex);
		pthread_cond_destroy(&cond);
		close(server_fd);
		_exit(0);
	}
}






// Queue for clients

int Q[MAX_CLIENTS];
int rear=0;
int front=0;
int qs=0;
void init_queue(){
	for(int i=0; i<MAX_CLIENTS; i++)Q[i]=-1;
}



// takes a clientfd and enqueues it
// also notifies a waiting thread

void enqueue(int client_fd){

	pthread_mutex_lock(&mutex);
	
	if(qs<MAX_CLIENTS){
		Q[rear]=client_fd;
		rear=(rear+1) % MAX_CLIENTS;
		qs++;
		pthread_cond_signal(&cond);
	}
	else{
		printf("max clients accepted, cannot accept this one right now\n");
		close(client_fd);
	}
	
	pthread_mutex_unlock(&mutex);
}




// dequeue removes the front client fd if available, if not it waits for it
// then returns it

int dequeue(){
	
	
	pthread_mutex_lock(&mutex);
	
	while(qs==0){
		// printf("wating for a client, currently queue is empty\n");
		pthread_cond_wait(&cond, &mutex);
	}
	
	int client_fd = Q[front];
	Q[front]=-1;						// reset it
	front= (front+1) % MAX_CLIENTS;
	qs--;
	
	pthread_mutex_unlock(&mutex);
	
	return client_fd;

}







// loggers

int loggers[LOGGER_SIZE];
void init_logger(){
	for(int i=0; i<LOGGER_SIZE; i++)loggers[i]=-1;
}








// accept any incoming connection request

void acceptor(){

	while(1){
		
		struct sockaddr_in client_addr;
		socklen_t client_len = sizeof(client_addr);
		
		int client_fd = accept(server_fd, (struct sockaddr* )&client_addr, &client_len);
		
		if(client_fd == -1){
			if(errno == EINTR) {
			  // Interrupted by signal, time to exit acceptor loop
			  printf("accept() interrupted by signal\n");
			  break;
            	}
            	else{
				printf("error accepting this client\ncontinuing\n");
				continue;
            	}
		}
		
		char ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
		
		printf("accepted connection with client at %s and port %d\n", ip, ntohs(client_addr.sin_port));
		
		enqueue(client_fd);
	}
	
	return;

}



// all threads will have to wait here, till a client is available

void* pool_of_threads(void* arg){


	while(1){
		int client_fd = dequeue();
		
		if(client_fd == -1){
			continue;
		}
		
		printf("thread %d is handling a connection\n", (int)pthread_self());
		
		char buf[BUF];
		
		while(1){
		
			memset(buf, 0 ,BUF);
			
			int bytes_read = recv(client_fd, buf, BUF, 0);
			
			if(bytes_read == -1){
				printf("some error receiving this message...continuing\n");
				continue;
			}
			
			if(bytes_read == 0){
				printf("client called for unconditional termination\n");
				close(client_fd);
				break;
			}
			
			buf[bytes_read] = '\0';
			
			// now we have the message, we just need to parse it
			
			if( buf[FirstByte] == NORMAL_CLIENT ){
				
				char name[NAME_SIZE+1]={0};
				strncpy(name, &buf[1], NAME_SIZE);
				name[NAME_SIZE]='\0';
				
				
				char msg[MSG_SIZE+1]={0};
				strncpy(msg, &buf[21],MSG_SIZE);
				msg[MSG_SIZE]='\0';
				
				
				printf("%s : %s\n", name, msg);
				
				for(int i=0; i<LOGGER_SIZE; i++){
					if(loggers[i] != -1){
						send(loggers[i], buf, BUF,0);
					}
				}
				
				
				if(strcmp(msg,"close")==0){
					printf("closing this client\n");
					close(client_fd);
					break;
				}
				
			}
			
			// add a logger
			else if(buf[FirstByte] == LOGGER){
				for(int i=0; i<LOGGER_SIZE; i++){
					if(loggers[i]==-1){
						loggers[i]=client_fd;
						break;
					}
				}
			}	
		}
	}
}




int main(){

	//signal(SIGINT, sigint_handler); // handle ctrl+c termination
	
	// better way would be to use sigaction
	
	struct sigaction sa;
	sa.sa_handler= sigint_handler;
	sa.sa_flags=0;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGINT, &sa, NULL);
	
	
	server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(server_fd == -1){
		printf("error creating server\n");
		return 1;
	}
	
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);
	
	
	
	int opt=1;
	int setsockopt_ret = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if(setsockopt_ret == -1){
		printf("error setting port reuse for server\ncontinuing\n");
	}
	
	
	int bind_ret = bind(server_fd, (struct sockaddr* )&server_addr, sizeof(server_addr));
	if(bind_ret == -1){
		printf("error in bind\nclosing\n");
		close(server_fd);
		return 2;
	}
	
	printf("bind done\n");
	
	
	int listen_ret = listen(server_fd, BACKLOG);
	if(listen_ret == -1){
		printf("error in listening for new connections\nclosing\n");
		close(server_fd);
		return 3;
	}
	
	printf("server listening at port : %d\n", ntohs(server_addr.sin_port));
	
	
	pthread_mutex_init(&mutex,NULL);
	pthread_cond_init(&cond,NULL);
	init_queue();
	init_logger();
	
	pthread_t threads[10];
	
	for(int i=0; i<10; i++){
		int ret = pthread_create(&threads[i], NULL, &pool_of_threads, NULL);
		if(ret!=0){
			printf("error creating thread %d, continuing\n", i);
		}
	}
	
	
	
	acceptor();
	
	
	
	close(server_fd);
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
	
	printf("server closed\n");
	

	return 0;
}































