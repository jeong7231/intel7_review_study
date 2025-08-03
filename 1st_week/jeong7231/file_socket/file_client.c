#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>

void error_handling(char *message);

int main(int argc, char* argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    char buf[100]; // 100 바이트 버퍼
    int fd, n;
    
    if(argc != 4) { // 인자 4개
        printf("Usage : %s <IP> <port> <file>\n", argv[0]); // 4번째 <file> argv[3]
        exit(1);
    }
    
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock < 0)
        error_handling("socket() error");
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));
    
    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        error_handling("connect() error!");
    





    fd = open(argv[3], O_RDONLY); 
    if(fd < 0) {
        perror("file open error");
        close(sock);
        exit(1);
    }


    do {
        n = read(fd, buf, sizeof(buf));


        if(n > 0)
            write(sock, buf, n);

    } while(n > 0);
    
    printf("Done..\n");
    


    close(fd);
    close(sock);
    
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

