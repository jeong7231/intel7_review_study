/* 서울기술교육센터 AIoT */
/* author : HJY */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <mysql/mysql.h>
#include <errno.h>

#define BUF_SIZE 100
#define MAX_CLNT 32
#define ID_SIZE 20
#define ARR_CNT 5
// 클라이언트 정보 구조체
typedef struct {
    int fd;
    char ip[20];
    char id[ID_SIZE];
} CLIENT_INFO;
// 메시지 정보 구조체
typedef struct {
    int fd;
    char *from;
    char *to;
    char *msg;
    int len;
} MSG_INFO;

void * clnt_connection(void * arg); // 클라이언트 쓰레드 함수
void send_msg(MSG_INFO * msg_info); // 메시지 브로드케스트/ 귓말
void error_handling(char * msg);    // 에러 출력
void log_file(char * msgstr);       // 서버 콘솔 로그 출력

int clnt_cnt = 0;                   // 접속중인 클라이언트 개수
pthread_mutex_t mutx;               // 동기화를 위한 뮤텍스
CLIENT_INFO client_info[MAX_CLNT];  // 클라이언트 배열
MYSQL *conn;                        // MySQL 핸들

// DB에 적재되어 있는 아이디/패스워드에 대한 검증
// 기존에 텍스트 파일에 있던 계정 정보를 DB에 넣어뒀으니 DB안에 클라이언트 계정정보를 확인하기 위해 작성
int db_login_check(const char *id, const char *pw) {
    char query[256];
    MYSQL_RES *res;
    sprintf(query, "SELECT * FROM users WHERE id='%s' AND pw='%s'", id, pw);
    if (mysql_query(conn, query)) return 0;
    res = mysql_store_result(conn);
    int result = mysql_num_rows(res) > 0;
    mysql_free_result(res);
    return result;
}

int main(int argc, char *argv[]) {
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    int clnt_adr_sz;
    int sock_option = 1;
    pthread_t t_id;
    char idpasswd[(ID_SIZE*2)+3]; // ID:PASSWD -> ID_SIZE + : (1) + ID_SIZE + (널문자) 1 
    char *pToken, *pArray[ARR_CNT] = {0};

    if(argc != 2) {
        printf("Usage : %s <port>\n",argv[0]);
        exit(1);
    }

    // DB 연결
    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, "localhost", "iot", "pwiot", "userdb", 0, NULL, 0)) {
        fprintf(stderr, "DB 연결 실패: %s\n", mysql_error(conn));
        exit(1);
    }

    if(pthread_mutex_init(&mutx, NULL))
        error_handling("mutex init error");
    
    // 서버 소켓 생성
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (void*)&sock_option, sizeof(sock_option));

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    // 바인딩 및 리슨 bind() -- listen()
    if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
        error_handling("bind() error");

    if(listen(serv_sock, 5)==-1)
        error_handling("listen() error");

    fputs("IoT Server Start!!\n", stdout);

    // 클라이언트 연결 수락 루프
    while(1) {
        // accept 클라이언트가 connect를 통해 서버에 접속을 요청하면 accept하고 새로운 소켓을 생성
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
        if(clnt_sock < 0) 
            continue;
        
        // read 로그인 정보를 수신/파싱
        int str_len = read(clnt_sock, idpasswd, sizeof(idpasswd));
        idpasswd[str_len] = '\0';

        // : 을 기준으로 ID와 비밀번호를 분리해 파싱
        // MASTER:PASSWD -> pArray[0] = MASTER, pArray[1] = PASSWD
        int i = 0;
        pToken = strtok(idpasswd, ":");
        while(pToken != NULL) {
            pArray[i++] = pToken;
            if (i >= ARR_CNT) break;
            pToken = strtok(NULL, ":");
        }
        // 로그인 인증 성공시 클라이언트 정보 저장/ 쓰레드 생성
        // 클라이언트가 ID/PW 를 보내면 DB에 정보가 있는지 확인/ 있다면 조건문 수행
        if (db_login_check(pArray[0], pArray[1])) {
            // 클라이언트 배열을 여러 쓰레드가 공유해서 사용하므로 동기화가 필요 이를 위한 뮤텍스로 보호
            pthread_mutex_lock(&mutx);
            int idx;
            // 서버의 client_info 배열에 인덱스를 저장/ 만약 공석이 없다면 오류 출력
            for (idx = 0; idx < MAX_CLNT; idx++) {
                if (client_info[idx].fd == 0) 
                    break;
            }
            if (idx == MAX_CLNT) {
                write(clnt_sock, "[SERVER] Server full\n", 22);
                close(clnt_sock);
                pthread_mutex_unlock(&mutx);
                continue;
            }
            // 클라이언트 정보 등록
            client_info[idx].fd = clnt_sock;
            strcpy(client_info[idx].ip, inet_ntoa(clnt_adr.sin_addr));
            strcpy(client_info[idx].id, pArray[0]);
            clnt_cnt++;
            // 동기화 종료후 뮤텍스 해제
            pthread_mutex_unlock(&mutx);
            
            // 이제 클라이언트가 서버에 접속을 했음 들어온 것을 서버에서 디버깅 
            char welcome[BUF_SIZE];
            sprintf(welcome, "[%s] New connected! (ip:%s,fd:%d,sockcnt:%d)\n", pArray[0], client_info[idx].ip, clnt_sock, clnt_cnt);
            log_file(welcome);
            write(clnt_sock, welcome, strlen(welcome));
            // 접속한 클라이언트에 대한 쓰레드를 생성 여기서 클라이언트가 보내는 메시지를 read로 수신/처리함
            pthread_create(&t_id, NULL, clnt_connection, (void*)&client_info[idx]);
            // 쓰레드 종료시 자원 회수
            pthread_detach(t_id);
        } else {
            write(clnt_sock, "[SERVER] Authentication Error!\n", 33);
            close(clnt_sock);
        }
    }
    close(serv_sock);
    mysql_close(conn);
    return 0;
}

