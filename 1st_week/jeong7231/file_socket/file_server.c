#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h> 


void error_handling(char *message);

int main(int argc, char *argv[])
{
    int serv_sock;
    int clnt_sock;
    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size;
    char buf[100];
    int fd, n;
    
    if(argc!=3) // 3개 받아와야됨 
    {
		printf("Usage : %s <port> <file>\n", argv[0]); // 사용법 출력 
		exit(1);
	}
    
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(serv_sock < 0)
        error_handling("socket() error");
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));
    
    if(bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
        error_handling("bind() error");
    
    if(listen(serv_sock, 5) < 0)
        error_handling("listen() error");
    
    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
    if(clnt_sock < 0)
        error_handling("accept() error");
    
 


    fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if(fd < 0)
        error_handling("file open error");
    

    do {
        n = read(clnt_sock, buf, sizeof(buf));

        printf("read : %d\n", n); 

        if(n > 0)
            write(fd, buf, n);

    } while(n > 0);
    
    
    printf("Done..\n");


    close(fd);
    close(clnt_sock);
    close(serv_sock);
    
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

