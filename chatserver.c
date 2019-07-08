#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define PORT 10140
#define MAXCLIENTS 5

struct clients{
  char name[32];
  int sock;
  char ipaddr[32];
};

struct clients users[MAXCLIENTS];

void sendAccept(int sock){
  char sbuf[32];
  strcpy(sbuf,"REQUEST ACCEPTED\n");
  write(sock,sbuf,strlen(sbuf));
  return;
}

void sendReject(int sock){
  char sbuf[32];
  strcpy(sbuf,"REQUEST REJECTED\n");
  write(sock,sbuf,strlen(sbuf));
  close(sock);
  return;
}

int checkUsers(int sock,int client_num){
  char rbuf[32];
  char sbuf[32];
  char *p;
  int i;
  memset(rbuf,0,sizeof(rbuf));
  read(sock,rbuf,sizeof(rbuf));
  p = strchr(rbuf, '\n');
  if(p != NULL) {
    *p = '\0';
  }
  for(i = 0 ; i < client_num ; i++){
    if(strcmp(users[i].name,rbuf) == 0){
      strcpy(sbuf,"USERNAME REJECTED\n");
      write(sock,sbuf,strlen(sbuf));
      close(sock);
      printf("REJECTED USER IS %s\n",rbuf);
      return 0;
    }
  }
  strcpy(users[client_num].name,rbuf);
  strcpy(sbuf,"USERNAME REGISTERED\n");
  write(sock,sbuf,strlen(sbuf));
  printf("REGISTERED USER IS %s\n",rbuf);
  return 1;
}

void relocUsers(int i, int client_num){
  int last = client_num-1;
  if(i == last){
    return;
  }
  strcpy(users[i].name,users[last].name);
  strcpy(users[i].ipaddr,users[last].ipaddr);
  users[i].sock = users[last].sock;
}

void returnList(int sock,int client_num){
  char sbuf[1024];
  memset(sbuf,0,sizeof(sbuf));
  sprintf(sbuf,"%s\n",users[0].name);
  for(int i = 1 ; i < client_num ; i++){
    sprintf(sbuf,"%s%s\n",sbuf,users[i].name);
  }
  write(sock,sbuf,strlen(sbuf));
  return;
}

void sendSecret(char rbuf[], int client_num, struct clients fromUser){
  char *ptr = strtok(rbuf," ");
  char *name,*message;
  int sock = -1;
  char sbuf[1024];
  ptr = strtok(NULL," ");
  name = ptr;
  ptr = strtok(NULL,"\n");
  message = ptr;
  for(int i = 0 ; i < client_num ; i++){
    if(strcmp(name,users[i].name) == 0) sock = users[i].sock;
  }
  if(sock == -1){
    write(fromUser.sock,"could not send the secret message\n",strlen("could not send the secret message\n"));
  }else{
    memset(sbuf,0,sizeof(sbuf));
    sprintf(sbuf,"secret message from %s > %s\n",fromUser.name,message);
    write(sock,sbuf,strlen(sbuf));
    write(fromUser.sock,"successfully sent\n",strlen("successfully sent\n"));
  }
  return;
}

int main(int argc,char **argv) {
  int sock,csock;
  int MAX;
  int client_num = 0;
  struct sockaddr_in svr;
  struct sockaddr_in clt;
  struct hostent *cp;
  int clen,nbytes,reuse;
  char rbuf[1024];
  char sendbuf[1024];
  fd_set rfds;
  struct timeval tv;
//----時間取得用-------
  time_t timer;
  struct tm *local;
/* ソケットの生成 */
  if ((sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))<0) {
    perror("socket");
    exit(1);
  }
  /* ソケットアドレス再利用の指定 */
  reuse=1;
  if(setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse))<0) {
    perror("setsockopt");
    exit(1);
  }
  /* client 受付用ソケットの情報設定 */
  bzero(&svr,sizeof(svr));
  svr.sin_family=AF_INET;
  svr.sin_addr.s_addr=htonl(INADDR_ANY); /* 受付側の IP アドレスは任意 */
  svr.sin_port=htons(PORT); /* ポート番号 10140 を介して受け付ける */
  /* ソケットにソケットアドレスを割り当てる */
  if(bind(sock,(struct sockaddr *)&svr,sizeof(svr))<0) {
    perror("bind");
    exit(1);
  }
  /* 待ち受けクライアント数の設定 */
  if (listen(sock,MAXCLIENTS)<0) { /* 待ち受け数に 5 を指定 */
    perror("listen");
    exit(1);
  }
  do {
    FD_ZERO(&rfds);
    FD_SET(sock,&rfds);
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    MAX = sock;
    for(int i = 0 ; i < client_num ; i++){
      if(MAX < users[i].sock) MAX = users[i].sock;
      FD_SET(users[i].sock,&rfds);
    }
    if(select(MAX+1,&rfds,NULL,NULL,&tv)>0){
      if(FD_ISSET(sock,&rfds)) {
        clen = sizeof(clt);
        if ( ( csock = accept(sock,(struct sockaddr *)&clt,&clen) ) <0 ) {
          perror("accept");
          exit(2);
        }
        cp = gethostbyaddr((char *)&clt.sin_addr,sizeof(struct in_addr),AF_INET);
        printf("[%s]\n",cp->h_name);
        if(client_num < MAXCLIENTS){
          sendAccept(csock);
          users[client_num].sock = csock;
        }else{
          sendReject(csock);
          continue;
        }
        client_num += checkUsers(csock,client_num);
        strcpy(users[client_num-1].ipaddr,inet_ntoa(clt.sin_addr));
      }
      for(int i = 0 ; i < client_num ; i++){
        if(FD_ISSET(users[i].sock,&rfds)){
          memset(rbuf,0,sizeof(rbuf));
          memset(sendbuf,0,sizeof(sendbuf));
          timer = time(NULL);
          local = localtime(&timer);
          nbytes = read(users[i].sock,rbuf,sizeof(rbuf));
          if(nbytes == 0){
            close(users[i].sock);
            FD_CLR(users[i].sock,&rfds);
            printf("USER %s IS DISCONNECTED\n",users[i].name);
            relocUsers(i,client_num);
            client_num--;
          }
          if(nbytes > 0){
            if(strncmp(rbuf,"/list\n",6) == 0){
              returnList(users[i].sock,client_num);
              continue;
            }else if(strncmp(rbuf,"/send ",6) == 0){
              sendSecret(rbuf,client_num,users[i]);
              continue;
            }
            for(int j = 0 ; j < client_num ; j++){
              sprintf(sendbuf,"%s[%s] %d:%d:%d> %s",users[i].name,users[i].ipaddr,local->tm_hour,local->tm_min,local->tm_sec,rbuf);
              write(users[j].sock,sendbuf,strlen(sendbuf));
            }
          }
        }
      }
    }
  }while(1); /* 次の接続要求を繰り返し受け付ける */
}
