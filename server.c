#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include "structs.h"


#include <sys/ipc.h>
#include <sys/shm.h>

#define BUFLEN 512    
#define PORT_MULT 7000
#define GROUP_MULT "239.0.0.1"

void erro(char *s) {
    perror(s);
    exit(1);
}

int addUser (char name[100], char pass[100], float saldo, char bolsas[2][100]);

int parseFile (char file[BUFLEN]);

int deleteUser(char nome[100]);

int listUsers(char* text);

int initializeStructs();

int processClient();

int sub(int i, char splitedBuf[3][100]);

int unsub(int i, char splitedBuf[3][100]);

void feed(int i);

double buy(char name[100], int num, double price, int user);

double sell(char name[100], int num, double price, int user);

//shared memory
struct shared *shared_var;
int shmid;

int PORT_UDP;
int PORT_TCP; 

void *refresh(void *time){
	int *t = (int *) time;
	//printf("---%d---", *t);
	while(1){
		sleep(*t);
		int i, j;
		for (i = 0; i < 2; i++) {
			for (j = 0; j < 3; j++) {
				int r = rand() % 10;
				if (r < 5){
					shared_var->mkt[i].st[j].value += 0.01;
				
	//printf("%s---%f\n", mkt[i].st[j].name, mkt[i].st[j].value);
				}
				else {
					if (shared_var->mkt[i].st[j].value - 0.01 >= 0.01){
						shared_var->mkt[i].st[j].value -= 0.01;
					}
					//printf("%s---%f\n", mkt[i].st[j].name, mkt[i].st[j].value);
				}
			}
		}
	}
}

//structs sockets

//UDP
struct sockaddr_in si_minha, si_outra;
int s, recv_len;
socklen_t slen = sizeof(si_outra);
char buf[BUFLEN];

//TCP
int fd, client;
struct sockaddr_in addr, client_addr;
int client_addr_size;

//MULTICAST
struct sockaddr_in mult;
int addrlen,sock, cnt;

//threads refresh
pthread_t my_t;
int id; 

