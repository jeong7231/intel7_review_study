// 표준 C 헤더
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
// 시스템/OS 헤더
#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

// mysql
#include <mysql/mysql.h>

#define BUF_SIZE 1000
#define MAX_CLIENT 50
#define ID_SIZE 10
#define ARRAY_COUNT 5

#define DEBUG

typedef struct
{
    int fd; // char → int
    char from[ID_SIZE];
    char to[ID_SIZE];
    char msg[BUF_SIZE];
    int len; // char → int
} MSG_INFO;

typedef struct
{
    int index;
    int fd;
    char ip[20];
    char id[ID_SIZE];
    char pw[ID_SIZE];
} CLIENT_INFO;

void *client_connection(void *arg);
void send_msg(MSG_INFO *msg_info, CLIENT_INFO *first_client_info);
void error_handling(char *msg);
void log_file(char *msgstr);
void get_localtime(char *buf);
// void load_file(const char *filename, CLIENT_INFO *client_info, int max_clients); // txt 파일에서

void load_idpasswd_db(CLIENT_INFO *client_info, int max_clients); // DB에서 가져오기
void sql_error(MYSQL *con);

int client_count = 0;
pthread_mutex_t mutex;

int main(int argc, char *argv[])
{
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_size;
    int socket_option = 1;
    pthread_t thread_id[MAX_CLIENT] = {0};
    int str_len = 0;
    int i;
    char idpasswd[(ID_SIZE * 2) + 100];
    char *pToken;
    char *pArray[ARRAY_COUNT] = {0};
    char msg[BUF_SIZE];

    CLIENT_INFO client_info[MAX_CLIENT];
    load_idpasswd_db(client_info, MAX_CLIENT);

    if (argc != 2)
    {
        char buf[100];
        sprintf(buf, "Usage : %s <port>\n", argv[0]);
        fputs(buf, stderr);
        exit(1);
    }

    fputs("Server Start!!\n", stdout);

    if (pthread_mutex_init(&mutex, NULL))
        error_handling("mutex init error");

    server_socket = socket(PF_INET, SOCK_STREAM, 0);

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(atoi(argv[1]));

    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (void *)&socket_option, sizeof(socket_option));

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
        error_handling("bind() error");

    if (listen(server_socket, 5) == -1)
        error_handling("listen() error");

    while (1)
    {
        client_address_size = sizeof(client_address);
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_size);

        if (client_count >= MAX_CLIENT)
        {
            fputs("socket full\n", stderr);
            shutdown(client_socket, SHUT_WR);
            continue;
        }
        else if (client_socket < 0)
        {
            perror("accept()");
            continue;
        }

        str_len = read(client_socket, idpasswd, sizeof(idpasswd) - 1);
        if (str_len <= 0)
        {
            shutdown(client_socket, SHUT_WR);
            close(client_socket);
            continue;
        }
        idpasswd[str_len] = '\0';

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        if (strncmp(idpasswd, "[SIGNUP:", 8) == 0)
        {
            // 파싱
            char signup_id[ID_SIZE] = {0}, signup_pw[ID_SIZE] = {0};
            sscanf(idpasswd, "[SIGNUP:%9[^:]:%9[^]]]", signup_id, signup_pw);

            // DB 연결 및 중복 체크
            MYSQL *conn = mysql_init(NULL);
            if (conn == NULL)
                error_handling("mysql_init() failed");

            if (mysql_real_connect(conn, "127.0.0.1", "ubuntu", "mariadb", "account", 0, NULL, 0) == NULL)
                sql_error(conn);

            char query[256];
            snprintf(query, sizeof(query), "SELECT id FROM idpasswd WHERE id='%s'", signup_id);
            if (mysql_query(conn, query))
                sql_error(conn);

            MYSQL_RES *result = mysql_store_result(conn);
            if (result == NULL)
                sql_error(conn);

            // 이미 존재하면 실패 메시지 송신
            if (mysql_num_rows(result) > 0)
            {
                char fail_msg[] = "SIGNUP failed (ID already exists)\n";
                write(client_socket, fail_msg, strlen(fail_msg));
                shutdown(client_socket, SHUT_WR);
                mysql_free_result(result);
                mysql_close(conn);
                close(client_socket);
                continue;
            }
            mysql_free_result(result);

            // 신규 가입 DB 등록
            snprintf(query, sizeof(query), "INSERT INTO idpasswd(id, password) VALUES('%s','%s')", signup_id, signup_pw);
            if (mysql_query(conn, query))
                sql_error(conn);
            mysql_close(conn);

            // 클라이언트 정보 할당
            pthread_mutex_lock(&mutex);

            int idx = -1;
            for (i = 0; i < MAX_CLIENT; i++)
            {
                if (client_info[i].fd == -1)
                {
                    idx = i;
                    break;
                }
            }

            if (idx == -1)
            {
                char fail_msg[] = "No slot for new client\n";
                write(client_socket, fail_msg, strlen(fail_msg));
                shutdown(client_socket, SHUT_WR);
                close(client_socket);
                pthread_mutex_unlock(&mutex);
                continue;
            }

            client_info[idx].fd = client_socket;
            client_info[idx].index = idx;

            strncpy(client_info[idx].ip, inet_ntoa(client_address.sin_addr), sizeof(client_info[idx].ip) - 1);
            client_info[idx].ip[sizeof(client_info[idx].ip) - 1] = '\0';

            strncpy(client_info[idx].id, signup_id, ID_SIZE - 1);
            client_info[idx].id[ID_SIZE - 1] = '\0';

            strncpy(client_info[idx].pw, signup_pw, ID_SIZE - 1);
            client_info[idx].pw[ID_SIZE - 1] = '\0';

            client_count++;
            pthread_mutex_unlock(&mutex);

            // 가입 성공 메시지 전송 및 스레드 생성
            snprintf(msg, sizeof(msg), "SIGNUP success. [%s] login OK\n", signup_id);
            write(client_socket, msg, strlen(msg));
            log_file(msg);

            pthread_create(thread_id + idx, NULL, client_connection, (void *)(client_info + idx));
            pthread_detach(thread_id[idx]);
            continue;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        i = 0;
        pToken = strtok(idpasswd, "[:]");

        while (pToken != NULL)
        {
            pArray[i] = pToken;
            if (i++ >= ARRAY_COUNT)
                break;
            pToken = strtok(NULL, "[:]");
        }

        for (i = 0; i < MAX_CLIENT; i++)
        {
            if (!strcmp(client_info[i].id, pArray[0]))
            {
                if (client_info[i].fd != -1)
                {
                    snprintf(msg, sizeof(msg), "[%s] Already logged!\n", pArray[0]);

                    ssize_t ret = write(client_socket, msg, strlen(msg));
                    if (ret == -1)
                        perror("write error");

                    log_file(msg);
                    shutdown(client_socket, SHUT_WR);

                    client_info[i].fd = -1; // mcu에서 fd 처리

                    break;
                }

                if (!strcmp(client_info[i].pw, pArray[1]))
                {
                    strncpy(client_info[i].ip, inet_ntoa(client_address.sin_addr), sizeof(client_info[i].ip) - 1);
                    client_info[i].ip[sizeof(client_info[i].ip) - 1] = '\0';

                    pthread_mutex_lock(&mutex);
                    client_info[i].index = i;
                    client_info[i].fd = client_socket;
                    client_count++;
                    pthread_mutex_unlock(&mutex);

                    snprintf(msg, sizeof(msg), "[%s] New connected! (ip:%s,fd:%d,socket:%d)\n", pArray[0], client_info[i].ip, client_socket, client_count);
                    log_file(msg);

                    ssize_t ret = write(client_socket, msg, strlen(msg));
                    if (ret == -1)
                        perror("write error");

                    pthread_create(thread_id + i, NULL, client_connection, (void *)(client_info + i));
                    pthread_detach(thread_id[i]);
                    break;
                }
            }
        }
        if (i == MAX_CLIENT)
        {
            snprintf(msg, sizeof(msg), "[%s] Authentication Error!\n", pArray[0]);

            ssize_t ret = write(client_socket, msg, strlen(msg));
            if (ret == -1)
                perror("write error");

            log_file(msg);
            shutdown(client_socket, SHUT_WR);
        }
    }
    return 0;
}

