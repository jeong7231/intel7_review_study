// 표준 C 헤더
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 시스템/OS 헤더
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_SIZE 100
#define NAME_SIZE 20
#define ARR_COUNT 5

void *send_msg(void *arg);
void *receive_msg(void *arg);
void error_handling(char *msg);

char name[NAME_SIZE] = "[Default]";
char msg[BUF_SIZE];

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in server_address;
    pthread_t send_thread, receive_thread;
    void *thread_return;

    // if (argc != 4)
    // {
    //     char buf[100];
    //     sprintf(buf, "Usage : %s <IP> <port> <name>\n", argv[0]);
    //     fputs(buf, stderr);
    // }

    if (!(argc == 4 || (argc == 6 && strcmp(argv[3], "SIGNUP") == 0)))
    {
        char buf[100];
        sprintf(buf, "Usage : %s <IP> <port> <ID> OR %s <IP> <port> SIGNUP <ID> <password>\n", argv[0], argv[0]);
        fputs(buf, stderr);
        exit(1);
    }

    sprintf(name, "%s", argv[3]);

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(argv[1]);
    server_address.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
        error_handling("connect() error");

    // sprintf(msg, "[%s:PASSWD]", name);

    // ssize_t ret = write(sock, msg, strlen(msg));
    // if (ret == -1)
    //     perror("write error");

    /////////////////////////////////////////////////////////////////////////////////

    if (argc == 6 && strcmp(argv[3], "SIGNUP") == 0)
    {
        sprintf(msg, "[SIGNUP:%s:%s]", argv[4], argv[5]);
        write(sock, msg, strlen(msg));

        int recv_len = read(sock, msg, BUF_SIZE - 1);
        msg[recv_len] = 0;
        if (strstr(msg, "SIGNUP failed") != NULL)
        {
            fputs(msg, stdout);
            close(sock);
            exit(1);
        }
        // 성공시 로그인
        strcpy(name, argv[4]);
        sprintf(msg, "[%s:%s]", name, argv[5]);
        write(sock, msg, strlen(msg));
    }
    else if (argc == 4)
    {
        strcpy(name, argv[3]);
        sprintf(msg, "[%s:PASSWD]", name);
        ssize_t ret = write(sock, msg, strlen(msg));
        if (ret == -1)
            perror("write error");
    }

    /////////////////////////////////////////////////////////////////////////////////
    pthread_create(&receive_thread, NULL, receive_msg, (void *)&sock);
    pthread_create(&send_thread, NULL, send_msg, (void *)&sock);

    pthread_join(send_thread, &thread_return);
    pthread_join(receive_thread, &thread_return);

    close(sock);

    return 0;
}

void *send_msg(void *arg)
{
    int *sock = (int *)arg;
    int ret;
    fd_set initset, newset;
    struct timeval tv;
    char name_msg[NAME_SIZE + BUF_SIZE + 2];

    FD_ZERO(&initset);
    FD_SET(STDIN_FILENO, &initset);

    fputs("Input a message! [ID]msg (Default ID:ALLMSG)\n", stdout);

    while (1)
    {
        memset(msg, 0, sizeof(msg));
        name_msg[0] = '\0';
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        newset = initset;
        ret = select(STDIN_FILENO + 1, &newset, NULL, NULL, &tv);

        if (FD_ISSET(STDIN_FILENO, &newset))
        {
            char *fgets_return = fgets(msg, BUF_SIZE, stdin);
            if (fgets_return == NULL)
            {
                if (feof(stdin))
                    exit(0);
                else if (ferror(stdin))
                    perror("fgets error");
            }

            if (!strncmp(msg, "quit\n", 5))
            {
                *sock = -1;
                return NULL;
            }

            else if (msg[0] != '[')
            {
                strcat(name_msg, "[ALLMSG]");
                strcat(name_msg, msg);
            }

            else
                strcpy(name_msg, msg);

            if (write(*sock, name_msg, strlen(name_msg)) <= 0)
            {
                *sock = -1;
                return NULL;
            }
        }
        if (ret == 0)
        {
            if (*sock == -1)
                return NULL;
        }
    }
}

void *receive_msg(void *arg)
{
    int *sock = (int *)arg;

    char name_msg[NAME_SIZE + BUF_SIZE + 1];
    int str_len;

    while (1)
    {
        memset(name_msg, 0x0, sizeof(name_msg));

        str_len = read(*sock, name_msg, NAME_SIZE + BUF_SIZE);

        if (str_len <= 0)
        {
            *sock = -1;
            return NULL;
        }
        name_msg[str_len] = 0;
        fputs(name_msg, stdout);
    }
}

void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}