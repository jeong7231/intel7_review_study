
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <mysql/mysql.h>

#define BUF_SIZE 100
#define MAX_CLNT 32
#define ID_SIZE 10
#define ARR_CNT 5

#define DEBUG
typedef struct {
		char fd;
		char *from;
		char *to;
		char *msg;
		int len;
}MSG_INFO;

typedef struct {
		int index;
		int fd;
		char ip[20];
		char id[ID_SIZE];
		char pw[ID_SIZE];
}CLIENT_INFO;

void * clnt_connection(void * arg);
void send_msg(MSG_INFO * msg_info, CLIENT_INFO * first_client_info);
void error_handling(char * msg);
void log_file(char * msgstr);

// DB에서 계정을 불러와 저장하는 함수
int load_accounts_from_db(CLIENT_INFO *client_info, int max_clients);
// DB에 계정을 생성하는 함수
int create_account_in_db(const char* id, const char* pw);
// DB에서 계정을 삭제하는 함수
int delete_account_in_db(const char* id);

int clnt_cnt=0;
pthread_mutex_t mutx;

int main(int argc, char *argv[])
{
		int serv_sock, clnt_sock;
		struct sockaddr_in serv_adr, clnt_adr;
		int clnt_adr_sz;
		int sock_option  = 1;
		pthread_t t_id[MAX_CLNT] = {0};
		int str_len = 0;
		int i = 0;
		char idpasswd[(ID_SIZE*2)+3];
		char *pToken;
		char *pArray[ARR_CNT]={0};
		char msg[BUF_SIZE];
		// FILE* in; // open파일의 pd를 저장할 변수.
		CLIENT_INFO* client_info;
		client_info = (CLIENT_INFO *)malloc(sizeof(CLIENT_INFO) * MAX_CLNT);
		
		if(argc != 2) {
            printf("Usage : %s <port>\n",argv[0]);
            exit(1);
		}

		//printf("id : %s, pw : %s\n",(client_info+i)->id, (client_info+i)->pw);	

		fputs("IoT Server Start!!\n",stdout);

        load_accounts_from_db(client_info, MAX_CLNT);

		if(pthread_mutex_init(&mutx, NULL))
            error_handling("mutex init error");

		serv_sock = socket(PF_INET, SOCK_STREAM, 0);

		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family=AF_INET;
		serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
		serv_adr.sin_port=htons(atoi(argv[1]));

		setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (void*)&sock_option, sizeof(sock_option));
		if(bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr))==-1)
            error_handling("bind() error");

		if(listen(serv_sock, 5) == -1)
            error_handling("listen() error");

		while(1) {
            clnt_adr_sz = sizeof(clnt_adr);
            clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
            if(clnt_cnt >= MAX_CLNT)
            {
                printf("socket full\n");
                shutdown(clnt_sock,SHUT_WR);
                continue;
            }
            else if(clnt_sock < 0)
            {
                perror("accept()");
                continue;
            }

            str_len = read(clnt_sock, idpasswd, sizeof(idpasswd));
            idpasswd[str_len] = '\0';

            if(str_len > 0)
            {
                i=0;
                pToken = strtok(idpasswd,"[:]");

                while(pToken != NULL)
                {
                    pArray[i] =  pToken;
                    if(i++ >= ARR_CNT)
                        break;	
                    pToken = strtok(NULL,"[:]");
                }
                for(i=0;i<MAX_CLNT;i++)
                {
                    if(!strcmp(client_info[i].id,pArray[0]))
                    {
                        if(client_info[i].fd != -1)
                        {
                            sprintf(msg,"[%s] Already logged!\n",pArray[0]);
                            write(clnt_sock, msg,strlen(msg));
                            log_file(msg);
                            shutdown(clnt_sock,SHUT_WR);
#if 1   //for MCU
                            client_info[i].fd = -1;
#endif  
                            break;
                        }
                        if(!strcmp(client_info[i].pw,pArray[1])) 
                        {

                            strcpy(client_info[i].ip,inet_ntoa(clnt_adr.sin_addr));
                            pthread_mutex_lock(&mutx);
                            client_info[i].index = i; 
                            client_info[i].fd = clnt_sock; 
                            clnt_cnt++;
                            pthread_mutex_unlock(&mutx);
                            sprintf(msg,"[%s] New connected! (ip:%s,fd:%d,sockcnt:%d)\n",pArray[0],inet_ntoa(clnt_adr.sin_addr),clnt_sock,clnt_cnt);
                            log_file(msg);
                            write(clnt_sock, msg,strlen(msg));

                            pthread_create(t_id+i, NULL, clnt_connection, (void *)(client_info + i));
                            pthread_detach(t_id[i]);
                            break;
                        }
                    }
                }
                if(i == MAX_CLNT)
                {
                    sprintf(msg,"[%s] Authentication Error!\n",pArray[0]);
                    write(clnt_sock, msg,strlen(msg));
                    log_file(msg);
                    shutdown(clnt_sock,SHUT_WR);
                }
            }
            else 
                shutdown(clnt_sock,SHUT_WR);

		}
		return 0;
}

