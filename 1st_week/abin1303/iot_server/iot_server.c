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
#include <mysql/mysql.h>

#define BUF_SIZE 100
#define MAX_CLNT 32
#define ID_SIZE 10
#define ARR_CNT 5

#define DEBUG
typedef struct
{
	char fd;
	char *from;
	char *to;
	char *msg;
	int len;
} MSG_INFO;

typedef struct
{
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
void getlocaltime(char *buf);
void update_client_info(MYSQL *conn, CLIENT_INFO *client_info);

int clnt_cnt = 0;
pthread_mutex_t mutx;

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_sz;
	int sock_option = 1;
	pthread_t t_id[MAX_CLNT] = {0};
	int str_len = 0;
	int i = 0;
	char idpasswd[(ID_SIZE * 2) + 3];
	char *pToken;
	char *pArray[ARR_CNT] = {0};
	char msg[BUF_SIZE];
	//-------------------------------------
	MYSQL *conn;
	MYSQL_RES *sqlres;
	MYSQL_ROW sqlrow;
	int res;
	char sql_cmd[200] = {0};
	char *host = "localhost";
	char *user = "iot";
	char *pass = "pwiot";
	char *dbname = "user_db";

	conn = mysql_init(NULL);
	// puts("MYSQL startup");
	if (!(mysql_real_connect(conn, host, user, pass, dbname, 0, NULL, 0)))
	{
		fprintf(stderr, "ERROR : %s[%d]\n", mysql_error(conn), mysql_errno(conn));
		exit(1);
	}
	else
		printf("Connection Successful!\n\n");
	//-------------------------------------
	char id[ID_SIZE];
	char pw[ID_SIZE];
	CLIENT_INFO *client_info = (CLIENT_INFO *)calloc(sizeof(CLIENT_INFO), MAX_CLNT);
	if (client_info == NULL)
	{
		perror("calloc()");
		exit(1);
	}
	//---------------------------------------------------------------
	update_client_info(conn, client_info);
	//---------------------------------------------------------------
	if (argc != 2)
	{
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	fputs("IoT Server Start!!\n", stdout);

	if (pthread_mutex_init(&mutx, NULL))
		error_handling("mutex init error");

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

	setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (void *)&sock_option, sizeof(sock_option));
	if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");

	if (listen(serv_sock, 5) == -1)
		error_handling("listen() error");

	int db_flag = 0;
	int ans = 0;
	while (1)
	{
		// for (int i = 0; i < MAX_CLNT; i++)
		// {
		// 	printf("[%d] fd=%d, id=%s\n", i, client_info[i].fd, client_info[i].id);
		// }

		clnt_adr_sz = sizeof(clnt_adr);
		clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
		if (clnt_cnt >= MAX_CLNT)
		{
			printf("socket full\n");
			shutdown(clnt_sock, SHUT_WR);
			continue;
		}
		else if (clnt_sock < 0)
		{
			perror("accept()");
			continue;
		}

		str_len = read(clnt_sock, idpasswd, sizeof(idpasswd));
		idpasswd[str_len] = '\0';
		//printf(">>>>[debug] idpasswd : %s\n", idpasswd);

		if (str_len > 0)
		{
			i = 0;
			pToken = strtok(idpasswd, "[:]");

			while (pToken != NULL)
			{
				pArray[i] = pToken;
				if (i++ >= ARR_CNT)
					break;
				pToken = strtok(NULL, "[:]");
			}
			for (i = 0; i < MAX_CLNT; i++)
			{
				if (!strcmp(client_info[i].id, pArray[0])) // db계정과 일치
				{
					if (client_info[i].fd != -1)
					{
						sprintf(msg, "[%s] Already logged!\n", pArray[0]);
						write(clnt_sock, msg, strlen(msg));
						log_file(msg);
						shutdown(clnt_sock, SHUT_WR);
#if 1 // for MCU
						client_info[i].fd = -1;
#endif
						break;
					}
					if (!strcmp(client_info[i].pw, pArray[1])) // pw확인
					{

						strcpy(client_info[i].ip, inet_ntoa(clnt_adr.sin_addr));
						pthread_mutex_lock(&mutx);
						client_info[i].index = i;
						client_info[i].fd = clnt_sock;
						clnt_cnt++;
						pthread_mutex_unlock(&mutx);
						sprintf(msg, "[%s] New connected! (ip:%s,fd:%d,sockcnt:%d)\n", pArray[0], inet_ntoa(clnt_adr.sin_addr), clnt_sock, clnt_cnt);
						log_file(msg);
						write(clnt_sock, msg, strlen(msg));

						pthread_create(t_id + i, NULL, clnt_connection, (void *)(client_info + i));
						pthread_detach(t_id[i]);

						db_flag = 1; // db의 id , pw와 일치
						break;
					}
				}
			}
			if (i == MAX_CLNT) // db와 불일치 -> 계정 생성
			{
#if 1
				sprintf(msg, "[%s] Authentication Error!\n", pArray[0]);
				write(clnt_sock, msg, strlen(msg));
				// log_file(msg);
				//----------------------------------------------
				// sprintf(msg, ">>>>>>>>>> Sign Up ? (yes: 1, No : 0) \n");
				write(clnt_sock, msg, strlen(msg));
				log_file(msg);
				// scanf("%d", &ans);
				// if(ans == 0)
				// shutdown(clnt_sock,SHUT_WR);

				sprintf(sql_cmd, "insert into idpasswd_info values ('%s', '%s')", pArray[0], pArray[1]); // 계정 생성
				res = mysql_query(conn, sql_cmd);
				if (!res)
				{
					printf("inserted %lu rows\n", (unsigned long)mysql_affected_rows(conn));
					update_client_info(conn, client_info);
					sprintf(msg, "[%s] Sign Up Success !\n", pArray[0]);
					write(clnt_sock, msg, strlen(msg));
					log_file(msg);
				}
				else // query 에러 처리
					fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn), mysql_errno(conn));

				for (i = 0; i < MAX_CLNT; i++)
				{
					if (!strcmp(client_info[i].id, pArray[0])) // db계정과 일치 확인
					{
						if (!strcmp(client_info[i].pw, pArray[1])) // pw확인
						{
							strcpy(client_info[i].ip, inet_ntoa(clnt_adr.sin_addr));
							pthread_mutex_lock(&mutx);
							client_info[i].index = i;
							client_info[i].fd = clnt_sock;
							clnt_cnt++;
							pthread_mutex_unlock(&mutx);
							sprintf(msg, "[%s] New connected! (ip:%s,fd:%d,sockcnt:%d)\n", pArray[0], inet_ntoa(clnt_adr.sin_addr), clnt_sock, clnt_cnt);
							log_file(msg);
							write(clnt_sock, msg, strlen(msg));

							pthread_create(t_id + i, NULL, clnt_connection, (void *)(client_info + i));
							pthread_detach(t_id[i]);

							db_flag = 1; // db의 id , pw와 일치
							break;
						}
					}
				}
//----------------------------------------------
// shutdown(clnt_sock,SHUT_WR);
#endif
			}
		}
		else
			shutdown(clnt_sock, SHUT_WR);
	}



	
	mysql_free_result(sqlres);
	mysql_close(conn);
	return 0;
}

