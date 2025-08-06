/* 서울기술교육센터 AIoT */
/* author : KSH */
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

int clnt_cnt=0;
int count = 0;
pthread_mutex_t mutx;

int main(int argc, char *argv[])
{
		FILE* fp;
		char* path = "idpasswd.txt";
		char tmp;
		//int count = 0;
		fp = fopen(path, "r");
		if (fp == NULL) {
			perror("fopen()");
			exit(1);
		}
		while(fscanf(fp, "%c", &tmp) != EOF){
			if (tmp == ' '){
				count++;
			}
		}
		//printf("%d\n", count);
		fseek(fp, 0, SEEK_SET);
		
		int serv_sock, clnt_sock;
		struct sockaddr_in serv_adr, clnt_adr;
		int clnt_adr_sz;
		int sock_option  = 1;
		pthread_t t_id[MAX_CLNT] = {0};
		int str_len = 0;
		int i;
		char idpasswd[(ID_SIZE*2)+3];
		char *pToken;
		char *pArray[ARR_CNT]={0};
		char msg[BUF_SIZE];
		
		CLIENT_INFO* client_info;
		client_info = (CLIENT_INFO *)calloc(sizeof(CLIENT_INFO), count);
		if (client_info == NULL) {
			perror("calloc()");
			exit(2);
		}
		char id[ID_SIZE];
		char pw[ID_SIZE];

		for (int i = 0; i < count; i++){
			client_info[i].index = 0;
			client_info[i].fd = -1;
			strcpy(client_info[i].ip, "");
			fscanf(fp, "%s", id);
			strcpy(client_info[i].id, id);
			fscanf(fp, "%s", pw);
			strcpy(client_info[i].pw, pw);
			//printf("%s %s\n", client_info[i].id, client_info[i].pw);
		}


		if(argc != 2) {
				printf("Usage : %s <port>\n",argv[0]);
				exit(1);
		}
		fputs("IoT Server Start!!\n",stdout);

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
				if(clnt_cnt >= count)
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
						for(i=0;i<count;i++)
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
						if(i == count)
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
		free(client_info);
		fclose(fp);

		return 0;
}

