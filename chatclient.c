#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define PORT 10140

void errorConn(int sock){
  close(sock);
  exit(1);
}


void readAccept(int sock){
  char rbuf[32];
  int nbytes;
  memset(rbuf,0,sizeof(rbuf));
  read(sock,rbuf,sizeof(rbuf));
  printf("%s",rbuf);
  if(strcmp(rbuf,"REQUEST ACCEPTED\n") != 0){
    printf("REQUEST REJECTED\n");
    errorConn(sock);
  }else{
    printf("connected to the server\n");
  }
  return;
}

void sendUsername(int sock,char uname[]){
  char sbuf[64];
  sprintf(sbuf,"%s\n",uname);
  write(sock,sbuf,strlen(sbuf));
  return;
}

void readUserReg(int sock){
  char rbuf[32];
  memset(rbuf,0,sizeof(rbuf));
  read(sock,rbuf,sizeof(rbuf));
  if(strcmp(rbuf,"USERNAME REGISTERED\n") != 0){
    printf("USERNAME REJECTED\n");
    errorConn(sock);
  }else{
    printf("your user name has been registered\n");
  }
  return;
}


void send_data(int sock){
 char sbuf[1024];
 if(feof(stdin)){
   close(sock);
   exit(0);
 }else{
   fgets(sbuf,sizeof(sbuf),stdin);
   write(sock,sbuf,strlen(sbuf));
   return;
 }
}

int main(int argc,char **argv) {
  int sock;
  struct sockaddr_in host;
  struct hostent *hp;
  int nbytes;
  char rbuf[1024];
  char username[32];
  fd_set rfds;
  struct timeval tv;
  if (argc != 3) {
    fprintf(stderr,"Usage: %s hostname username\n",argv[0]);
    exit(1);
  }
  strcpy(username,argv[2]);
  if ( ( sock = socket(AF_INET,SOCK_STREAM,0) ) < 0) {
    perror("socket");
    exit(1);
  }
  bzero(&host,sizeof(host));
  host.sin_family=AF_INET;
  host.sin_port=htons(PORT);

  if ( ( hp = gethostbyname(argv[1]) ) == NULL ) {
    fprintf(stderr,"unknown host %s\n",argv[1]);
    exit(1);
  }
  bcopy(hp->h_addr,&host.sin_addr,hp->h_length);
  if(connect(sock,(struct sockaddr *)&host,sizeof(host)) < 0){
    perror("could not connect to the host");
    close(sock);
    exit(1);
  }
  readAccept(sock);
  sendUsername(sock,username);
  readUserReg(sock);

  memset(rbuf,0,sizeof(rbuf));
  do{
    FD_ZERO(&rfds);
    FD_SET(0,&rfds);
    FD_SET(sock,&rfds);

    tv.tv_sec = 1;
    tv.tv_usec = 0;

    if(select(sock+1,&rfds,NULL,NULL,&tv)>0) {
      if(FD_ISSET(0,&rfds)) {
        send_data(sock);
      }
      if(FD_ISSET(sock,&rfds)) {
        nbytes = read(sock,rbuf,sizeof(rbuf));
        if(nbytes == 0) break;
        printf("%s",rbuf);
        memset(rbuf,0,sizeof(rbuf));
      }
    }
  } while(1); /* 繰り返す */
  close(sock);
}