int main(int argc, char *argv[]) {
    id = 2;
    
    if (argc != 4) {
		printf("stock_server {PORTO_BOLSA} {PORTO_CONFIG} {ficheiro configuração}\n");
		exit(-1);
	} 
	PORT_TCP = atoi(argv[1]);
	PORT_UDP = atoi(argv[2]);
	
	shmid = shmget(6666, sizeof(struct shared), IPC_CREAT | 0766);
	
	if (shmid == -1){
		perror("Failed to create shm!");
		return(1);	
	}
	
	shared_var = shmat(shmid, NULL, 0);
	shared_var->time = 2;
	
    initializeStructs();
    parseFile(argv[3]);
	pthread_create(&my_t, NULL, refresh, &id);
	
	//*********************MULTICAST**************************
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("socket");
		exit(1);
	}
	
	int multicastTTL = 255;
	if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *) &multicastTTL, sizeof(multicastTTL)) < 0){
		perror("socket opt");
		exit(1);
	}
	
	bzero((char *)&addr, sizeof(addr));
	mult.sin_family = AF_INET;
	mult.sin_addr.s_addr = htonl(INADDR_ANY);
	mult.sin_port = htons(PORT_MULT);
	mult.sin_addr.s_addr = inet_addr(GROUP_MULT);
	addrlen = sizeof(mult);
	
	
	//***********************UDP**************************

    // Cria um socket para recepção de pacotes UDP
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        erro("Erro na criação do socket");
    }

    // Preenchimento da socket address structure
    si_minha.sin_family = AF_INET;
    si_minha.sin_port = htons(PORT_UDP);
    si_minha.sin_addr.s_addr = htonl(INADDR_ANY);

    // Associa o socket à informação de endereço
    if (bind(s, (struct sockaddr *) &si_minha, sizeof(si_minha)) == -1) {
        erro("Erro no bind");
    }
    
    //******************************TCP****************************************
 
	
	bzero((void *) &addr, sizeof(addr));
	
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(PORT_TCP);
	
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		erro("na funcao socket");
	if (bind(fd,(struct sockaddr*)&addr,sizeof(addr)) < 0)
		erro("na funcao bind");
	if(listen(fd, 5) < 0)
		erro("na funcao listen");
	client_addr_size = sizeof(client_addr);

  //*************************************************************************
	
    if (fork() == 0){
    	while(1) {
			while(waitpid(-1,NULL,WNOHANG)>0);
    		client = accept(fd,(struct sockaddr *)&client_addr,(socklen_t *)&client_addr_size);
        	if (client > 0) {
				if (fork() == 0) {    	
					processClient();
        			close(fd); 
        			close(client);  
        			exit(0);
 				}   
	
        	}
		}
	}

	int val = 0;
	int aux = 0;
    while(1){
    	char user[100], pass[100], auxt[100];
    	if (aux == 0 && val == 0) {
    		strcpy(auxt, "User: ");
    		sendto(s, auxt, strlen(auxt) + 1, 0, (struct sockaddr *) &si_outra, sizeof(si_outra));
    	}else if (aux == 1 && val == 0) {
    		strcpy(auxt, "Password: ");
    		sendto(s, auxt, strlen(auxt) + 1, 0, (struct sockaddr *) &si_outra, sizeof(si_outra));
    	}
		
        // Para ignorar o restante conteúdo (anterior do buffer)
        if((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_outra, (socklen_t *)&slen)) == -1) {
        erro("Erro no recvfrom");
        }	
        
        if (val == 0){
        	if (aux == 0) {
        		buf[recv_len-1] = '\0';
        		strcpy(user, buf);
        		aux++;
        	}else if (aux == 1) {
        		strcpy(pass, buf);
        		aux++;
        	} else if (aux == 2){
        		char mess[100];
        		if (strcmp(shared_var->adm.user, user) == 0 && strcmp(shared_var->adm.pass, pass) == 0){
        			printf("LOGIN_SUCESSFUL\n");
        			strcpy(mess, "LOGIN_SUCESSFUL\n");
        			val = 1;
        			aux++;
        		}
        		else {
        			printf("LOGIN FAILED\n");
        			strcpy(mess, "LOGIN FAILED\n");
        			aux = 0;
        		}	
        		sendto(s, mess, strlen(mess) + 1, 0, (struct sockaddr *) &si_outra, sizeof(si_outra));
        		if (val == 1) {
        			continue;
        		}
        	}
        	//aux=0;
        }
        //printf("val = %d", val);
        if (val == 1) {
        	buf[recv_len]='\0';
       		char *token = strtok(buf, " ");
        	char splitedBuf[5][100];
        	int aux = 0;
        	while (token != NULL) {
            	strcpy(splitedBuf[aux],token);
            	aux++;
            	token = strtok(NULL, " ");
        	}
	
        	if (strcmp(splitedBuf[0], "ADD_USER") == 0){
            	char *token = strtok(splitedBuf[3], "; ");
            	int aux = 0, ext = 0;	
            	char markets[2][100];
            	
            	while (token != NULL) {
            		int i;
            		ext = 0;
            		for (i = 0; i < 2; i++) {
            			if (strcmp(token, shared_var->mkt[i].name) == 0){
            				strcpy(markets[aux],token);
                			aux++;
                			ext++;
            			}            					
            		}
					if (ext == 0) {
						printf("ADD_USER FAILED\n");
						break;
					}
                	token = strtok(NULL, "; ");
            	}
            	if (ext != 0){
            		addUser(splitedBuf[1], splitedBuf[2], atof(splitedBuf[4]), markets);
        		}
        	} else if (strcmp(splitedBuf[0], "DEL") == 0){
        		splitedBuf[1][strlen(splitedBuf[1])-1] = '\0';
            	deleteUser(splitedBuf[1]);
        	} else if (strcmp(buf, "LIST\n") == 0){
        		char text[1024];
        		text[0] = '\0';
            	listUsers(text);
            	sendto(s, text, strlen(text) + 1, 0, (struct sockaddr *) &si_outra, sizeof(si_outra));
        	} else if (strcmp(splitedBuf[0], "REFRESH") == 0){
        		pthread_cancel(my_t);
        		id = atoi(splitedBuf[1]);
        		shared_var->time = id;
				printf("REFRESH TIME UPDATED TO: %d\n", shared_var->time);
        		pthread_create(&my_t, NULL, refresh, &id);
        	} else if (strcmp(buf, "QUIT\n") == 0){
        		printf("LOGGED OUT\n");
        		//close(s);
        		//break;
        		//pthread_exit(NULL);
        	} else if (strcmp(buf, "QUIT_SERVER\n") == 0){
        		pthread_cancel(my_t);
            	return 0;
        	} else {
        		char mes[100];
        		strcpy(mes, "SERVER: Command not found\n");
        		sendto(s, mes, strlen(mes) + 1, 0, (struct sockaddr *) &si_outra, sizeof(si_outra));
        	}
	
        }
        
    
    }

     
    return 0;
}