void * clnt_connection(void *arg)
{
    CLIENT_INFO * client_info = (CLIENT_INFO *)arg;
    int str_len = 0;
    int index = client_info->index;
    char msg[BUF_SIZE];
    char to_msg[MAX_CLNT*ID_SIZE+1];
    int i = 0;
    char *pToken;
    char *pArray[ARR_CNT] = {0};
    char strBuff[BUF_SIZE*2] = {0};

    MSG_INFO msg_info;
    CLIENT_INFO * first_client_info;

    first_client_info = (CLIENT_INFO *)((void *)client_info - sizeof(CLIENT_INFO) * index);

    while (1) {
        memset(msg, 0x0, sizeof(msg));
        str_len = read(client_info->fd, msg, sizeof(msg) - 1);
        if (str_len <= 0) break;

        msg[str_len] = '\0';

        // [SIGNUP], [CHPASS], [DELETE] 명령어 처리
        if (msg[0] == '[' && strstr(msg, "]") && strstr(msg, "@")) {
            char command[20] = {0};
            char *body = strchr(msg, ']');
            if (body != NULL) {
                strncpy(command, msg + 1, body - msg - 1);
                body++;

                char *id = strtok(body, "@");
                char *pw = strtok(NULL, "@");

                if (id) id[strcspn(id, "\r\n")] = 0;
                if (pw) pw[strcspn(pw, "\r\n")] = 0;

                // --- SIGNUP ---
                if (!strcmp(command, "SIGNUP")) {
                    if (strcmp(client_info->id, "MASTER") != 0) {
                        write(client_info->fd, "[SERVER] Only MASTER can SIGNUP\n", 33);
                        continue;
                    }
                    FILE *fp = fopen("idpasswd.txt", "a+");
                    if (!fp) {
                        write(client_info->fd, "[SERVER] File open error\n", 27);
                        continue;
                    }

                    // ID 중복 확인
                    rewind(fp);
                    char temp_id[ID_SIZE], temp_pw[ID_SIZE];
                    int exists = 0;
                    while (fscanf(fp, "%s %s", temp_id, temp_pw) != EOF) {
                        if (!strcmp(temp_id, id)) {
                            exists = 1;
                            break;
                        }
                    }

                    if (exists) {
                        write(client_info->fd, "[SERVER] ID already exists\n", 30);
                    } else {
                        fseek(fp, -1, SEEK_END);
                        int ch = fgetc(fp);
                        if (ch != '\n') fputc('\n', fp);
                        fprintf(fp, "%s %s\n", id, pw);
                        write(client_info->fd, "[SERVER] SIGNUP success\n", 27);
                    }
                    fclose(fp);
                    continue;
                }

                // --- CHPASS ---
                else if (!strcmp(command, "CHPASS")) {
    				if (strcmp(client_info->id, id) != 0 && strcmp(client_info->id, "MASTER") != 0) {
        				write(client_info->fd, "[SERVER] Permission denied.\n", 31);
        				continue;
    				}

    				FILE *fp = fopen("idpasswd.txt", "r");
    				FILE *fp_tmp = fopen("idpasswd_tmp.txt", "w");
				    if (!fp || !fp_tmp) {
        				write(client_info->fd, "[SERVER] File open error.\n", 30);
        				continue;
    				}

				    char temp_id[ID_SIZE], temp_pw[ID_SIZE];
    				int changed = 0;
    				int same_as_old = 0;

				    while (fscanf(fp, "%s %s", temp_id, temp_pw) != EOF) {
        				if (!strcmp(temp_id, id)) {
            			// 현재 비밀번호랑 새로운 비밀번호가 같으면 변경하지 않음
            				if (!strcmp(temp_pw, pw)) {
                				same_as_old = 1;
                				break;
            				}
            				fprintf(fp_tmp, "%s %s\n", temp_id, pw);
            				changed = 1;
				        } else {
            				fprintf(fp_tmp, "%s %s\n", temp_id, temp_pw);
        				}
    				}

    				fclose(fp);
    				fclose(fp_tmp);

    				if (same_as_old) {
        				remove("idpasswd_tmp.txt");
        				write(client_info->fd, "[SERVER] New password is same as current.\n", 44);
				        continue;
    				}

    				if (changed) {
				        rename("idpasswd_tmp.txt", "idpasswd.txt");
        				write(client_info->fd, "[SERVER] Password changed.\n", 30);
    				} else {
        				remove("idpasswd_tmp.txt");
        				write(client_info->fd, "[SERVER] ID not found.\n", 27);
    				}
    				continue;
				}
                // --- DELETE ---
                else if (!strcmp(command, "DELETE")) {
                    if (strcmp(client_info->id, id) != 0 && strcmp(client_info->id, "MASTER") != 0) {
                        write(client_info->fd, "[SERVER] Permission denied\n", 30);
                        continue;
                    }

                    FILE *fp = fopen("idpasswd.txt", "r");
                    FILE *fp_tmp = fopen("idpasswd_tmp.txt", "w");
                    if (!fp || !fp_tmp) {
                        write(client_info->fd, "[SERVER] File open error\n", 27);
                        continue;
                    }

                    char temp_id[ID_SIZE], temp_pw[ID_SIZE];
                    int deleted = 0;
                    while (fscanf(fp, "%s %s", temp_id, temp_pw) != EOF) {
                        if (!strcmp(temp_id, id) && !strcmp(temp_pw, pw)) {
                            deleted = 1;
                            continue;
                        }
                        fprintf(fp_tmp, "%s %s\n", temp_id, temp_pw);
                    }
                    fclose(fp); fclose(fp_tmp);
                    rename("idpasswd_tmp.txt", "idpasswd.txt");

                    if (deleted)
                        write(client_info->fd, "[SERVER] Account deleted\n", 27);
                    else
                        write(client_info->fd, "[SERVER] Delete failed\n", 26);

                    continue;
                }
            }
        }

        // ===== 기존 메시지 처리 유지 =====
        pToken = strtok(msg, "[:]");
        i = 0;
        while (pToken != NULL) {
            pArray[i] = pToken;
            if (i++ >= ARR_CNT) break;
            pToken = strtok(NULL, "[:]");
        }

        msg_info.fd = client_info->fd;
        msg_info.from = client_info->id;
        msg_info.to = pArray[0];
        sprintf(to_msg, "[%s]%s", msg_info.from, pArray[1]);
        msg_info.msg = to_msg;
        msg_info.len = strlen(to_msg);

        sprintf(strBuff, "msg : [%s->%s] %s", msg_info.from, msg_info.to, pArray[1]);
        log_file(strBuff);
        send_msg(&msg_info, first_client_info);
    }

    close(client_info->fd);
    sprintf(strBuff, "Disconnect ID:%s (ip:%s,fd:%d,sockcnt:%d)\n", client_info->id, client_info->ip, client_info->fd, clnt_cnt - 1);
    log_file(strBuff);

    pthread_mutex_lock(&mutx);
    clnt_cnt--;
    client_info->fd = -1;
    pthread_mutex_unlock(&mutx);

    return 0;
}

void send_msg(MSG_INFO * msg_info, CLIENT_INFO * first_client_info)
{
	int i = 0;

	if (!strcmp(msg_info->to, "ALLMSG"))
	{
		for (i = 0; i < count; i++)  // <-- 여기 수정
		{
			if ((first_client_info + i)->fd != -1)
				write((first_client_info + i)->fd, msg_info->msg, msg_info->len);
		}
	}
	else if (!strcmp(msg_info->to, "IDLIST"))
	{
		char *idlist = (char *)malloc(ID_SIZE * count + 100);  // 넉넉하게
		msg_info->msg[strlen(msg_info->msg) - 1] = '\0';
		strcpy(idlist, msg_info->msg);

		for (i = 0; i < count; i++)  // <-- 여기 수정
		{
			if ((first_client_info + i)->fd != -1)
			{
				strcat(idlist, (first_client_info + i)->id);
				strcat(idlist, " ");
			}
		}
		strcat(idlist, "\n");
		write(msg_info->fd, idlist, strlen(idlist));
		free(idlist);
	}
	else
	{
		for (i = 0; i < count; i++)  // <-- 여기 수정
		{
			if ((first_client_info + i)->fd != -1 &&
				!strcmp(msg_info->to, (first_client_info + i)->id))
			{
				write((first_client_info + i)->fd, msg_info->msg, msg_info->len);
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

void log_file(char * msgstr)
{
		fputs(msgstr,stdout);
}
