#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <signal.h>
#include <sys/time.h>
#include "rdwrn.h"

#define INPUTSIZE 10
struct timeval tv1, tv2;

static void handler (int, siginfo_t *, void *);
char get_user_input(int);
void send_student_info(int);
void send_random_numbers(int);
void send_uname_info(int);
void send_filenames(int);
void send_file(int);
void *client_handler(void *);

int main(void)
{
    struct sigaction act;
    memset(&act, '\0', sizeof(act));

    act.sa_sigaction = &handler;

    act.sa_flags = SA_SIGINFO;

    if (sigaction(SIGINT, &act, NULL) == -1) {
		perror("sigaction");
		exit(EXIT_FAILURE);
    }

	if (gettimeofday(&tv1, NULL) == -1) {
    	perror("gettimeofday error");
    	exit(EXIT_FAILURE);
	}


    int listenfd = 0, connfd = 0;

    struct sockaddr_in serv_addr;
    struct sockaddr_in client_addr;
    socklen_t socksize = sizeof(struct sockaddr_in);
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(50031);

    bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

    if (listen(listenfd, 10) == -1) {
		perror("Failed to listen");
		exit(EXIT_FAILURE);
    }

    puts("Waiting for incoming connections...");
    while (1) {
		printf("Waiting for a client to connect...\n");
		connfd =
	    	accept(listenfd, (struct sockaddr *) &client_addr, &socksize);
		printf("Connection accepted...\n");

		pthread_t sniffer_thread;
		if (pthread_create (&sniffer_thread, NULL, client_handler, (void *) &connfd) < 0) {
	    	perror("could not create thread");
	    	exit(EXIT_FAILURE);
		}
		printf("Handler assigned\n");
    }

    exit(EXIT_SUCCESS);
}


static void handler(int sig, siginfo_t *siginfo, void *context)
{
	if (gettimeofday(&tv2, NULL) == -1) {
		perror("gettimeofday error");
		exit(EXIT_FAILURE);
	}

    printf("\nTotal execution time = %f seconds\n",
	  	(double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
   		(double) (tv2.tv_sec - tv1.tv_sec));

	exit(130);
}


char get_user_input(int socket){
	char input [INPUTSIZE];

	readn(socket, (unsigned char *) input,sizeof(size_t));

	return input[0];
}


void send_student_info(int socket){
	char hostbuffer[256];
	char *ip_address;
	struct hostent *host_machine;
	int hostname;
	char student_info [50] = ", Michael Kofi Badu, S1719029";
	char combined [100];

	hostname = gethostname(hostbuffer, sizeof(hostbuffer));

	host_machine = gethostbyname(hostbuffer);

	ip_address = inet_ntoa(*((struct in_addr*)
						host_machine->h_addr_list[0]));

	strcat(combined, ip_address);
	strcat(combined, student_info);

	size_t n = strlen(combined) + 1;

	writen(socket, (unsigned char *) &n, sizeof(size_t));
	writen(socket, (unsigned char *) &combined, n);

}


void send_random_numbers(int socket){
	int i, k;
	char random_list [10000];

	srand(time(0));

	for (i = 0; i <= 4; i++) {
		char str_k[5];

	 	k = rand() % 1001;
		sprintf(str_k, "%d", k);
	 	if (i==0){
	 		strcat(random_list,str_k);
 		}
		else{
			strcat(random_list,",");
			strcat(random_list,str_k);
		}
	}
	size_t n = strlen(random_list);
	writen(socket, (unsigned char *) &n, sizeof(size_t));
	writen(socket, (unsigned char *) random_list, n);

}


void send_uname_info(int socket){
	struct utsname uts;

	if (uname(&uts) == -1) {
    	perror("uname error");
    	exit(EXIT_FAILURE);
	}

    size_t payload_length = sizeof(uts);

	writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
    writen(socket, (unsigned char *) &uts, payload_length);

}


void send_filenames(int socket){
	char *files_list = malloc(1000 * sizeof(char));
	DIR *mydir;
        if ((mydir = opendir("upload")) == NULL) {
    		perror("error");
    		exit(EXIT_FAILURE);
        }

	struct dirent *entry = NULL;

	while ((entry = readdir(mydir)) != NULL){
		strcat(files_list,entry->d_name);
		strcat(files_list,"\n");
	}

	size_t n = strlen(files_list);
	writen(socket, (unsigned char *) &n, sizeof(size_t));
	writen(socket, (unsigned char *) files_list, n);
        free(files_list);
}


void send_file(int socket){
	char filename [50];
	FILE *filePointer ;
        char *file_content = malloc(10000 * sizeof(char));
	char line[100];

	readn(socket, (unsigned char *) filename, sizeof(strlen(filename)));
	char upload [60]= "./upload/";

	strcat(upload,filename);
	filePointer = fopen(upload, "rb") ;
	if ( filePointer == NULL ){
		printf( "%s file failed to open.\n",filename ) ;
	}
	else{
		while( fgets ( line, 50, filePointer ) != NULL ){
			strcat(file_content,line);
		}
		fclose(filePointer) ;
	}
	printf("%s", file_content);
	size_t n = strlen(file_content);
	writen(socket, (unsigned char *) &n, sizeof(size_t));
        writen(socket, (unsigned char *) file_content, n);

        free(file_content);
}


void *client_handler(void *socket_desc)
{
    int connfd = *(int *) socket_desc;
	char option;
	do {
		option = get_user_input(connfd);
		switch (option) {
			case '1':
				send_student_info(connfd);
				break;
			case '2':
				send_random_numbers(connfd);
				break;
			case '3':
				send_uname_info(connfd);
				break;
			case '4':
				send_filenames(connfd);
				break;
			case '5':
				send_file(connfd);
				break;
			case '6':
				printf("Goodbye!\n");
				break;
			case 's':
				printf("Goodbye!\n");
			default:
				{};
			break;
		}
    		}
    		while (option != '6' || option != 's');

    shutdown(connfd, SHUT_RDWR);
    close(connfd);

    printf("Thread %lu exiting\n", (unsigned long) pthread_self());

    shutdown(connfd, SHUT_RDWR);
    close(connfd);

    return 0;
}