int processClient() {
	char buf[BUFLEN];
	char user[BUFLEN], pass[BUFLEN];
    read(client, user, BUFLEN);
    
    read(client, pass, BUFLEN);
    
    int i, aux = 0;
    for (i = 0; i < 10; i++) {
        if ((strcmp(user, shared_var->usr[i].user) == 0) && (strcmp(pass, shared_var->usr[i].pass) == 0)){
        	
        	strcpy(buf, "LOGIN SUCESSFUL\n\nMARKETS:\n");
        	int j;
        	for (j = 0; j < 2; j++) {
        		strcat(buf, shared_var->usr[i].markets[j]);
        		strcat(buf, "\n");
        	}
        	//puts(buf);
        	//write(client, buf, strlen(buf) + 1);
        	aux = 1;
        	break;
        }
    }
    if (aux == 0) {
    	printf("LOGIN FAILED\n");
    	return 0;
    }
    //puts(buf);

	//bzero(buf, strlen(buf));
	strcat(buf, "\n--------MENU--------\n");
	strcat(buf, "SUB [markets] \n");
	strcat(buf, "UNSUB [markets] \n");
	strcat(buf, "1 [START FEED] \n");
	strcat(buf, "0 [STOP FEED] \n");
	strcat(buf, "BUY [auction] [number] [price]\n");
	strcat(buf, "SELL [auction] [number] [price]\n");
	strcat(buf, "WALLET\n");
	strcat(buf, "QUIT\n");
	
	write(client, buf, strlen(buf) + 1);
	//puts(buf);
	bzero(buf, BUFLEN);
	pid_t subpid;
	int nread;
	while(1) {
		nread = read(client, buf, BUFLEN);
		buf[nread] = '\0';
    	char *token = strtok(buf, " \n");
        char splitedBuf[4][100];
		bzero(splitedBuf[0], 100);
		bzero(splitedBuf[1], 100);
		bzero(splitedBuf[2], 100);
        int aux = 0;
        while (token != NULL) {
            strcpy(splitedBuf[aux],token);
            aux++;
            token = strtok(NULL, " \n");
        }
        
		if(strcmp(splitedBuf[0],"1") == 0){
			write(client, GROUP_MULT, 100);
			if ((subpid = fork())== 0) {
				feed(i);
				exit(0);
			}
		}else if(strcmp(splitedBuf[0],"SUB") == 0){
			char mes[BUFLEN];
			if (sub(i, splitedBuf) == 0) {
				strcpy(mes, "MARKET(S) SUBSCRIBED\n");
			}else {
				strcpy(mes, "ERROR SUBSCRIBING MARKET(S)\n");
			}
			write(client, mes, BUFLEN);
			bzero(mes, BUFLEN);
		}else if(strcmp(splitedBuf[0],"UNSUB") == 0){
			char mes[BUFLEN];
			if (unsub(i, splitedBuf) == 0) {
				strcpy(mes, "MARKET(S) UNSUBSCRIBED\n");
			}else {
				strcpy(mes, "ERROR UNSUBSCRIBING MARKET(S)\n");
			}
			write(client, mes, BUFLEN);
			bzero(mes, BUFLEN);
		}else if (strcmp(splitedBuf[0],"0") == 0){
			kill(subpid, SIGKILL);
		}else if (strcmp(splitedBuf[0],"SELL") == 0){
			char mes[BUFLEN];
			double val;
			if ((val = sell(splitedBuf[1], atoi(splitedBuf[2]), atof(splitedBuf[3]), i)) == -1) {
				strcpy(mes, "ERROR SELLING STOCK");
				write(client, mes, BUFLEN);
			}else {
				sprintf(mes, "AUCTION SOLD BY %f\n", val);
				write(client, mes, BUFLEN);
			}
			bzero(mes, BUFLEN);
		}else if (strcmp(splitedBuf[0],"BUY") == 0){
			char mes[BUFLEN];
			double val;
			if ((val = buy(splitedBuf[1], atoi(splitedBuf[2]), atof(splitedBuf[3]), i)) == -1) {
				strcpy(mes, "ERROR BUYING STOCK");
				write(client, mes, BUFLEN);
			}else {
				sprintf(mes, "AUCTION BOOUGHT BY %f\n", val);
				write(client, mes, BUFLEN);
			}
			bzero(mes, BUFLEN);
		}else if (strcmp(splitedBuf[0],"WALLET") == 0) {
			int l;
			char mes[BUFLEN], auxmes[BUFLEN];
			sprintf(mes, "Saldo: %f\n", shared_var->usr[i].saldo);

			for (l = 0; l < 6; l++){
				if (shared_var->usr[i].wal[l].name[0] != '\0') {
					sprintf(auxmes, "%s: %d\n", shared_var->usr[i].wal[l].name, shared_var->usr[i].wal[l].num);
					strcat(mes, auxmes);
					bzero(auxmes, BUFLEN);
				}
			}
			write(client, mes, BUFLEN);
			bzero(mes, BUFLEN);
		}else if (strcmp(splitedBuf[0],"QUIT") == 0) {
			//kill(subpid, SIGKILL);
			break;
		}else {
			printf("COMMAND NOT FOUND\n");
		}
	}
	return 0;
}