void * clnt_connection(void *arg)
{
    CLIENT_INFO * client_info = (CLIENT_INFO *)arg;
    int str_len = 0;
    int index = client_info->index;
    char msg[BUF_SIZE];
    char to_msg[MAX_CLNT*ID_SIZE+1];
    int i=0;
    char *pToken;
    char *pArray[ARR_CNT]={0};
    char strBuff[BUF_SIZE*2]={0};

    MSG_INFO msg_info;
    CLIENT_INFO  * first_client_info;

    first_client_info = (CLIENT_INFO *)((void *)client_info - (void *)( sizeof(CLIENT_INFO) * index ));
    while(1)
    {
        memset(msg,0x0,sizeof(msg));
        str_len = read(client_info->fd, msg, sizeof(msg)-1); 
        if(str_len <= 0)
            break;

        msg[str_len] = '\0';
        pToken = strtok(msg,"[:]"); // pArray[0] = 목적지(ALLMSG/ID), pArray[1] = 내용
        i = 0; 
        while(pToken != NULL)
        {
            pArray[i] =  pToken;
            if(i++ >= ARR_CNT)
                break;	
            pToken = strtok(NULL,"[:]");
        }

        // pArray[1] = "SIGNUP@ID@PW"
        if(pArray[1] && strncmp(pArray[1], "SIGNUP@", 7) == 0) {
            char *signup_cmd = strdup(pArray[1]);
            char *saveptr = NULL;
            char *cmd = strtok_r(signup_cmd, "@", &saveptr); // "SIGNUP"
            char *newid = strtok_r(NULL, "@", &saveptr);     // ID
            char *newpw = strtok_r(NULL, "@", &saveptr);     // PW
            // 개행 문자 제거 코드
            if(newid)
                newid[strcspn(newid, "\r\n")] = '\0';
            if(newpw)
                newpw[strcspn(newpw, "\r\n")] = '\0';

            char signup_result[BUF_SIZE] = {0};

            if(newid && newpw) {
                int ret = create_account_in_db(newid, newpw);
                if(ret == 1) {
                    snprintf(signup_result, sizeof(signup_result),
                        "[%s]SIGNUP@%s@%s@SUCCESS\n", client_info->id, newid, newpw);
                } else if(ret == -1) {
                    snprintf(signup_result, sizeof(signup_result),
                        "[%s]SIGNUP@%s@%s@FAIL\n", client_info->id, newid, newpw);
                } else {
                    snprintf(signup_result, sizeof(signup_result),
                        "[%s]SIGNUP@%s@%s@FAIL\n", client_info->id, newid, newpw);
                }
            } else {
                snprintf(signup_result, sizeof(signup_result),
                    "[%s]SIGNUP@FAIL@FAIL@FAIL\n", client_info->id);
            }
            write(client_info->fd, signup_result, strlen(signup_result));
            log_file(signup_result);
            free(signup_cmd);
            continue;
        }
        else if(pArray[1] && strncmp(pArray[1], "DELACC@", 7) == 0) {
            char *delacc_cmd = strdup(pArray[1]);
            char *saveptr = NULL;
            char *cmd = strtok_r(delacc_cmd, "@", &saveptr); // "DELACC"
            char *delid = strtok_r(NULL, "@", &saveptr);     // ID

            // 개행 문자 제거
            if(delid)
                delid[strcspn(delid, "\r\n")] = '\0';

            char delacc_result[BUF_SIZE] = {0};

            if(delid) {
                int ret = delete_account_in_db(delid);
                if(ret == 1) {
                    snprintf(delacc_result, sizeof(delacc_result),
                        "[%s]DELACC@%s@SUCCESS\n", client_info->id, delid);
                } else {
                    snprintf(delacc_result, sizeof(delacc_result),
                        "[%s]DELACC@%s@FAIL\n", client_info->id, delid);
                }
            } else {
                snprintf(delacc_result, sizeof(delacc_result),
                    "[%s]DELACC@FAIL@FAIL\n", client_info->id);
            }
            write(client_info->fd, delacc_result, strlen(delacc_result));
            log_file(delacc_result);
            free(delacc_cmd);
            continue;
        }

        msg_info.fd = client_info->fd;
        msg_info.from = client_info->id;
        msg_info.to = pArray[0];
        sprintf(to_msg,"[%s]%s",msg_info.from,pArray[1]);
        msg_info.msg = to_msg;
        msg_info.len = strlen(to_msg);

        sprintf(strBuff,"msg : [%s->%s] %s",msg_info.from,msg_info.to,pArray[1]);
        log_file(strBuff);
        send_msg(&msg_info, first_client_info);
    }

    close(client_info->fd);

    sprintf(strBuff,"Disconnect ID:%s (ip:%s,fd:%d,sockcnt:%d)\n",client_info->id,client_info->ip,client_info->fd,clnt_cnt-1);
    log_file(strBuff);

    pthread_mutex_lock(&mutx);
    clnt_cnt--;
    client_info->fd = -1;
    pthread_mutex_unlock(&mutx);

    return 0;
}

