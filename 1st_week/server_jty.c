// 표준 C 헤더
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

#define BUF_SIZE 100
#define MAX_CLIENT 50
#define ID_SIZE 10
#define ARRAY_COUNT 5

#define DEBUG

typedef struct
{
    char fd;
    char *from;
    char *to;
    char *msg;
    char len;
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
void load_file(const char *filename, CLIENT_INFO *client_info, int max_clients);
void get_localtime(char *buf);

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
    char idpasswd[(ID_SIZE * 2) + 3];
    char *pToken;
    char *pArray[ARRAY_COUNT] = {0};
    char msg[BUF_SIZE];

    // idpasswd.txt 파일에서 로그인 정보 받아오기
    CLIENT_INFO client_info[MAX_CLIENT];
    load_file("idpasswd.txt", client_info, MAX_CLIENT);

    if (argc != 2)
    {
        char buf[100];
        sprintf(buf, "Usage : %s <port>\n", argv[0]);
        fputs(buf, stderr);
    }

    fputs("Server Start!!\n", stdout);

    // pthread_mutex_init 은 성공시 0 반환 실패시 0이 아닌 값 반환
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


    if(listen(server_socket, 5) == -1)
        error_handling("listen() error");

    while(1)
    {
        client_address_size = sizeof(client_address);
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_size);

        if(client_count >= MAX_CLIENT)
        {
            fputs("socket full\n", stderr);
            shutdown(client_socket, SHUT_WR);
            continue;
        }
        else if(client_socket < 0)
        {
            perror("accept()");
            continue;
        }

        str_len = read(client_socket, idpasswd, sizeof(idpasswd));
        idpasswd[str_len] = '\0';

        if (str_len > 0)
        {
            i = 0;
            pToken = strtok(idpasswd, "[:]");
            while(pToken != NULL)
            {
                pArray[i] = pToken;
                if(i++ >= ARRAY_COUNT) break;
                pToken = strtok(NULL, "[:]");
            } // 여기까지 정리해둠

            for(i=0; i < MAX_CLIENT; i++)
            {
                if(!strcmp(client_info[i].id, pArray[0]))
                {
                    if(client_info[i].fd != -1)
                    {
                        sprintf(msg, "[%s] Already logged!\n", pArray[0]);
                        write(client_socket, msg, strlen(msg));
                        log_file(msg);
                        shutdown(client_socket, SHUT_WR);

                        client_info[i].fd = -1; // mcu에서 fd 처리

                        break;
                    }

                    if(!strcmp(client_info[i].pw, pArray[1]))
                    {
                        strcpy(client_info[i].ip, inet_ntoa(client_address.sin_addr));
                        pthread_mutex_lock(&mutex);
                        client_info[i].index = i;
                        client_info[i].fd = client_socket;
                        client_count++;

                        pthread_mutex_unlock(&mutex);
                        sprintf(msg, "[%s] New connected! (ip:%s,fd:%d,socket:%d)\n", pArray[0], inet_ntoa(client_address.sin_addr), client_socket, client_count);
                        log_file(msg);
                        write(client_socket, msg, strlen(msg));

                        pthread_create(thread_id + 1, NULL, client_connection, (void *)(client_info + 1));
                        pthread_detach(thread_id[i]);
                        break;
                    }
                }
            }
            if(i == MAX_CLIENT)
            {
                sprintf(msg, "[%s] Authentication Error!\n", pArray[0]);
                write(client_socket, msg, strlen(msg));
                log_file(msg);
                shutdown(client_socket, SHUT_WR);
            }
        }
        else shutdown(client_socket, SHUT_WR);
    }
    return 0;
}

void *client_connection(void *arg)
{
    CLIENT_INFO *client_info = (CLIENT_INFO *)arg;
    int str_len = 0;
    int index = client_info -> index;
    char msg[BUF_SIZE];
    char to_msg[MAX_CLIENT * ID_SIZE + 1];
    int i=0;
    char *pToken;
    char *pArray[ARRAY_COUNT] = {0};
    char str_buf[BUF_SIZE * 2] = {0};

    MSG_INFO msg_info;
    CLIENT_INFO *first_client_info;
}
