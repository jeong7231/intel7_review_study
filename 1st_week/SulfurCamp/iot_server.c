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

typedef struct {
    int fd;
    char ip[20];
    char id[ID_SIZE];
} CLIENT_INFO;

typedef struct {
    int fd;
    char *from;
    char *to;
    char *msg;
    int len;
} MSG_INFO;

void * clnt_connection(void * arg);
void send_msg(MSG_INFO * msg_info);
void error_handling(char * msg);
void log_file(char * msgstr);

int clnt_cnt = 0;
pthread_mutex_t mutx;
CLIENT_INFO client_info[MAX_CLNT];
MYSQL *conn;

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
    char idpasswd[(ID_SIZE*2)+3];
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

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (void*)&sock_option, sizeof(sock_option));

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
        error_handling("bind() error");

    if(listen(serv_sock, 5)==-1)
        error_handling("listen() error");

    fputs("IoT Server Start!!\n", stdout);

    while(1) {
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
        if(clnt_sock < 0) continue;

        int str_len = read(clnt_sock, idpasswd, sizeof(idpasswd));
        idpasswd[str_len] = '\0';

        int i = 0;
        pToken = strtok(idpasswd, ":");
        while(pToken != NULL) {
            pArray[i++] = pToken;
            if (i >= ARR_CNT) break;
            pToken = strtok(NULL, ":");
        }

        if (db_login_check(pArray[0], pArray[1])) {
            pthread_mutex_lock(&mutx);
            int idx;
            for (idx = 0; idx < MAX_CLNT; idx++) {
                if (client_info[idx].fd == 0) break;
            }
            if (idx == MAX_CLNT) {
                write(clnt_sock, "[SERVER] Server full\n", 22);
                close(clnt_sock);
                pthread_mutex_unlock(&mutx);
                continue;
            }

            client_info[idx].fd = clnt_sock;
            strcpy(client_info[idx].ip, inet_ntoa(clnt_adr.sin_addr));
            strcpy(client_info[idx].id, pArray[0]);
            clnt_cnt++;
            pthread_mutex_unlock(&mutx);

            char welcome[BUF_SIZE];
            sprintf(welcome, "[%s] New connected! (ip:%s,fd:%d,sockcnt:%d)\n",
                    pArray[0], client_info[idx].ip, clnt_sock, clnt_cnt);
            log_file(welcome);
            write(clnt_sock, welcome, strlen(welcome));
            pthread_create(&t_id, NULL, clnt_connection, (void*)&client_info[idx]);
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

void * clnt_connection(void * arg) {
    CLIENT_INFO * cli = (CLIENT_INFO *)arg;
    char msg[BUF_SIZE];
    int str_len;

    while ((str_len = read(cli->fd, msg, sizeof(msg) - 1)) > 0) {
        msg[str_len] = 0;

        char *pToken, *pArray[ARR_CNT] = {0};
        int i = 0;
        pToken = strtok(msg, "[:]");  // ex) [SIGNUP]id@pw
        while (pToken != NULL && i < ARR_CNT) {
            pArray[i++] = pToken;
            pToken = strtok(NULL, "[:]");
        }

        if (i >= 2) {
            //  줄바꿈 제거
            pArray[1][strcspn(pArray[1], "\r\n")] = 0;

            // [SIGNUP]id@pw (MASTER만)
            if (!strcmp(pArray[0], "SIGNUP")) {
                if (strcmp(cli->id, "MASTER") != 0) {
                    write(cli->fd, "[SERVER] Signup failed or unauthorized.\n", 41);
                    continue;
                }

                char *newid = strtok(pArray[1], "@");
                char *newpw = strtok(NULL, "@");

                if (newid && newpw) {
                    char query[256];
                    sprintf(query, "INSERT INTO users (id, pw) VALUES ('%s', '%s')", newid, newpw);

                    if (mysql_query(conn, query) == 0) {
                        write(cli->fd, "[SERVER] Account created.\n", 28);
                    } else {
                        write(cli->fd, "[SERVER] Signup failed (maybe duplicate ID).\n", 46);
                    }
                } else {
                    write(cli->fd, "[SERVER] Invalid SIGNUP format. Use id@pw\n", 43);
                }
                continue;
            }

            // [CHPASS]id@newpw (본인 or MASTER)
            if (!strcmp(pArray[0], "CHPASS")) {
                char *target_id = strtok(pArray[1], "@");
                char *newpw = strtok(NULL, "@");

                if (!target_id || !newpw) {
                    write(cli->fd, "[SERVER] Invalid CHPASS format. Use id@newpw\n", 47);
                    continue;
                }

                if (strcmp(cli->id, "MASTER") != 0 && strcmp(cli->id, target_id) != 0) {
                    write(cli->fd, "[SERVER] Unauthorized to change this password.\n", 49);
                    continue;
                }

                char query[256];
                sprintf(query, "UPDATE users SET pw='%s' WHERE id='%s'", newpw, target_id);

                if (mysql_query(conn, query) == 0 && mysql_affected_rows(conn) > 0) {
                    write(cli->fd, "[SERVER] Password changed.\n", 29);
                } else {
                    write(cli->fd, "[SERVER] Password change failed.\n", 36);
                }
                continue;
            }

            // [DELETE]id@pw (본인 or MASTER, 강제 로그아웃 포함)
            if (!strcmp(pArray[0], "DELETE")) {
                char *target_id = strtok(pArray[1], "@");
                char *pw = strtok(NULL, "@");

                if (!target_id || !pw) {
                    write(cli->fd, "[SERVER] Invalid DELETE format. Use id@pw\n", 43);
                    continue;
                }

                if (strcmp(cli->id, "MASTER") != 0 && strcmp(cli->id, target_id) != 0) {
                    write(cli->fd, "[SERVER] Delete failed or unauthorized.\n", 43);
                    continue;
                }

                char query[256];
                MYSQL_RES *res;
                sprintf(query, "SELECT * FROM users WHERE id='%s' AND pw='%s'", target_id, pw);
                if (mysql_query(conn, query) != 0) {
                    write(cli->fd, "[SERVER] DB error.\n", 20);
                    continue;
                }

                res = mysql_store_result(conn);
                if (mysql_num_rows(res) == 0) {
                    write(cli->fd, "[SERVER] ID or password incorrect.\n", 36);
                    mysql_free_result(res);
                    continue;
                }
                mysql_free_result(res);

                // 삭제 수행
                sprintf(query, "DELETE FROM users WHERE id='%s'", target_id);
                if (mysql_query(conn, query) == 0 && mysql_affected_rows(conn) > 0) {
                    write(cli->fd, "[SERVER] Account deleted.\n", 27);

                    // 강제 로그아웃
                    pthread_mutex_lock(&mutx);
                    for (int k = 0; k < MAX_CLNT; k++) {
                        if (client_info[k].fd != 0 && strcmp(client_info[k].id, target_id) == 0) {
                            write(client_info[k].fd, "[SERVER] You have been deleted.\n", 34);
                            close(client_info[k].fd);
                            client_info[k].fd = 0;
                            clnt_cnt--;
                            break;
                        }
                    }
                    pthread_mutex_unlock(&mutx);
                } else {
                    write(cli->fd, "[SERVER] Delete failed.\n", 28);
                }
                continue;
            }

            // 일반 메시지 처리 (채팅)
            MSG_INFO info;
            info.fd = cli->fd;
            info.from = cli->id;
            info.to = pArray[0];

            char *fullmsg = malloc(BUF_SIZE);
            snprintf(fullmsg, BUF_SIZE, "[%s]%s\n", info.from, pArray[1]);
            info.msg = fullmsg;
            info.len = strlen(fullmsg);

            printf("msg : [%s->%s] %s\n", info.from, info.to, pArray[1]);

            send_msg(&info);
            free(fullmsg);
        }
    }

    //연결 종료
    close(cli->fd);
    pthread_mutex_lock(&mutx);
    cli->fd = 0;
    clnt_cnt--;
    pthread_mutex_unlock(&mutx);

    return NULL;
}

void send_msg(MSG_INFO * msg_info) {
    pthread_mutex_lock(&mutx);
    for (int i = 0; i < MAX_CLNT; i++) {
        if (client_info[i].fd == 0) continue;

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
