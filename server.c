#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <netdb.h>

#define BUFSZ 1024
#define PORT (short)5678

typedef struct{
  char usr[20], pass[20];
  int connected;
}client;

typedef struct{
  int socket;
  client c;
  pthread_t thread;
  struct sockaddr_in addr;
}connection;

client clients[20];

connection threads[20];

pthread_mutex_t mutex;
int threadNumber = 0;
int noClients = 0;
pthread_t thread;

int set_addr(struct sockaddr_in *addr, char *name, u_int32_t inaddr, short sin_port){
  struct hostent *h;
  memset((void *) addr, 0, sizeof(*addr));
  addr->sin_family = AF_INET;
  if(name != NULL){
    h = gethostbyname(name);
    if(h == NULL)
      return -1;
    addr->sin_addr.s_addr = *(u_int32_t *) h->h_addr_list[0];
  }
  else addr->sin_addr.s_addr = htonl(inaddr);
  addr->sin_port = htons(sin_port);
  return 0;
}

void addClient(char u[20], char p[20]){
  client *c;
  c = (client *)malloc(sizeof(client));
  strcpy(c->usr, u);
  strcpy(c->pass, p);
  clients[noClients].connected = 0;
  clients[noClients++] = *c;
}

void delete(int conn){
  for(int i = 0; i<threadNumber; i++){
    if(conn == threads[i].socket){
      for(int j = i+1; j<threadNumber; j++)
	threads[j-1] = threads[j];
    }
    threadNumber--;
    break;
  }
}

int validate(char usr[20], char pass[20]){
  for(int i = 0; i < noClients; i++){
    if((strcmp(clients[i].usr, usr) == 0) && (strcmp(clients[i].pass, pass) == 0)){
      return i;
    }
  }
  return -1;
}

void *ex4_proto(void *arg) {
  int r;
  int *conn = (int*)arg;
  char usr[20], pass[20], msg[BUFSZ];
  char buf[BUFSZ];

  recvfrom(*conn, usr, 20, 0, NULL, NULL);
  recvfrom(*conn, pass, 20, 0, NULL, NULL);

  if((r = validate(usr, pass)) < 0){
    char err[] = "1";
    send(*conn, err, strlen(err), 0);
  }
  else if(clients[r].connected == 1){
    char err[] = "2";
    send(*conn, err, strlen(err), 0);
  }
  else{
    clients[r].connected = 1;
      
    printf("S-a conectat %s!\n", usr);

    while(1){
      if((r = recvfrom(*conn, buf, BUFSZ, 0, NULL, NULL)) <= 0)
	break;
      else{
	strcpy(msg, usr);
	strcat(msg, ": ");
	strcat(msg, buf);
	fputs(msg, stdout);
	for(int i = 0; i < threadNumber; i++){
	  sendto(threads[i].socket, msg, BUFSZ, 0, (struct sockaddr *)&threads[i].addr, sizeof(threads[i].addr));
	}
      }
    }
  }

  clients[r].connected = 0;

  delete(*conn);
  close(*conn);
  free(conn);

  pthread_mutex_lock(&mutex);
  printf("Un client s-a deconectat!\n");
  pthread_mutex_unlock(&mutex);

  return NULL;
}

int main(int argc, char *argv[]){
  int sockfd, *connfd;
  struct sockaddr_in local_addr, remote_addr;
  socklen_t rlen;
  pthread_attr_t attr;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, 1);
  pthread_mutex_init(&mutex, NULL);

  if(argc > 1){
    printf("Server-ul nu primeste argumente!");
    exit(1);
  }

  addClient("laura", "alabalaportocala");
  addClient("seby", "beby");

  if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
    printf("Nu am putut crea soclul!\n");
    exit(1);
  }

  set_addr(&local_addr, NULL, INADDR_ANY, PORT);
  
  if(bind(sockfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) == -1){
    printf("Eroare la bind()\n!");
    exit(1);
  }

  if(listen(sockfd, 5) == -1) {
    printf("Eroare la listen()!\n");
    exit(1);
  }

  rlen = sizeof(remote_addr);

  while(1){
    connfd = (int *)malloc(sizeof(int));
    if(connfd == NULL){
      printf("Eroare la malloc()!\n");
      exit(1);
    }
    *connfd = accept(sockfd, (struct sockaddr *)&remote_addr, &rlen);

    if(*connfd < 0){
      printf("Eroare la accept()!\n");
      exit(1);
    }
    
    if(pthread_create(&threads[threadNumber].thread, &attr, ex4_proto, (void *)connfd) != 0){
      printf("Eroare la crearea unui fir nou de executie!\n");
      exit(1);
    }

    threads[threadNumber].socket = *connfd;
    threads[threadNumber].addr = remote_addr;

    pthread_mutex_lock(&mutex);
    threadNumber++;
    printf("%d conexiuni!\n", threadNumber);
    pthread_mutex_unlock(&mutex);
  }

  exit(0);
  
}