double sell(char name[100], int num, double price, int user) {
	int i, j, k;
	for (i = 0; i < 2; i++) {
		for (j = 0; j < 3; j++) {
			if (strcmp(name, shared_var->mkt[i].st[j].name) == 0 && price <= shared_var->mkt[i].st[j].value) {
				for (k = 0; k < 6; k++){
					if ((strcmp(shared_var->usr[user].wal[k].name, name) == 0) && (shared_var->usr[user].saldo >= shared_var->mkt[i].st[j].value) && (shared_var->usr[user].wal[k].num >= num)) {
						shared_var->usr[user].saldo += shared_var->mkt[i].st[j].value;
						shared_var->usr[user].wal[k].num -= num;
						return shared_var->mkt[i].st[j].value;
					}
				}
			}
		}
	}
	return -1;
} 

double buy(char name[100], int num, double price, int user) {
	int i, j, k;
	for (i = 0; i < 2; i++) {
		for (j = 0; j < 3; j++) {
			if (strcmp(name, shared_var->mkt[i].st[j].name) == 0 && price >= shared_var->mkt[i].st[j].value) {
				for (k = 0; k < 6; k++){
					if ((strcmp(shared_var->usr[user].wal[k].name, name) == 0) && (shared_var->usr[user].saldo >= shared_var->mkt[i].st[j].value)) {
						shared_var->usr[user].saldo -= shared_var->mkt[i].st[j].value;
						shared_var->usr[user].wal[k].num += num;
						return shared_var->mkt[i].st[j].value;
					}
				}
				for (k = 0; k < 6; k++){
					if ((shared_var->usr[user].wal[k].name[0] == '\0') && (shared_var->usr[user].saldo >= shared_var->mkt[i].st[j].value)) {
						shared_var->usr[user].saldo -= shared_var->mkt[i].st[j].value;
						strcpy(shared_var->usr[user].wal[k].name, name);
						shared_var->usr[user].wal[k].num = num;
						return shared_var->mkt[i].st[j].value;
					}
				}
			}
		}
		
	}
	return -1;
}

int unsub(int i, char splitedBuf[3][100]) {
	int j, l;
	int count = 0;
	
	for(j = 0; j < 2; j++){
		for (l = 0; l < 2; l++) {
			if(strcmp(splitedBuf[j+1], shared_var->usr[i].markets[l]) == 0){
				//printf("---%s-----%s-----\n", splitedBuf[j+1], shared_var->usr[i].markets[l]);
				int k;
				for (k = 0; k < 2; k++) {
					if (strcmp(shared_var->usr[i].sub[k], splitedBuf[j + 1]) == 0){
						bzero(shared_var->usr[i].sub[k], 100);
						break;
					}
				}
				count++;
			}
		}
	}
	if (count == 0) {
		return 1;
	}
	return 0;
	//printf("----%s-----%s-----\n", shared_var->usr[i].sub[0], shared_var->usr[i].sub[1]);
	
}


