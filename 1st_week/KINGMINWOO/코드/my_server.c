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

#define BUF_SIZE 100
#define ID_SIZE 10
#define ARR_CNT 10
#define MAX_SOCKET 50

typedef struct {
    char fd;
    char *from;
    char *to;
    char *msg;
    int len;
} MSG_INFO;

typedef struct {
    int index;
    int fd;
    char ip[20];
    char id[ID_SIZE];
    char pw[ID_SIZE];
} CLIENT_INFO;

void *clnt_connection(void *arg);
void send_msg(MSG_INFO *msg_info, CLIENT_INFO *first_client_info);
void error_handling(char *msg);
void log_file(char *msgstr);
void finish_with_error(MYSQL *con);
CLIENT_INFO* id_password_read(CLIENT_INFO* client_info, int *member_count);
bool signup_new_member(const char *id, const char *pw);
bool change_password(const char* id, const char* new_pw);
bool delete_member(const char* id);

int countMember = 0;
int clnt_cnt = 0;
pthread_mutex_t mutx;
CLIENT_INFO *client_info = NULL;

int main(int argc, char *argv[]) {
    client_info = id_password_read(NULL, &countMember);
    
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t clnt_adr_sz;
    int sock_option = 1;
    pthread_t *t_id = malloc(MAX_SOCKET * sizeof(pthread_t));
    char msg[BUF_SIZE];

    if (argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    fputs("Server Start!!\n",stdout);

    if (pthread_mutex_init(&mutx, NULL))
        error_handling("mutex init error");

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));
    
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &sock_option, sizeof(sock_option));
    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");

    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    while (1) {
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
        if (clnt_cnt >= MAX_SOCKET) {
            shutdown(clnt_sock, SHUT_WR);
            continue;
        }

        char idpasswd[(ID_SIZE * 2) + 3] = {0};
        int str_len = read(clnt_sock, idpasswd, sizeof(idpasswd));
        if (str_len <= 0) {
            shutdown(clnt_sock, SHUT_WR);
            continue;
        }
        idpasswd[str_len] = '\0';

        char *pArray[ARR_CNT] = {0};
        char *pToken = strtok(idpasswd, "[:]");
        int i = 0;
        while (pToken != NULL && i < ARR_CNT) {
            pArray[i++] = pToken;
            pToken = strtok(NULL, "[:]");
        }

        for (i = 0; i < countMember; i++) {
            if (!strcmp(client_info[i].id, pArray[0])) {
                if (!strcmp(client_info[i].pw, pArray[1])) {
                    pthread_mutex_lock(&mutx);
                    client_info[i].fd = clnt_sock;
                    strcpy(client_info[i].ip, inet_ntoa(clnt_adr.sin_addr));
                    client_info[i].index = i;
                    clnt_cnt++;
                    pthread_mutex_unlock(&mutx);
					sprintf(msg, "[%s] New connected! (ip:%s, fd:%d, sockcnt: %d)\n", pArray[0], inet_ntoa(clnt_adr.sin_addr), clnt_sock, clnt_cnt);
					log_file(msg);
                    write(clnt_sock, "Login success\n", 15);
                    pthread_create(&t_id[i], NULL, clnt_connection, (void *)(client_info + i));
                    pthread_detach(t_id[i]);
                    break;
                }
            }
        }

        if (i == countMember) {
            sprintf(msg,"[%s] Authentication Error!\n",pArray[0]);
            write(clnt_sock, msg,strlen(msg));
            log_file(msg);
            shutdown(clnt_sock, SHUT_WR);
        }
    }

    close(serv_sock);
    free(t_id);
    free(client_info);
    return 0;
}