void *clnt_connection(void *arg)
{
	CLIENT_INFO *client_info = (CLIENT_INFO *)arg;
	int str_len = 0;
	int index = client_info->index;
	char msg[BUF_SIZE];
	char to_msg[MAX_CLNT * ID_SIZE + 1];
	int i = 0;
	char *pToken;
	char *pArray[ARR_CNT] = {0};
	char strBuff[BUF_SIZE * 2] = {0};

	MSG_INFO msg_info;
	CLIENT_INFO *first_client_info;

	first_client_info = (CLIENT_INFO *)((void *)client_info - (void *)(sizeof(CLIENT_INFO) * index));
	//first_client_info = client_info;
	// char* test = first_client_info;
	// printf("first_client_info : %s \n", test);

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
			if (i++ >= ARR_CNT)
				break;
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

void send_msg(MSG_INFO *msg_info, CLIENT_INFO *first_client_info)
{
	int i = 0;

	if (!strcmp(msg_info->to, "ALLMSG"))
	{
		for (i = 0; i < MAX_CLNT; i++)
			if ((first_client_info + i)->fd != -1)
				write((first_client_info + i)->fd, msg_info->msg, msg_info->len);
	}
	else if (!strcmp(msg_info->to, "IDLIST"))
	{
		char *idlist = (char *)malloc(ID_SIZE * MAX_CLNT);
		msg_info->msg[strlen(msg_info->msg) - 1] = '\0';
		strcpy(idlist, msg_info->msg);

		for (i = 0; i < MAX_CLNT; i++)
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
	else if (!strcmp(msg_info->to, "GETTIME"))
	{
		sleep(1);
		getlocaltime(msg_info->msg);
		write(msg_info->fd, msg_info->msg, strlen(msg_info->msg));
	}
	else
		for (i = 0; i < MAX_CLNT; i++)
			if ((first_client_info + i)->fd != -1)
				if (!strcmp(msg_info->to, (first_client_info + i)->id))
					write((first_client_info + i)->fd, msg_info->msg, msg_info->len);
}

void error_handling(char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

void log_file(char *msgstr)
{
	fputs(msgstr, stdout);
}
void getlocaltime(char *buf)
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

void update_client_info(MYSQL *conn, CLIENT_INFO *client_info)
{
	MYSQL_RES *sqlres;
	MYSQL_ROW sqlrow;
	int res;
	char sql_cmd[200] = {0};
	int i = 0;

	sprintf(sql_cmd, "select ID, Passwd from idpasswd_info");
	res = mysql_query(conn, sql_cmd);

	if (!res)
	{
		sqlres = mysql_store_result(conn);
		if (sqlres == NULL)
		{ // 테이블에 ID, Passwd가 없을 때 에러 처리
			fprintf(stderr, "%s\n", mysql_error(conn));
			exit(1);
		}

		while ((sqlrow = mysql_fetch_row(sqlres)) != NULL)
		{
			//printf("id : %s , pw : %s \n", sqlrow[0], sqlrow[1]);
			strcpy(client_info[i].id, sqlrow[0]);
			strcpy(client_info[i].pw, sqlrow[1]);
			//printf("[copy] id : %s , pw : %s \n", client_info[i].id, client_info[i].pw);
			client_info[i].fd = -1;
			i++;

			if (i > MAX_CLNT)
			{
				printf("error client_info pull(Max:%d)\n", MAX_CLNT);
				break;
			}
		}
	}
	else // query 에러 처리
		fprintf(stderr, "%s\n", mysql_error(conn));

	return;
}