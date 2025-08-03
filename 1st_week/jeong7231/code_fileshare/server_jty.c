/* ----------  server_jty.c  ---------- */
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE   4096
#define MAX_CLNT     50
#define ID_SIZE      10

/* ───── 구조체 ───── */
typedef struct{
    int  fd;
    char id[ID_SIZE];
} CLNT;

/* ───── 전역 ───── */
CLNT clnt[MAX_CLNT];
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

/* ───── 헬퍼 ───── */
static int  id2fd(const char *id){
    for(int i=0;i<MAX_CLNT;i++)
        if(clnt[i].fd>0 && !strcmp(clnt[i].id,id))
            return clnt[i].fd;
    return -1;
}
static void relay_file(int src,int dst,long size){
    char buf[BUF_SIZE]; ssize_t n; long left=size;
    while(left>0 && (n=read(src,buf,(left>BUF_SIZE)?BUF_SIZE:left))>0){
        write(dst,buf,n); left-=n;
    }
}
static void log_file(const char *msg){ fputs(msg,stdout); }

/* ───── 스레드 ───── */
static void *clnt_thr(void *arg)
{
    int cfd=(long)arg;
    char buf[BUF_SIZE];

    /* 최초 로그인 패킷: [ID:PASSWD] */
    ssize_t n=read(cfd,buf,sizeof(buf));
    char my_id[ID_SIZE]="";
    if(n>0) sscanf(buf,"[%9[^:]]",my_id);

    pthread_mutex_lock(&mtx);
    for(int i=0;i<MAX_CLNT;i++)
        if(clnt[i].fd==0){ clnt[i].fd=cfd; strncpy(clnt[i].id,my_id,ID_SIZE-1); break; }
    pthread_mutex_unlock(&mtx);

    printf("login : %s (fd %d)\n",my_id,cfd);

    while((n=read(cfd,buf,sizeof(buf)))>0){
        buf[n]=0;

        /* --------- 로그 출력 --------- */
        printf("recv (%s) : %s",my_id,buf);

        /* --------- 대상 ID 추출 --------- */
        char dest[ID_SIZE]="";
        if(sscanf(buf,"[%9[^]]]",dest)!=1) continue;

        /* --------- 파일 전송 --------- */
        char *mk=strstr(buf,"]SENDFILE@");
        if(mk){
            char fname[256]; long fsize;
            if(sscanf(mk+1,"SENDFILE@%255[^@]@%ld",fname,&fsize)!=2) continue;

            int dfd=id2fd(dest);
            if(dfd<0) continue;

            /* 헤더 중계 */
            write(dfd,buf,(mk-buf)+1);          /* '['~']' */
            write(dfd,mk+1,strlen(mk+1));       /* 'SENDFILE@...' */

            /* 이미 받아온 파일 바이트 중계 */
            char *nl=strchr(mk,'\n');
            size_t sent=0;
            if(nl && (buf+n)>(nl+1)){
                sent=(buf+n)-(nl+1);
                write(dfd,nl+1,sent);
            }
            long left=fsize-(long)sent;
            if(left>0) relay_file(cfd,dfd,left);

            printf("relay file %s (%ld bytes) : %s → %s\n",
                   fname,fsize,my_id,dest);
            continue;
        }

        /* --------- 일반 채팅 --------- */
        pthread_mutex_lock(&mtx);
        if(!strcmp(dest,"ALLMSG")){
            for(int i=0;i<MAX_CLNT;i++)
                if(clnt[i].fd>0)
                    write(clnt[i].fd,buf,n);
        }else if(!strcmp(dest,"IDLIST")){
            /* IDLIST 요청 처리 예시 */
            char idlist[ID_SIZE*MAX_CLNT]={0};
            for(int i=0;i<MAX_CLNT;i++)
                if(clnt[i].fd>0){
                    strcat(idlist,clnt[i].id);
                    strcat(idlist," ");
                }
            strcat(idlist,"\n");
            write(cfd,idlist,strlen(idlist));
        }else if(!strcmp(dest,"GETTIME")){
            char tbuf[64]; time_t tt=time(NULL);
            strftime(tbuf,sizeof(tbuf),"[%F %T]\n",localtime(&tt));
            write(cfd,tbuf,strlen(tbuf));
        }else{
            int dfd=id2fd(dest);
            if(dfd>0) write(dfd,buf,n);
        }
        pthread_mutex_unlock(&mtx);

        /* 로그 파일(콘솔) 기록 */
        log_file(buf);
    }

    /* --------- 종료 처리 --------- */
    pthread_mutex_lock(&mtx);
    for(int i=0;i<MAX_CLNT;i++) if(clnt[i].fd==cfd){ clnt[i].fd=0; break; }
    pthread_mutex_unlock(&mtx);
    printf("logout : %s (fd %d)\n",my_id,cfd);
    close(cfd);
    return NULL;
}

/* ───── main ───── */
int main(int ac,char *av[])
{
    if(ac!=2){ puts("사용법 : ./server <port>"); return 1; }

    int srv=socket(PF_INET,SOCK_STREAM,0);
    struct sockaddr_in ad={0};
    ad.sin_family=AF_INET; ad.sin_addr.s_addr=htonl(INADDR_ANY);
    ad.sin_port=htons(atoi(av[1]));
    bind(srv,(struct sockaddr*)&ad,sizeof(ad));
    listen(srv,5);

    puts("Server Start!");

    while(1){
        int cfd=accept(srv,NULL,NULL);
        pthread_t t; pthread_create(&t,NULL,clnt_thr,(void*)(long)cfd);
        pthread_detach(t);
    }
}