void *client_connection(void *arg)
{
    CLIENT_INFO *client_info = (CLIENT_INFO *)arg;
    int str_len = 0;
    int index = client_info->index;
    char msg[BUF_SIZE];
    char to_msg[BUF_SIZE];
    int i = 0;
    char *pToken;
    char *pArray[ARRAY_COUNT] = {0};
    char str_buf[BUF_SIZE * 2] = {0};

    MSG_INFO msg_info;
    CLIENT_INFO *first_client_info;

    first_client_info = (CLIENT_INFO *)((void *)client_info - (void *)(sizeof(CLIENT_INFO) * index));

    while (1)
    {
        memset(msg, 0x0, sizeof(msg));
        str_len = read(client_info->fd, msg, sizeof(msg) - 1);
        if (str_len <= 0)
            break;

        msg[str_len] = '\0';
        pToken = strtok(msg, "[:]");
        i = 0;
        while (pToken != NULL)
        {
            pArray[i] = pToken;
            if (i++ >= ARRAY_COUNT)
                break;
            pToken = strtok(NULL, "[:]");
        }

        memset(&msg_info, 0, sizeof(MSG_INFO));
        msg_info.fd = client_info->fd;
        strncpy(msg_info.from, client_info->id, ID_SIZE - 1);
        msg_info.from[ID_SIZE - 1] = '\0';
        if (pArray[0])
        {
            strncpy(msg_info.to, pArray[0], ID_SIZE - 1);
            msg_info.to[ID_SIZE - 1] = '\0';
        }
        else
        {
            msg_info.to[0] = '\0';
        }
        if (pArray[1])
        {
            snprintf(to_msg, sizeof(to_msg), "[%s]%s", msg_info.from, pArray[1]);
            strncpy(msg_info.msg, to_msg, BUF_SIZE - 1);
            msg_info.msg[BUF_SIZE - 1] = '\0';
        }
        else
        {
            msg_info.msg[0] = '\0';
        }
        msg_info.len = strlen(msg_info.msg);
        snprintf(str_buf, sizeof(str_buf), "msg : [%s->%s] %s", msg_info.from, msg_info.to, pArray[1] ? pArray[1] : "");
        log_file(str_buf);
        send_msg(&msg_info, first_client_info);
    }

    close(client_info->fd);

    snprintf(str_buf, sizeof(str_buf), "Disconnect ID:%s (ip:%s,fd:%d,sockcnt:%d)\n", client_info->id, client_info->ip, client_info->fd, client_count - 1);
    log_file(str_buf);

    pthread_mutex_lock(&mutex);
    client_count--;
    client_info->fd = -1;
    pthread_mutex_unlock(&mutex);

    return 0;
}

