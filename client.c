#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

#define PORT 5678
#define MAXBUF 1024

int *sockfd;

pthread_mutex_t mutex;
char usr[20], pass[20], err[MAXBUF];

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

void *receiveMessage(void *arg){
  int *conn = (int *)arg;
  char buf[MAXBUF];

  while(1){
    if(recvfrom(*conn, buf, MAXBUF, 0, NULL, NULL) < 0){
      continue;
    }
    else{
      printf("%s", buf);
      if(strcmp(buf, "1") == 0){
	printf("Nu esti inregistrat pe server!\n");
	
	close(*conn);
	free(conn);
	exit(1);
      }
      else if(strcmp(buf, "2") == 0){
	printf("Utilizatorul este deja conectat la server!\n");
	
	close(*conn);
	free(conn);
	exit(2);
      }
    }
  }

  close(*conn);
  free(conn);

  return NULL;
}

void *sendMessage(void *arg){
  int *conn = (int *)arg;
  char buf[MAXBUF];

  send(*conn, usr, 20, 0);
  send(*conn, pass, 20, 0);
  
  while(1){
    fgets(buf, MAXBUF, stdin);
    send(*conn, buf, MAXBUF, 0);
  }

  close(*conn);
  free(conn);

  return NULL;
}

int main(int argc, char *argv[]) {
  //char username[20], password[20];
  struct sockaddr_in local_addr, remote_addr;
  pthread_t thread_r, thread_w;
  pthread_attr_t attr;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, 1);
  pthread_mutex_init(&mutex, NULL);
  
  if(argc != 2){
    printf("Trebuie sa transmiti adresa IP!\n");
    exit(1);
  }

  sockfd = (int *)malloc(sizeof(int));

  if((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
    printf("Nu am putut crea soclul!\n");
    exit(1);
  }

  set_addr(&local_addr, NULL, INADDR_ANY, 0);
  
  if(bind(*sockfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) == -1){
    printf("Eroare la bind()!\n");
    exit(1);
  }

  if(set_addr(&remote_addr, argv[1], 0, PORT) == -1){
    printf("Eroare la adresa!\n");
    exit(1);
  }

  if((connect(*sockfd, (struct sockaddr *)&remote_addr, sizeof(remote_addr))) == -1){
    printf("Conectarea la server a esuat!\n");
    exit(1);
  }

  
  printf("Introdu numele de utilizator: ");

  scanf("%s", usr);

  printf("Introdu parola: ");

  scanf("%s", pass);

  if(pthread_create(&thread_r, &attr, receiveMessage, (void *)sockfd) != 0){
    printf("Eroare la crearea unui fir nou de executie!\n");
    exit(1);
  }

  if(pthread_create(&thread_w, &attr, sendMessage, (void *)sockfd) != 0){
    printf("Eroare la crearea unui fir nou de executie!\n");
    exit(1);
  }
  
  while(1){

  }

  exit(0);
}