void *clnt_connection(void *arg) {
    CLIENT_INFO *ci = (CLIENT_INFO *)arg;
    char msg[BUF_SIZE];
    char to_msg[countMember*ID_SIZE+1];
    char strBuff[BUF_SIZE*2]={0};
    MSG_INFO msg_info;
    CLIENT_INFO *first_info = client_info;

    while (1) {
        int str_len = read(ci->fd, msg, sizeof(msg) - 1);
        if (str_len <= 0) break;

        msg[str_len] = '\0';
        char *pArray[ARR_CNT] = {0};
        char *pToken = strtok(msg, "[:@]");
        int i = 0;
        while (pToken != NULL && i < ARR_CNT) {
            pArray[i++] = pToken;
            pToken = strtok(NULL, "[:@]");
        }
        
        // SIGNUP
        if (pArray[1] && !strcmp(pArray[1], "SIGNUP") && !strcmp(ci->id, "MASTER")) {
            pArray[3][strcspn(pArray[3], "\r\n")] = '\0';
            if (signup_new_member(pArray[2], pArray[3])) {
                pthread_mutex_lock(&mutx);
                CLIENT_INFO *new_info = id_password_read(NULL, &countMember);
                if (new_info) {
                    free(client_info);
                    client_info = new_info;
                }
                pthread_mutex_unlock(&mutx);
                write(ci->fd, "SIGNUP success\n", 15);
            } 
            else {
                write(ci->fd, "SIGNUP failed\n", 14);
            }
            continue;
        }
        else if (pArray[1] && !strcmp(pArray[1], "CHANGE") && (!strcmp(ci->id, "MASTER") || !strcmp(pArray[2], ci->id))) { // CHANGE
            pArray[3][strcspn(pArray[3], "\r\n")] = '\0';
            if (change_password(pArray[2], pArray[3])){
                pthread_mutex_lock(&mutx);
                CLIENT_INFO *new_info = id_password_read(NULL, &countMember);
                if (new_info) {
                    free(client_info);
                    client_info = new_info;
                }
                pthread_mutex_unlock(&mutx);
                write(ci->fd, "CHANGE success\n", 15);
            }
            else {
                write(ci->fd, "CHANGE failed\n", 14);
            }
            continue;
        }
        else if(pArray[1] && !strcmp(pArray[1], "DELETE") && (!strcmp(ci->id, "MASTER") || !strcmp(pArray[2], ci->id))){ // DELETE
            pArray[2][strcspn(pArray[2], "\r\n")] = '\0';
            if(delete_member(pArray[2])){
                pthread_mutex_lock(&mutx);

                // 접속 중인 해당 ID 유저를 찾아 소켓 종료
                for (int j = 0; j < countMember; j++) {
                    if (client_info[j].fd != -1 && !strcmp(client_info[j].id, pArray[2])) {
                        char disconnect_msg[] = "Your account has been deleted. Disconnecting...\n";
                        write(client_info[j].fd, disconnect_msg, strlen(disconnect_msg));
                        shutdown(client_info[j].fd, SHUT_WR);
                        close(client_info[j].fd);
                        client_info[j].fd = -1;
                        clnt_cnt--;
                        char logbuf[100];
                        sprintf(logbuf, "User %s has been deleted and disconnected.\n", pArray[2]);
                        log_file(logbuf);
                        break;
                    }
                }

                CLIENT_INFO *new_info = id_password_read(NULL, &countMember);
                if (new_info) {
                    free(client_info);
                    client_info = new_info;
                }
                pthread_mutex_unlock(&mutx);
                write(ci->fd, "DELETE success\n", 15);
                
            }
            else {
                write(ci->fd, "DELETE failed\n", 14);
            }
            continue;
        }
             
        // 일반 메시지 처리
        msg_info.fd = ci->fd;
        msg_info.from = ci->id;
        msg_info.to = pArray[0];
        sprintf(to_msg, "[%s]%s", msg_info.from, pArray[1]);
        msg_info.msg = to_msg;
        msg_info.len = strlen(to_msg);

        sprintf(strBuff,"msg : [%s->%s] %s",msg_info.from,msg_info.to,pArray[1]);
        log_file(strBuff);
        send_msg(&msg_info, first_info);
    }

    close(ci->fd);
	sprintf(strBuff,"Disconnect ID: %s (IP:%s, fd: %d, sockcnt:%d)\n", ci->id, ci->ip, ci->fd, clnt_cnt -1);
	log_file(strBuff);
    pthread_mutex_lock(&mutx);
    clnt_cnt--;
    ci->fd = -1;
    pthread_mutex_unlock(&mutx);
    return 0;
}