void send_msg(MSG_INFO *msg_info, CLIENT_INFO *first_client_info)
{
    int i = 0;

    if (!strcmp(msg_info->to, "ALLMSG"))
    {
        for (i = 0; i < MAX_CLIENT; i++)
        {
            if ((first_client_info + i)->fd != -1)
            {
                ssize_t ret = write((first_client_info + i)->fd, msg_info->msg, msg_info->len);
                if (ret == -1)
                    perror("write error");
            }
        }
    }
    else if (!strcmp(msg_info->to, "IDLIST"))
    {
        char *idlist = (char *)malloc(ID_SIZE * MAX_CLIENT);
        msg_info->msg[strlen(msg_info->msg) - 1] = '\0';
        strcpy(idlist, msg_info->msg);

        for (i = 0; i < MAX_CLIENT; i++)
        {
            if ((first_client_info + i)->fd != -1)
            {
                strcat(idlist, (first_client_info + i)->id);
                strcat(idlist, " ");
            }
        }
        strcat(idlist, "\n");

        ssize_t ret = write(msg_info->fd, idlist, strlen(idlist));
        if (ret == -1)
            perror("write error");

        free(idlist);
    }
    else if (!strcmp(msg_info->to, "GETTIME"))
    {
        sleep(1);
        get_localtime(msg_info->msg);

        ssize_t ret = write(msg_info->fd, msg_info->msg, strlen(msg_info->msg));
        if (ret == -1)
            perror("write error");
    }
    else
    {
        for (i = 0; i < MAX_CLIENT; i++)
        {
            if (((first_client_info + i)->fd != -1) && (!strcmp(msg_info->to, (first_client_info + i)->id)))
            {
                ssize_t ret = write((first_client_info + i)->fd, msg_info->msg, msg_info->len);
                if (ret == -1)
                    perror("write error");
            }
        }
    }
}

void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

void log_file(char *msgstr) { fputs(msgstr, stdout); }

void get_localtime(char *buf)
{
    struct tm *t;
    time_t tt;
    char wday[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    tt = time(NULL);
    if (errno == EFAULT)
        perror("time()");
    t = localtime(&tt);
    sprintf(buf, "[GETTIME]%02d.%02d.%02d %02d:%02d:%02d %s", t->tm_year + 1900 - 2000, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, wday[t->tm_wday]);
    return;
}

void load_idpasswd_db(CLIENT_INFO *client_info, int max_clients)
{
    for (int i = 0; i < max_clients; i++)
    {
        client_info[i].index = 0;
        client_info[i].fd = -1;
        strcpy(client_info[i].ip, "");
        strcpy(client_info[i].id, "");
        strcpy(client_info[i].pw, "");
    }
    MYSQL *conn = mysql_init(NULL);

    if (conn == NULL)
    {
        fputs("mysql_init() failed\n", stderr);
        exit(1);
    }

    if (mysql_real_connect(conn, "127.0.0.1", "ubuntu", "mariadb", "account", 0, NULL, 0) == NULL)
    {
        sql_error(conn);
    }

    if (mysql_query(conn, "SELECT id, password FROM idpasswd"))
    {
        sql_error(conn);
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL)
    {
        sql_error(conn);
    }

    int i = 0;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) && i < max_clients)
    {
        client_info[i].index = i;
        if (row[0])
        {
            strncpy(client_info[i].id, row[0], ID_SIZE - 1);
            client_info[i].id[ID_SIZE - 1] = '\0';
        }
        if (row[1])
        {
            strncpy(client_info[i].pw, row[1], ID_SIZE - 1);
            client_info[i].pw[ID_SIZE - 1] = '\0';
        }
        i++;
    }

    mysql_free_result(result);
    mysql_close(conn);
}

void sql_error(MYSQL *conn)
{
    fputs(mysql_error(conn), stderr);
    fputs("\n", stderr);
    mysql_close(conn);
    exit(1);
}
