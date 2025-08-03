/* ----------  client_jty.c  ---------- */
#define _GNU_SOURCE               /* memmem() */
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE   4096
#define NAME_SIZE    20

/* ───── 전역 ───── */
char name[NAME_SIZE] = "[Default]";

/* ───── 프로토타입 ───── */
static void *snd_thr(void *arg);
static void *rcv_thr(void *arg);
static void  send_file(int sock,const char *dest,const char *fname);
static void  die(const char *m){ perror(m); exit(1); }

/* ───── main ───── */
int main(int ac,char *av[])
{
    if(ac!=4){
        fprintf(stderr,"사용법 : %s <IP> <port> <ID>\n",av[0]);
        return 1;
    }
    strncpy(name,av[3],NAME_SIZE-1);

    int sock = socket(PF_INET,SOCK_STREAM,0);
    if(sock==-1) die("socket");

    struct sockaddr_in ad={0};
    ad.sin_family      = AF_INET;
    ad.sin_addr.s_addr = inet_addr(av[1]);
    ad.sin_port        = htons(atoi(av[2]));
    if(connect(sock,(struct sockaddr*)&ad,sizeof(ad))==-1) die("connect");

    /* 로그인 통지 */
    char first[64];
    snprintf(first,sizeof(first),"[%s:PASSWD]",name);
    write(sock,first,strlen(first));

    pthread_t t_snd,t_rcv;
    pthread_create(&t_snd,NULL,snd_thr,&sock);
    pthread_create(&t_rcv,NULL,rcv_thr,&sock);
    pthread_join(t_snd,NULL);
    pthread_join(t_rcv,NULL);
    close(sock);
    return 0;
}

/* ───── 송신 스레드 ───── */
static void *snd_thr(void *arg)
{
    int sock=*(int*)arg;
    char line[BUF_SIZE], out[BUF_SIZE+NAME_SIZE];

    fputs("형식: [ID]메시지   |  [ID]SENDFILE@파일명\n",stdout);

    while(fgets(line,sizeof(line),stdin)){
        /* 파일 전송 */
        if(line[0]=='[' && strstr(line,"]SENDFILE@")){
            char dest[NAME_SIZE],fname[256];
            if(sscanf(line,"[%[^]]]SENDFILE@%255s",dest,fname)==2)
                send_file(sock,dest,fname);
            continue;
        }
        /* 채팅 전송 */
        if(line[0]=='[')                      /* 지정 수신자 */
            write(sock,line,strlen(line));
        else{                                /* 전체 방송 */
            snprintf(out,sizeof(out),"[ALLMSG]%s",line);
            write(sock,out,strlen(out));
        }
    }
    return NULL;
}

/* ───── 파일 전송 서브루틴 ───── */
static void send_file(int sock,const char *dest,const char *fname)
{
    struct stat st;
    if(stat(fname,&st)==-1){ perror("stat"); return; }
    int fd=open(fname,O_RDONLY);
    if(fd==-1){ perror("open"); return; }

    char header[NAME_SIZE+512];
    snprintf(header,sizeof(header),
             "[%s]SENDFILE@%s@%ld\n",dest,fname,(long)st.st_size);
    write(sock,header,strlen(header));

    char buf[BUF_SIZE]; ssize_t n; off_t left=st.st_size;
    while(left>0 && (n=read(fd,buf,(left>BUF_SIZE)?BUF_SIZE:left))>0){
        write(sock,buf,n); left-=n;
    }
    close(fd);
    printf("send complete : %s → %s (%ld bytes)\n",
           fname,dest,(long)st.st_size);
}

/* ───── 수신 스레드 ───── */
static void *rcv_thr(void *arg)
{
    int sock=*(int*)arg;
    static int  file_mode=0;
    static long remain   =0;
    static FILE *fp      =NULL;
    static char fname[256];

    char buf[BUF_SIZE];

    while(1){
        ssize_t n=read(sock,buf,sizeof(buf));
        if(n<=0) break;

        size_t idx=0;
        while(idx<(size_t)n){
            if(file_mode){                    /* 파일 데이터 저장 */
                size_t chunk=(remain<(long)(n-idx))?remain:(n-idx);
                fwrite(buf+idx,1,chunk,fp);
                idx+=chunk; remain-=chunk;
                if(remain==0){
                    fclose(fp); file_mode=0;
                    printf("file saved : %s\n",fname);
                }
                continue;
            }

            /* 파일 헤더 검사 */
            char *mark=memmem(buf+idx,n-idx,"SENDFILE@",9);
            if(mark){
                char *nl=memchr(mark,'\n',(buf+n)-mark);
                if(!nl){ idx=n; break; }      /* 헤더가 완전하지 않음 → 다음 read 에서 처리 */
                sscanf(mark,"SENDFILE@%255[^@]@%ld",fname,&remain);
                fp=fopen(fname,"wb");
                if(!fp){ perror("fopen"); idx=(nl-buf)+1; continue; }
                file_mode=1;
                printf("receiving %s (%ld bytes)\n",fname,remain);
                idx=(nl-buf)+1;
                if(remain==0){ fclose(fp); file_mode=0; }
                continue;
            }

            /* 일반 채팅 출력 */
            fwrite(buf+idx,1,n-idx,stdout);
            break;
        }
    }
    return NULL;
}