void send_msg(MSG_INFO * msg_info, CLIENT_INFO * first_client_info)
{
		int i=0;

		if(!strcmp(msg_info->to,"ALLMSG"))
		{
            for(i=0;i<MAX_CLNT;i++)
                if((first_client_info+i)->fd != -1)	
                    write((first_client_info+i)->fd, msg_info->msg, msg_info->len);
		}
		else if(!strcmp(msg_info->to,"IDLIST"))
		{
            char* idlist = (char *)malloc(ID_SIZE * MAX_CLNT);
            msg_info->msg[strlen(msg_info->msg) - 1] = '\0';
            strcpy(idlist,msg_info->msg);

            for(i=0;i<MAX_CLNT;i++)
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
            for(i=0;i<MAX_CLNT;i++)
                if((first_client_info+i)->fd != -1)	
                    if(!strcmp(msg_info->to,(first_client_info+i)->id))
                        write((first_client_info+i)->fd, msg_info->msg, msg_info->len);
}

void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

void log_file(char * msgstr)
{
    fputs(msgstr,stdout);
}

int load_accounts_from_db(CLIENT_INFO *client_info, int max_clients)
{
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char *host = "localhost";
    char *user = "root";
    char *password = "1";
    char *database = "socket";
    char *query = "SELECT id, pw FROM account";
    int db_cnt = 0;

    conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        exit(1);
    }
    if (mysql_real_connect(conn, host, user, password, database, 0, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(1);
    }
    if (mysql_query(conn, query)) {
        fprintf(stderr, "SELECT query failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(1);
    }
    res = mysql_store_result(conn);
    if (res == NULL) {
        fprintf(stderr, "mysql_store_result() failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(1);
    }
    while ((row = mysql_fetch_row(res))) {
        client_info[db_cnt].fd = -1;
        strncpy(client_info[db_cnt].id, row[0], ID_SIZE - 1);
        client_info[db_cnt].id[ID_SIZE - 1] = '\0';
        strncpy(client_info[db_cnt].pw, row[1], ID_SIZE - 1);
        client_info[db_cnt].pw[ID_SIZE - 1] = '\0';
        db_cnt++;
        if (db_cnt >= max_clients) {
            printf("error client_info pull(Max : %d)\n", max_clients);
            break;
        }
    }
    mysql_free_result(res);
    mysql_close(conn);

    for (int i = db_cnt; i < max_clients; i++) {
        client_info[i].fd = -1;
        client_info[i].id[0] = '\0';
        client_info[i].pw[0] = '\0';
        client_info[i].ip[0] = '\0';
        client_info[i].index = i;
    }
    
    return db_cnt;
}

int create_account_in_db(const char* id, const char* pw)
{
    MYSQL *conn;
    char *host = "localhost";
    char *user = "root";
    char *password = "1";
    char *database = "socket";
    char query[256];
    int result = 0;

    conn = mysql_init(NULL);
    if (conn == NULL) return 0;
    if (mysql_real_connect(conn, host, user, password, database, 0, NULL, 0) == NULL) {
        mysql_close(conn);
        return 0;
    }
    // 이미 존재하는지 확인
    snprintf(query, sizeof(query), "SELECT COUNT(*) FROM account WHERE id='%s'", id);
    if (mysql_query(conn, query)) {
        mysql_close(conn);
        return 0;
    }
    MYSQL_RES *res = mysql_store_result(conn);
    if (!res) {
        mysql_close(conn);
        return 0;
    }
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row && atoi(row[0]) > 0) {
        mysql_free_result(res);
        mysql_close(conn);
        return -1; // 이미 있는 경우
    }
    mysql_free_result(res);

    // 계정 생성
    snprintf(query, sizeof(query), "INSERT INTO account (id, pw) VALUES ('%s', '%s')", id, pw);
    if (mysql_query(conn, query)) {
        mysql_close(conn);
        return 0;
    }
    mysql_close(conn);
    return 1;
}

int delete_account_in_db(const char* id) {
    MYSQL *conn;
    char *host = "localhost";
    char *user = "root";
    char *password = "1";
    char *database = "socket";
    char query[256];

    conn = mysql_init(NULL);
    if (conn == NULL) return 0;
    if (mysql_real_connect(conn, host, user, password, database, 0, NULL, 0) == NULL) {
        mysql_close(conn);
        return 0;
    }
    snprintf(query, sizeof(query), "DELETE FROM account WHERE id='%s'", id);
    int ret = mysql_query(conn, query);
    int affected = mysql_affected_rows(conn);
    mysql_close(conn);

    // 삭제된 행이 1개 이상이면 성공
    if (ret == 0 && affected > 0)
        return 1;
    else
        return 0; // 없는 아이디 or 에러
}