int sub(int i, char splitedBuf[3][100]){
	//verificar se esta registado no mercado
	int j, l;
	int count = 0;
	
	for(j = 0; j < 2; j++){
		for (l = 0; l < 2; l++) {
			if(strcmp(splitedBuf[j+1], shared_var->usr[i].markets[l]) == 0){
				int k;
				for (k = 0; k < 2; k++) {
					if (strcmp(shared_var->usr[i].sub[k], "\0") == 0){
						strcpy(shared_var->usr[i].sub[k], shared_var->usr[i].markets[l]);
						break;
					}
				}
				count++;
			}
		}
	}
	if (count == 0) {
		return 1;
	}
	return 0;
	//if (strcmp(shared_var->usr[i].sub[0], shared_var->usr[i].sub[1]) == 0) {
	//	bzero(shared_var->usr[i].sub[1], 100);
	//}
}

void feed(int i) {
	write(client, GROUP_MULT, strlen(GROUP_MULT) + 1);
	char message[BUFLEN];
	while (1) {
		bzero(message, BUFLEN);
		int j, k, l;
		char aux[100];
		strcpy(aux, "atum");
		for (j = 0; j < 2; j++) {
			for (l = 0; l < 2; l++){

				if(strcmp(shared_var->usr[i].sub[j],shared_var->mkt[l].name)==0) {
					//printf("---%s---%s---\n", shared_var->usr[i].);
					if (strcmp(aux, shared_var->mkt[l].name) != 0){
						strcpy(aux, shared_var->mkt[l].name);
						for (k = 0; k < 3; k++) {
							char aux[BUFLEN];
							sprintf(aux, "%s: %0.3f\n", shared_var->mkt[l].st[k].name, shared_var->mkt[l].st[k].value);
							strcat(message, aux);
						}
					}
				}
			}
		}
		strcat(message, "\n");
		cnt = sendto(sock, message, sizeof(message), 0, (struct sockaddr *) &mult, addrlen);
		if (cnt < 0) {
			perror("sendto");
			exit(1);
		}
		sleep(shared_var->time);
	}
	//puts(buf);
	//write(client, buf, strlen(buf) + 1);
	bzero(buf, BUFLEN);
}


int initializeStructs(){
    int i, j, l;
    for (i = 0; i < 2; i++) {
        shared_var->mkt[i].name[0] = '\0';
        for (l = 0; l < 3; l++) {
            shared_var->mkt[i].st[l].name[0] = '\0';
        }
    }
    for (i = 0; i < 10; i++){
        shared_var->usr[i].user[0] = '\0';
        shared_var->usr[i].pass[0] = '\0';
        for (l = 0; l < 2; l++){
            shared_var->usr[i].markets[l][0] = '\0';
            shared_var->usr[i].sub[l][0] = '\0';
        }
		for (j = 0; j < 6; j++) {
			shared_var->usr[i].wal[j].name[0] = '\0';
			shared_var->usr[i].wal[j].num = 0;
		}
    }
    return 0;
}

