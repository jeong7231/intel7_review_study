/* 서울기술 교육센터 IoT */
/* author : HJY + DB 개량 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>

#define BUF_SIZE 100
#define NAME_SIZE 20
#define ARR_CNT 5

// 송신 및 수신 스레드 함수 선언
void * send_msg(void * arg);
void * recv_msg(void * arg);
void error_handling(char * msg);

// 전역 변수 (클라이언트 이름 및 송신 메시지 버퍼)
char name[NAME_SIZE] = "[Default]";
char msg[BUF_SIZE];

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread;
    void * thread_return;

    // 인자 확인: IP, 포트, 이름
    if(argc != 4) {
        printf("Usage : %s <IP> <port> <name>\n", argv[0]);
        exit(1);
    }

    sprintf(name, "%s", argv[3]);

    // 비밀번호 입력
    char passwd[NAME_SIZE] = {0};
    printf("Password for [%s]: ", name);
    scanf("%s", passwd);

    // 소켓 생성
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1)
        error_handling("socket() error");

    // 서버 주소 설정
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    // 서버 연결 요청
    if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    // 로그인 정보 전송 (id:pw)
    sprintf(msg, "%s:%s", name, passwd);
    write(sock, msg, strlen(msg));

    // 메시지 수신 및 송신용 스레드 생성
    pthread_create(&rcv_thread, NULL, recv_msg, (void *)&sock);
    pthread_create(&snd_thread, NULL, send_msg, (void *)&sock);

    // 송신 스레드 종료 시 메인도 종료
    pthread_join(snd_thread, &thread_return);
    close(sock);
    return 0;
}

// 메시지 송신 스레드
void * send_msg(void * arg)
{
    int *sock = (int *)arg;
    int ret;
    fd_set initset, newset;
    struct timeval tv;
    char name_msg[NAME_SIZE + BUF_SIZE + 2];

    FD_ZERO(&initset);
    FD_SET(STDIN_FILENO, &initset);

    fputs("Input a message! [ID]msg (Default ID:ALLMSG)\n", stdout);
    while(1) {
        memset(msg, 0, sizeof(msg));
        name_msg[0] = '\0';
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        newset = initset;
        ret = select(STDIN_FILENO + 1, &newset, NULL, NULL, &tv);

        if(FD_ISSET(STDIN_FILENO, &newset))
        {
            fgets(msg, BUF_SIZE, stdin);

            if(!strncmp(msg, "quit\n", 5)) {
                *sock = -1;
                return NULL;
            }

            //메시지 유효성 검사
            if (strspn(msg, "\r\n") == strlen(msg)) continue; // 줄바꿈만 있을 경우 무시

            if(msg[0] != '[') {
                strcat(name_msg, "[ALLMSG]");
                strcat(name_msg, msg);
            } else {
                if (strlen(msg) < 5 || strchr(msg, ']') == NULL) continue; // 잘못된 형식 무시
                strcpy(name_msg, msg);
            }

            if(write(*sock, name_msg, strlen(name_msg)) <= 0) {
                *sock = -1;
                return NULL;
            }
        }

        if(ret == 0 && *sock == -1) return NULL;
    }
}


// 메시지 수신 스레드
void * recv_msg(void * arg)
{
    int * sock = (int *)arg;
    char name_msg[NAME_SIZE + BUF_SIZE + 1];
    int str_len;

    while(1) {
        memset(name_msg, 0x0, sizeof(name_msg));
        str_len = read(*sock, name_msg, NAME_SIZE + BUF_SIZE);
        if(str_len <= 0) {
            *sock = -1;
            return NULL;
        }

        name_msg[str_len] = '\0';

        // 빈 문자열, 공백, 줄바꿈만 있는 메시지 무시
        if (strspn(name_msg, "\r\n") == str_len) continue;

        // 삭제, 비밀번호변경 메시지 수신 시 종료 처리
        if (strstr(name_msg, "You have been deleted") != NULL ||
            strstr(name_msg, "Your password has been changed") != NULL) {
            fputs(name_msg, stdout);
            fflush(stdout);
            *sock = -1;
            close(*sock);
            exit(0);  // 즉시 종료
        }

        fputs(name_msg, stdout);
        fflush(stdout);
    }
}

// 에러 처리 함수
void error_handling(char * msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