// 쓰레드 함수 클라이언트와 개별 통신을 처리
void * clnt_connection(void * arg) {
    CLIENT_INFO * cli = (CLIENT_INFO *)arg;
    char msg[BUF_SIZE];
    int str_len;

    while ((str_len = read(cli->fd, msg, sizeof(msg) - 1)) > 0) {
        msg[str_len] = 0;

        char *pToken, *pArray[ARR_CNT] = {0};
        int i = 0;
        pToken = strtok(msg, "[:]");  // [SIGNUP]id@pw
        while (pToken != NULL && i < ARR_CNT) {
            pArray[i++] = pToken;
            pToken = strtok(NULL, "[:]");
        }

        if (i >= 2) {
            //  줄바꿈 제거
            pArray[1][strcspn(pArray[1], "\r\n")] = 0;

            // [SIGNUP]id@pw (MASTER만 계정을 생성할 수 있게끔)
            if (!strcmp(pArray[0], "SIGNUP")) {
                if (strcmp(cli->id, "MASTER") != 0) {
                    write(cli->fd, "[SERVER] 계정 생성 권한이 없습니다.\n", 41);
                    continue;
                }

                char *newid = strtok(pArray[1], "@");
                char *newpw = strtok(NULL, "@");

                if (newid && newpw) {
                    char query[256];
                    sprintf(query, "INSERT INTO users (id, pw) VALUES ('%s', '%s')", newid, newpw);

                    if (mysql_query(conn, query) == 0) {
                        write(cli->fd, "[SERVER] 게정 생성 완료.\n",  50);
                    } else {
                        write(cli->fd, "[SERVER] 회원 가입 불가!!(중복계정)\n", 50);
                    }
                } else {
                    write(cli->fd, "[SERVER] 계정 생성 형식이 맞지 않습니다. Use id@pw\n", 50);
                }
                continue;
            }

            // [CHPASS]id@newpw (본인 or MASTER)
            if (!strcmp(pArray[0], "CHPASS")) {
                char *target_id = strtok(pArray[1], "@");
                char *newpw = strtok(NULL, "@");

                if (!target_id || !newpw) {
                    write(cli->fd, "[SERVER] 비밀번호 형식이 맞지 않습니다. Use id@newpw\n", 50);
                    continue;
                }

                if (strcmp(cli->id, "MASTER") != 0 && strcmp(cli->id, target_id) != 0) {
                    write(cli->fd, "[SERVER] 비밀번호 변경 권한이 없습니다.\n", 50);
                    continue;
                }

                char query[256];
                sprintf(query, "UPDATE users SET pw='%s' WHERE id='%s'", newpw, target_id);
                // 패스워드 변경 요청 mysql_query를 실행 passwd 변경하는 UPDATE 명령을 요청
                // mysql_affected_rows(conn) > 0 현재 계정 정보가 있었는지 확인 틀렸는데 바꾸면 안되니까
                if (mysql_query(conn, query) == 0 && mysql_affected_rows(conn) > 0) {
                    write(cli->fd, "[SERVER] 비밀번호 재설정 완료. 해당 계정은 강제 로그아웃됩니다.\n", 100);

                    // 강제 로그아웃 처리
                    // 뮤텍스로 client_info[], clnt_cnt 접근을 보호
                    pthread_mutex_lock(&mutx);
                    for (int k = 0; k < MAX_CLNT; k++) {
                        // 로그인중인 계정인지 찾기, 변경 대상 ID와 일치하는지 비교
                        if (client_info[k].fd != 0 && strcmp(client_info[k].id, target_id) == 0) {
                            write(client_info[k].fd, "[SERVER] 게정 정보가 변경되어 강제 로그아웃합니다.\n", 63);
                            // close를 통해 연결을 끊음
                            close(client_info[k].fd);
                            // 원래 있던 자리를 0으로 초기화
                            client_info[k].fd = 0;
                            // 접속 클라이언트 개수 감소
                            clnt_cnt--;
                            break;
                        }
                    }
                    // 뮤텍스 동기화 해제
                    pthread_mutex_unlock(&mutx);
                } else {
                    // 일련의 이유로 비밀번호를 못바꾸게 되면 출력
                    write(cli->fd, "[SERVER] 비밀번호 변경 실패.\n", 36);
                }
                continue;
            }

            // [DELETE]id@pw (본인 or MASTER, 강제 로그아웃 포함)
            if (!strcmp(pArray[0], "DELETE")) {
                char *target_id = strtok(pArray[1], "@");
                char *pw = strtok(NULL, "@");

                if (!target_id || !pw) { // 제거 형식이 맞지 않으면 
                    write(cli->fd, "[SERVER] 제거형식이 맞지 않습니다. Use id@pw\n", 50);
                    continue;
                }
                // MASTER또는 본인 계정이 아닌 곳에서 제거를 시도하는 경우
                if (strcmp(cli->id, "MASTER") != 0 && strcmp(cli->id, target_id) != 0) { 
                    write(cli->fd, "[SERVER] 권한이 없는 곳에서 제거할 수 없습니다.\n", 50);
                    continue;
                }
                // SQL에서 계정 정보를 조회
                char query[256];
                MYSQL_RES *res;
                sprintf(query, "SELECT * FROM users WHERE id='%s' AND pw='%s'", target_id, pw);
                if (mysql_query(conn, query) != 0) {
                    write(cli->fd, "[SERVER] DB정보가 일치하지 않습니다.\n", 50);
                    continue;
                }
                // SELECT 문에서 가져온 결과 값을 res 포인터에 저장
                res = mysql_store_result(conn);
                // mysql_num_rows(res) 는 결과를 가져와서 저장함 여기에 아무 값이 없다는 것은 삭제할 계정의 정보가 일치하지 않다는 것을 의미
                if (mysql_num_rows(res) == 0) {
                    write(cli->fd, "[SERVER] 아이디 또는 비밀번호가 일치하지 않습니다.\n", 100);
                    mysql_free_result(res);
                    continue;
                }
                // 사용 메모리 해제, 메모리 누수 예방
                mysql_free_result(res);

                // 삭제 수행
                // DB에 계정 정보를 삭제 요청
                sprintf(query, "DELETE FROM users WHERE id='%s'", target_id);
                if (mysql_query(conn, query) == 0 && mysql_affected_rows(conn) > 0) {
                    write(cli->fd, "[SERVER] 계정이 삭제되었습니다.\n", 50);

                    // 강제 로그아웃
                    pthread_mutex_lock(&mutx);
                    for (int k = 0; k < MAX_CLNT; k++) {
                        if (client_info[k].fd != 0 && strcmp(client_info[k].id, target_id) == 0) {
                            write(client_info[k].fd, "[SERVER] 당신의 계정은 삭제되었습니다.\n", 50);
                            close(client_info[k].fd);
                            client_info[k].fd = 0;
                            clnt_cnt--;
                            break;
                        }
                    }
                    pthread_mutex_unlock(&mutx);
                } else {
                    write(cli->fd, "[SERVER] 삭제 실패.\n", 50);
                }
                continue;
            }

            // 일반 채팅 메시지 처리
            MSG_INFO info;
            info.fd = cli->fd;      // 메시지를 보낸 클라이언트의 소켓
            info.from = cli->id;    // 보낸 사람 ID
            info.to = pArray[0];    // 수신 대상

            char fullmsg[BUF_SIZE];  // malloc 대신 스택 버퍼
            snprintf(fullmsg, sizeof(fullmsg), "[%s]%s\n", info.from, pArray[1]);
            info.msg = fullmsg;
            info.len = strlen(fullmsg);

            printf("msg : [%s->%s] %s\n", info.from, info.to, pArray[1]);

            send_msg(&info); // 메시지를 실제로 송신하는 함수를 호출
        }
    }

    // 연결 종료
    close(cli->fd);
    pthread_mutex_lock(&mutx);
    cli->fd = 0;
    clnt_cnt--;
    pthread_mutex_unlock(&mutx);

    return NULL;
}

// 서버가 클라이언트로부터 받은 메시지를 다른 클라이언트에게 전송하는 역할수행 함수
void send_msg(MSG_INFO * msg_info) {
    // 뮤텍스 동기화 다중 클라이언트가 이 함수를 호출할 수 있게끔
    pthread_mutex_lock(&mutx);

    for (int i = 0; i < MAX_CLNT; i++) {
        if (client_info[i].fd == 0) 
            continue;
        // 전체 전송
        if (!strcmp(msg_info->to, "ALLMSG")) {
            write(client_info[i].fd, msg_info->msg, msg_info->len);
        }
        // 귓속말
        else if (!strcmp(msg_info->to, client_info[i].id)) {
            write(client_info[i].fd, msg_info->msg, msg_info->len);
        }
    }
    pthread_mutex_unlock(&mutx);
}

void error_handling(char * msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

void log_file(char * msgstr) {
    fputs(msgstr, stdout);
}
