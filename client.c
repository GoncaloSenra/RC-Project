#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>



#define BUFLEN 512    // Tamanho do buffer
#define PORT_MULT 7000

void erro(char *s) {
    perror(s);
    exit(1);
}

void worker(char ip[50]);


int fd, client;
struct sockaddr_in mult;
int client_addr_size, addrlen;
struct ip_mreq mreq;
int sock, cnt;



int main(int argc, char *argv[]) {
	char endServer[100];
	int fd;
	struct sockaddr_in addr;
	struct hostent *hostPtr;
	
	if (argc != 3) {
		printf("operations_terminal <endereco_servidor> <porto_bolsa>\n");
		exit(-1);
	}
	
	strcpy(endServer, argv[1]);
	if ((hostPtr = gethostbyname(endServer)) == 0)
		erro("Não consegui obter endereço");
	
	bzero((void *) &addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
	addr.sin_port = htons((short) atoi(argv[2]));
	
	if ((fd = socket(AF_INET,SOCK_STREAM,0)) == -1)
		erro("socket");
	if (connect(fd,(struct sockaddr *)&addr,sizeof (addr)) < 0)
		erro("Connect");
	
	
	char user[BUFLEN], pass[BUFLEN], buf[BUFLEN];	
	
	
	printf("User: ");
	fgets(user, BUFLEN, stdin);
	user[strlen(user)-1] = '\0';
	write(fd, user, strlen(user)+1);
	fflush(stdout);
	
	printf("Password: ");
	fgets(pass, BUFLEN, stdin);
	pass[strlen(pass)-1] = '\0';
	write(fd, pass, strlen(pass)+1);
	fflush(stdout);
	
	read(fd, buf, BUFLEN);
	puts(buf);
	fflush(stdout);
	bzero(buf, strlen(buf));
	
	pid_t pid;

	while(1) {
		fgets(buf, BUFLEN, stdin);
		write(fd, buf, strlen(buf)+1);
		fflush(stdout);
		
		char *token = strtok(buf, " \n");
        char splitedBuf[2][100];
        int aux = 0;
        while (token != NULL) {
            strcpy(splitedBuf[aux],token);
            aux++;
            token = strtok(NULL, " \n");
        }
        
        if (strcmp(splitedBuf[0], "1") == 0){
        	bzero(buf, BUFLEN);
        	int nread = read(fd, buf, 100);
			buf[nread] = '\0';      	
        	if ((pid = fork()) == 0) {
        		worker(buf);
        		exit(0);
        	}
        } else if (strcmp(splitedBuf[0], "0") == 0) {
			kill(pid, SIGKILL);
		}else if (strcmp(splitedBuf[0],"BUY") == 0){
			char mes[BUFLEN];
			int nread = read(fd, mes, BUFLEN);
			mes[nread] = '\0';
			puts(mes);
			bzero(mes, BUFLEN);
		}else if (strcmp(splitedBuf[0], "WALLET") == 0) {
			char mes[BUFLEN];
			int nread = read(fd, mes, BUFLEN);
			mes[nread] = '\0';
			puts(mes);
			bzero(mes, BUFLEN);
		}else if (strcmp(splitedBuf[0], "BUY") == 0) {
			char mes[BUFLEN];
			int nread = read(fd, mes, BUFLEN);
			mes[nread] = '\0';
			puts(mes);
			bzero(mes, BUFLEN);
		}else if (strcmp(splitedBuf[0], "SELL") == 0) {
			char mes[BUFLEN];
			int nread = read(fd, mes, BUFLEN);
			mes[nread] = '\0';
			puts(mes);
			bzero(mes, BUFLEN);
		} else if (strcmp(splitedBuf[0], "QUIT") == 0) {
			kill(pid, SIGKILL);
			close(fd);
			return 0;
		} else if (strcmp(splitedBuf[0], "SUB") == 0) {
			char mes[BUFLEN];
			int nread = read(fd, mes, BUFLEN);
			mes[nread] = '\0';
			puts(mes);
			bzero(mes, BUFLEN);
		} else if (strcmp(splitedBuf[0], "UNSUB") == 0) {
			char mes[BUFLEN];
			int nread = read(fd, mes, BUFLEN);
			mes[nread] = '\0';
			puts(mes);
			bzero(mes, BUFLEN);
		} else {
			printf("COMMAND NOT FOUND\n");
		}
	
		//read(fd, buf, BUFLEN);
		//puts(buf);
		//fflush(stdout);
		bzero(buf, strlen(buf));
		//sleep(2);
	}
	
	return 0;	  
}

void worker(char ip[50]) {

	char message[BUFLEN]; 
	
	mreq.imr_multiaddr.s_addr = inet_addr(ip); 
	mreq.imr_interface.s_addr = htonl(INADDR_ANY); 
	
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("socket");
		exit(1);
	}

	int multicastTTL = 255;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *) &multicastTTL, sizeof(multicastTTL)) < 0) {
		perror("setsockopt mreq");
		exit(1);
	}
	if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
    	erro("setsockopt mreq");
    }

	bzero((char *)&mult, sizeof(mult));
	mult.sin_family = AF_INET;
	mult.sin_addr.s_addr = htonl(INADDR_ANY);
	mult.sin_port = htons(PORT_MULT);
	
	if (bind(sock, (struct sockaddr *) &mult, sizeof(mult)) < 0) { 
		perror("bind");
		exit(1);
	} 

	while(1) {
		cnt = recvfrom(sock, message, sizeof(message), 0, (struct sockaddr *) &mult, (socklen_t *)sizeof(mult));
		puts(message);
		sleep(2);
	}
}



	  