int parseFile (char file[BUFLEN]){
    FILE *fp;
    char line[1000];
    fp = fopen(file, "r");
    if (!fp)
        return 1;

    int numusers = 0;

    int i = 0;
    int l = 0;
    int num = 0;
    int aux = 0;
    while (fgets(line, 500, fp) != NULL) {
        char market[100];

        if (i == 0) {
            char *token = strtok(line, "/");
            int j = 0;
            while (token != NULL) {
                if (j == 0) {
                    strcpy(shared_var->adm.user, token);
                    j++;
                } else {
                    strcpy(shared_var->adm.pass, token);
                }
                token = strtok(NULL, "/");
            }
        } else if (i == 1) {
            numusers = atoi(line);
        } else if (i < numusers + 2) {
            char *token = strtok(line, ";");
            int j = 0;
            while (token != NULL) {
                if (j == 0) {
                    strcpy(shared_var->usr[num].user, token);
                } else if (j == 1){
                    strcpy(shared_var->usr[num].pass, token);
                } else {
                    shared_var->usr[num].saldo = atof(token);
                }
                j++;
                token = strtok(NULL, ";");
            }
            num++;
        } else {
            char *token = strtok(line, ";");
            int j = 0;

            while (token != NULL) {
                if (j == 0) {
                    if (aux == 0 || strcmp(token, market) == 0) {
                        strcpy(shared_var->mkt[0].name, token);
                        strcpy(market, token);
                        l = 0;
                        aux++;
                    }
                    else {
                        strcpy(shared_var->mkt[1].name, token);
                        l = 1;
                    }
                } else if (j == 1) {
                    int k;
                    for (k = 0; k < 3; k++) {
                        if (shared_var->mkt[l].st[k].name[0] == '\0') {
                            strcpy(shared_var->mkt[l].st[k].name, token);
                            token = strtok(NULL, ";");
                            shared_var->mkt[l].st[k].value = atoi(token);
                            break;
                        }
                    }
                }
                token = strtok(NULL, ";");
                j++;
            }
        }
        i++;
    }
    
    int h;
    for (h = 0; h < numusers; h++) {
    	strcpy(shared_var->usr[h].markets[0], shared_var->mkt[0].name);
    	strcpy(shared_var->usr[h].markets[1], shared_var->mkt[1].name);
    	//strcpy(shared_var->usr[h].sub[0], shared_var->mkt[0].name);
    	//strcpy(shared_var->usr[h].sub[1], shared_var->mkt[1].name);
    }
    
    fclose(fp);
    return 0;
}


int addUser (char name[100], char pass[100], float saldo, char bolsas[2][100]) {

    int i;
    for (i = 0; i < 10; i++) {
        if (strcmp(shared_var->usr[i].user, name) == 0 && strcmp(shared_var->usr[i].pass, pass) == 0) {
            shared_var->usr[i].saldo = saldo;
            int j;
            int k;
            for (j = 0; j < 2; j++) {
                for (k = 0; k < 2; k++) {
                    if (strcmp(bolsas[j], shared_var->mkt[k].name) == 0) {
                        int l;
                        for (l = 0; l < 2; l++) {
                            if (shared_var->usr[i].markets[l][0] == '\0' )  {
                                strcpy(shared_var->usr[i].markets[l], bolsas[k]);
                                break;
                            }
                        }
                    }
                }
            }
            break;
        }
        
        
        if (shared_var->usr[i].user[0] == '\0') {
            strcpy(shared_var->usr[i].user, name);
            strcpy(shared_var->usr[i].pass, pass);
            shared_var->usr[i].saldo = saldo;
            int j;
            int k;
            for (j = 0; j < 2; j++) {
                for (k = 0; k < 2; k++) {
                    if (strcmp(bolsas[j], shared_var->mkt[k].name) == 0) {
                        int l;
                        for (l = 0; l < 2; l++) {
                            if (shared_var->usr[i].markets[l][0] == '\0' && strcmp(bolsas[l],"\0") != 0) {
                                strcpy(shared_var->usr[i].markets[l], bolsas[k]);
                                break;
                            }
                        }
                    }
                }
            }
            break;
        }

    }
    
    char buf[BUFLEN];
    strcpy(buf, "USER CREATED\n");
    puts(buf);
    sendto(s, buf, strlen(buf) + 1, 0, (struct sockaddr *) &si_outra, sizeof(si_outra));
    
    return 0;
}

int deleteUser(char nome[100]){
    int i;
    for (i = 0; i < 10; i++){
        if (strcmp(nome, shared_var->usr[i].user) == 0){
            shared_var->usr[i].user[0] = '\0';
            shared_var->usr[i].pass[0] = '\0';
            shared_var->usr[i].saldo = 0;
            shared_var->usr[i].markets[0][0] = '\0';
            shared_var->usr[i].markets[1][0] = '\0';
        }
    }
    printf("A User as been deleted\n");
    return 0;
}

int listUsers(char* text){
    int i;
    char buf[1024];

    for (i = 0; i < 10; i++) {
        if (shared_var->usr[i].user[0] != '\0') {
            sprintf(buf, "Nome: %s, Password: %s, Saldo: %f\n", shared_var->usr[i].user, shared_var->usr[i].pass, shared_var->usr[i].saldo);
            strcat(text, buf);
        }
    }
    return 0;
}