void send_msg(MSG_INFO *msg_info, CLIENT_INFO *first_client_info) {
    int i=0;

    if(!strcmp(msg_info->to,"ALLMSG"))
    {
        for(i=0;i<countMember;i++)
            if((first_client_info+i)->fd != -1)	
                write((first_client_info+i)->fd, msg_info->msg, msg_info->len);
    }
    else if(!strcmp(msg_info->to,"IDLIST"))
    {
        char* idlist = (char *)malloc(ID_SIZE * countMember);
        msg_info->msg[strlen(msg_info->msg) - 1] = '\0';
        strcpy(idlist,msg_info->msg);

        for(i=0;i<countMember;i++)
        {
            if((first_client_info+i)->fd != -1)	
            {
                strcat(idlist,(first_client_info+i)->id);
                strcat(idlist," ");
            }
        }
        strcat(idlist,"\n");
        write(msg_info->fd, idlist, strlen(idlist));
        free(idlist);
    }
    else
        for(i=0;i<countMember;i++)
            if((first_client_info+i)->fd != -1)	
                if(!strcmp(msg_info->to,(first_client_info+i)->id))
                    write((first_client_info+i)->fd, msg_info->msg, msg_info->len);
}

CLIENT_INFO* id_password_read(CLIENT_INFO* client_info, int *member_count) {
    MYSQL *con = mysql_init(NULL);
    if (!mysql_real_connect(con, "localhost", "id", "pw", "idpw", 0, NULL, 0)) finish_with_error(con);
    if (mysql_query(con, "SELECT * FROM idpasswd")) finish_with_error(con);

    MYSQL_RES *res = mysql_store_result(con);
    int num_rows = mysql_num_rows(res);

    if (member_count) *member_count = num_rows;
    client_info = (CLIENT_INFO *)calloc(num_rows, sizeof(CLIENT_INFO));

    MYSQL_ROW row;
    int i = 0;
    while ((row = mysql_fetch_row(res))) {
        strcpy(client_info[i].id, row[0]);
        strcpy(client_info[i].pw, row[1]);
        client_info[i].fd = -1;
        i++;
    }

    mysql_free_result(res);
    mysql_close(con);
    return client_info;
}

bool signup_new_member(const char *id, const char *pw) {
    MYSQL *con = mysql_init(NULL);
    if (!mysql_real_connect(con, "localhost", "id", "pw", "idpw", 0, NULL, 0)) return false;

    char query[200];
    sprintf(query, "SELECT * FROM idpasswd WHERE id='%s'", id);
    mysql_query(con, query);
    MYSQL_RES *res = mysql_store_result(con);

    if (mysql_num_rows(res) > 0) {
        mysql_free_result(res);
        mysql_close(con);
        return false;
    }

    mysql_free_result(res);
    sprintf(query, "INSERT INTO idpasswd VALUES('%s', '%s')", id, pw);
    bool success = mysql_query(con, query) == 0;
    mysql_close(con);
    return success;
}

bool change_password(const char* id, const char* new_pw) {
    MYSQL *con = mysql_init(NULL);
    if (con == NULL) return false;

    if (!mysql_real_connect(con, "localhost", "id", "pw", "idpw", 0, NULL, 0)) {
        mysql_close(con);
        return false;
    }

    char query[200];
    sprintf(query, "UPDATE idpasswd SET password='%s' WHERE id='%s'", new_pw, id);
    if (mysql_query(con, query)) {
        mysql_close(con);
        return false;
    }

    mysql_close(con);
    return true;
}

bool delete_member(const char* id) {
    MYSQL *con = mysql_init(NULL);
    if (con == NULL) return false;

    if (!mysql_real_connect(con, "localhost", "id", "pw", "idpw", 0, NULL, 0)) {
        mysql_close(con);
        return false;
    }

    char query[200];
    sprintf(query, "DELETE FROM idpasswd WHERE id='%s'", id);
    if (mysql_query(con, query)) {
        mysql_close(con);
        return false;
    }

    mysql_close(con);
    return true;
}

void error_handling(char *msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

void log_file(char *msgstr) {
    fputs(msgstr, stdout);
}

void finish_with_error(MYSQL *con) {
    fprintf(stderr, "%s\n", mysql_error(con));
    mysql_close(con);
    exit(1);
}
