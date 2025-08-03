# 기존 코드 분석
## 메인 함수

```c
pthread_mutex_init(&mutex, NULL)
```
- `mutex` 객체를 생성/초기화 하는 함수
- 여러 스레드가 동시에 접근하는 공유 자원 보호 위해 사용
- 성공 시 `0` 반환, 실패 시 `0`이 아닌 값 반환

<br>

```c
server_socket = socket(PF_INET, SOCK_STREAM, 0);
```
- `socket` : socket 생성 함수


- `PF_INET` : IPv4 인터넷 프로토콜 패밀리, `AF_INET`와 같은 값, 주로 IP 주소 체계 지정


- `SOCK_STREAM` : 스트림 소켓, TCP(연결지향, 신뢰성 보장) 통신 방식 지정


- `0` : 사용되는 프로토콜 지정, 0을 주면 PF/TYPE에 맞는 기본 프로토콜 사용, 여기선 IPPROTO_TCP 와 동일

- 반환값 : 성공 시 소켓 디스크립터(정수) 반환, 실패시 -1 반환

<br>

```c
server_address.sin_family = AF_INET;
```
- 소켓 주소 체계 지정
- IPv4를 의미
- 항상 `AF_INET`으로 설정

<br>

```c
server_address.sin_addr.s_addr = htonl(INADDR_ANY);
```

- 서버가 바인딩할 IP 주소 지정
- `INADDR_ANY`는 모든 네트워크 인터페이스를 의미
- `htonl()`함수는 호스트 바이트 오더를 네트워크 바이트 오더로 변환
- 즉, 모든 IP에서 오는 요청을 받을 수 있도록 설정

<br>

```c
server_address.sin_port = htons(atoi(argv[1]));
```

- 포트 번호 지정
- `argv[1]` 에서 받은 문자열을 `atoi()`함수를 사용해서 정수로 변환
- `htons()`함수는 호스트 바이트 오더를 네트워크 바이트 오더로 변환 
- 즉, 서버가 수신할 포트 번호 설정

<br>

|`htonl`|`htons`|
|:-|:-|
|Host To Network Lon|Host To Network Short|
|32bit  IP 주소용|16bit 포트 번호용|

<br>

`htonl`, `htons` 함수는 호스트 바이 오더를 (리틀엔디안 빅엔디안 상관없이) 네트워크 바이 오더(빅 엔디안) 로 변환한다.

<br>

```c
setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (void *)&socket_option, sizeof(socket_option));
```

- `server_socket` : 옵션을 설정할 소켓 디스크립터
- `SOL_SOCKET` : 소켓 레벨에서 옵션 설정(일반 소켓 옵션)
- `SO_REUSEADDR` : 주소 재사용 옵션, 이미 사용 중인 주소(포트)에 대해 즉시 재사용 가능하게 함
- `(void *)&socket_option` : 옵션 값의 포인터, 대개 `int socket_option = 1;` 처럼 1로 설정(사용 활성화)
- `sizeof(socket_option)` : 옵션 값의 크기
- 서버 소켓을 종류 후 곧바로 재시작 해도 "주소 이미 사용중" 에러 발생 방지
- TIME_WAIT 상태일 때도 같은 포트로 바인드 가능
- 같은 IP/포트에 대해 빠르게 재사용 가능하도록 설정하는 서버 개발에서 거의 필수적으로 추가하는 설정

<br>

```c
bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));
```

- `server_socket` : 바인딩 할 소켓의 디스키립터(생성된 소켓 핸들)
- `(struct sockaddr *)&server_address` : 주소 정보가 들어 있는 구조체 포인터, `struct sockaddr_in`을 `struct sockaddr*`로 캐스팅, 내부에 IP, 포트 등 정보 포함
- `sizeof(server_address)` : 주소 구조체의 크기
- 해당 소켓이 어떤 IP와 포트 번호로 통신할지 결정하는 서버에서 반드시 거쳐야 하는 단계, 이후 `listen` `accept` 등 연결 처리 가능
- 성공 시 `0`반환, 실패 시 `-1` 반환(이미 사용 중인 포트, 권한 없음 등)

<br>

```c
listen(server_socket, 5);
```

- `server_socket` : 대기 상태로 만들 소켓 디스크립터(이비 `bind`된 상태여야 함)
- `5` : 동시에 대기할 수 있는(백로그) 연결 요청의 최대 개수, 연결 요청이 5개 초과시 추가 요청은 거부되거나 큐에서 대기
- 소켓을 클라이언트 연결 요청을 받을 수 있는 상태로 만듦, 서버 소켓의 "Passive Open" 단계
- 성공 시 `0` 반환, 실패 시 `-1` 반환(`bind` 안했거나 잘못된 인자 등)
- `listen` 이후 `accept`로 실제 연결 처리 가능

<br>

```c
accept(server_socket, (struct sockaddr *)&client_address, &client_address_size);
```

- `server_socket` : 연결 요청 대기중인 서버 소켓 디스크립터
- `(struct sockaddr *)&client_address` : 연결이 성공한 클라이언트 주소 정보(IP, 포트)를 저장할 구조체 포인터
- `&client_address_size` : 주소 구조체의 크기를 저장하는 변소의 포인터, 호출 전에는 구조체 크기, 호출 후에는 실제 저장된 크기
- 성공 시 클라이언트와 통신에 사용할 새 소켓 디스크립터 반환, 실패시 `-1` 반환
- 연결 요청이 올 때까지 블로킹
- 연결이 오면 새 소켓 생성, 그 소켓으로 클라이언트와 데이터 송수신

<br>

```c
read(client_socket, idpasswd, sizeof(idpasswd));
```

- 지정한 소켓으로 부터 데이터를 읽어옴
- `client_socket` : 데이터를 읽을 대상(클라이언트와 연결된 소켓 디스크립터)
- `idpasswd` : 데이터를 저장할 버퍼(메모리 주소)
- `sizeof(idpasswd)` : 읽을 수 있는 최대 바이트 수(버퍼 크기)
- 클라이언트로부터 데이터가 도착할 때까지 블로킹
- 데이터가 오면 최대 지정한 크기만큼 읽어 버퍼에 저장
- 실제 읽은 바이트 수 반환, 연결 종료 시 `0` 반환, 에러 시 `-1` 반환
- 클라이언트가 전송한 ID/PW 등 인증 데이터 수신

<br>

```c
idpasswd[str_len] = '\0';
```
- `read` 함수는 문자열이 아닌 바이트 배열만 복사한다.
- C 문자열 함수들(`strtok`, `strcmp`, `strcpy` 등)은 널 문자 `\0`로 끝나는 문자열이어야 정상 동작
- 클라이언트가 보낸 데이터가 "abcd:1234" 라면, `read`로 9바이트(`\n` 포함)만 복사되고 마지막에 `\0`이 없을 수 있음
- 따라서 실제 읽은 길이(`str_len`)다음 NULL 종료 문자`\0` 추가 &rarr; 문자열 함수 사용 가능

<br>

```c
pToken = strtok(idpasswd, "[:]");
while(pToken != NULL)
{
    pArray[i] = pToken;
    if(i++ >= ARRAY_COUNT) break;
    pToken = strtok(NULL, "[:]");
}
```

- 입력 문자열을 구분자(`:`, `[`, `]`)기준으로 토큰 단위로 분리해서 `pArray`에 차례대로 저장
- 작동 흐름
    1. `strtok(idpasswd, "[:]");` : `idpasswd` 문자열에서 처음 구분자 전까지의 첫 토큰을 얻음
    2. `while`문 : 토큰이 `NULL`이 아닐때 반복, `pArray[i]`에 토큰 포인터 저장, `i`가 `ARRAY_COUNT`보다 크면(지정된 토큰 수 보다 크면) 종료, 다음 토큰은 `strtok(NULL, "[:]")`로 얻음(이전 위치에서 계속 탐색)
    3. 루프 종료 후 : `pArray[0]`, `pArray[1]`, ... 에 분리된 부분 문자열이 차례대로 저장됨

<br> 

```c
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
            pthread_mutex_lock(&mutex); // 뮤텍스 락을 건 상태에서
            client_info[i].index = i; // 인덱스
            client_info[i].fd = client_socket; // fd
            client_count++; // 클라이언트 수 증가
            pthread_mutex_unlock(&mutex); // 뮤텍스 언락
            sprintf(msg, "[%s] New connected! (ip:%s,fd:%d,socket:%d)\n", pArray[0], inet_ntoa(client_address.sin_addr), client_socket, client_count);
            log_file(msg);
            write(client_socket, msg, strlen(msg));

            pthread_create(thread_id + 1, NULL, client_connection, (void *)(client_info + 1));
            pthread_detach(thread_id[i]);
            break;
        }
    }
}
```

- `for(i=0; i < MAX_CLIENT; i++)` : 등록된 모든 클라이언트 정보를 순회
- `if(!strcmp(client_info[i].pw, pArray[1]))` : 입력받은 ID(`pArray[0]`)와 `client_info[i]`의 ID가 같으면
    - `if(!strcmp(client_info[i].pw, pArray[1]))` : 이미 해당 ID로 접속 중이라면(fd 가 -1이 아니면, 즉 이미 연결된 상태), 이미 로그인 되었다는 메세지 출력, 로그 기록, 소켓의 쓰기 채널 종료
    - `SHUT_WR` : 쓰기 채널(write half)을 종료, 즉 이후 이 소켓에 `write/sned`를 하면 에러 발생, 하지만 읽기`read`는 여전히 가능
    - `if(!strcmp(client_info[i].pw, pArray[1]))` : 패스워드 까지 일치하면 인증 성공
        - 뮤텍스 락을 건 상태에서 인덱스, fd, 클라이언트 수 증가 이후 뮤텍스 언락
        - `inet_ntoa(client_address.sin_addr)` : 32bit 이진수(IPv4주소)를 사람이 읽을 수 있는 점(dot) 표기 문자열로 변환하는 함수
            - `client_address.sin_addr` : `struct sockaddr_in` 구조체의 `sin_addr` 멤버
            - 4byte(32bit) 이진 IPv4 주소(네트워크 바이트 오더)
            - `sin_addr`에 저장된 값이 0xC0A80164(192.168.1.100)이면, `inet_ntoa`는 "192.168.1.100" 형태의 문자열을 반환
        - New connected! 메세지 로그 및 전송
        - 새로운 스레드 생성(`pthread_create`) : i번째 인덱스의 클라이언트 정보 포인터를 인자로 넘김
        - 생성한 스레드는 바로 분리(`pthread_detach`)
        - `break`로 루프 탈출


<br>

## 함수 구현

### `client_connection`

- 클라이언트 별 스레드 함수
- 각 클라이언트가 서버에 접속할 때 마다 생성되는 스레드의 메인 루틴
- 해당 클라이언트와 메세지 수신/처리/송신 전담

```c
first_client_info = (CLIENT_INFO *)((void *)client_info - (void *)(sizeof(CLIENT_INFO) * index));
```
- `client_info`배열의 첫 번째 원소 주소를 역산하는 코드
- `client_info`는 `CLIENT_INFO` 구조체 배열의 index번째 요소 주소임, 해당 포인터에서 `index * sizeof(CLIENT_INFO)` 만큼 빼면 배열의 첫 번째 원소 주소가 됨
- 스레드 함수 내 배열 전체의 시작 주소를 구할 때 사용
- 특정 요소 포인터만 받았지만, 전체 배열 시작 주소가 필요할 때

```c
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
```

```c
str_len = read(client_info->fd, msg, sizeof(msg) - 1);
if (str_len <= 0)
    break;
```
- 클라이언트 소켓에서 메세지 수진
- 반환값이 0 이하(에러)면 break

```c
msg[str_len] = '\0';
```

- 수신된 메세지 문자열을 `\0`로 끝나도록 

```c
pToken = strtok(msg, "[:]");
i = 0;
while (pToken != NULL)
{
    pArray[i] = pToken;
    if (i++ >= ARR_CNT)
        break;
    pToken = strtok(NULL, "[:]");
}
```
- `strtok`으로 구분자(`[`, `:`, `]`) 기준으로 메세지 파싱

```c
msg_info.fd = client_info->fd;
```
- 현재 클라이언트의 소켓 파일 디스크립터를 저장

```c
msg_info.from = client_info->id;
```
- 메세지의 발신자 ID 저장

```c
msg_info.to = pArray[0];
```
- 메세지 수신자 ID 저장

```c
sprintf(to_msg, "[%s]%s", msg_info.from, pArray[1]);
```
- 실제 전송할 메세지 내용을 만들기 위해 `[보낸사람]메세지내용` 형식의 문자열`to_msg` 생성

```c
msg_info.msg = to_msg;
```
- 만들어진 메세지 문자열의 포인터 저장

```c
msg_info.len = strlen(to_msg);
```
- 실제 메세지 길이 저장(전송 시 사용)

```c
sprintf(strBuff, "msg : [%s->%s] %s", msg_info.from, msg_info.to, pArray[1]);
```
- 로그 출력용 문자열 생성 `[보낸사람->받는사람] 메세지내용`

```c
log_file(strBuff);
```
- `log_file()` 사용자 정의 함수

```c
send_msg(&msg_info, first_client_info);
```
- `send_msg()` 사용자 정의 함수 

<br>

```c
close(client_info->fd);

sprintf(strBuff, "Disconnect ID:%s (ip:%s,fd:%d,sockcnt:%d)\n", client_info->id, client_info->ip, client_info->fd, client_count - 1);
log_file(strBuff);

pthread_mutex_lock(&mutex);
client_count--;
client_info->fd = -1;
pthread_mutex_unlock(&mutex);
```

```c
close(client_info->fd);
```
- 현재 클라이언트와 소켓 연결 종료(열었으면 닫자)
- 이후 해당 클라이언트와 데이터 송수신 불가

```c
sprintf(strBuff, "Disconnect ID:%s (ip:%s,fd:%d,sockcnt:%d)\n", client_info->id, client_info->ip, client_info->fd, client_count - 1);
log_file(strBuff);
```
- 연결 종료 로그 메세지 생성, 출력

```c
pthread_mutex_lock(&mutex);
client_count--;
client_info->fd = -1;
pthread_mutex_unlock(&mutex);
```
- 접속자 수 감소 및 상태 초기화
    - 여러 스레드가 동시에 접근할 수 있으므로 뮤텍스 락
    - 전체 클라이언트 수(`client_count`) 1 감소
    - 현재 클라이언트의 fd를 -1로 초기화(접속 해제 표시)
    - 뮤텍스 언락

<br>

### `send_msg`
```c
void send_msg(MSG_INFO *msg_info, CLIENT_INFO *first_client_info)
{
    int i = 0;

    if (!strcmp(msg_info->to, "ALLMSG"))
    {
        for (i = 0; i < MAX_CLIENT; i++)
        {
            if ((first_client_info + 1)->fd != -1)
                write((first_client_info + i)->fd, msg_info->msg, msg_info->len);
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
        write(msg_info->fd, idlist, strlen(idlist));
        free(idlist);
    }
    else if (!strcmp(msg_info->to, "GETTIME"))
    {
        sleep(1);
        get_localtime(msg_info->msg);
        write(msg_info->fd, msg_info->msg, strlen(msg_info->msg));
    }
    else
    {
        for (i = 0; i < MAX_CLIENT; i++)
        {
            if (((first_client_info + i)->fd != -1) && (!strcmp(msg_info->to, (first_client_info + i)->id)))
            {
                write((first_client_info + i)->fd, msg_info->msg, msg_info->len);
            }
        }
    }
}
```

- 클라이언트로부터 받은 메세지`msg_info`를 전송하는 역할 수행
- 매개변수
    - `MSG_INFO *msg_info` : 전송할 메세지 정보(보낸 사람, 받는 사람, 메세지 내용, 길이 등)
    - `CLIENT_INFO *first_client_info` : 클라이언트 정보 배열의 첫 번재 원소 포인터, 서버에 접속한 모든 클라이언트 상태 (소켓 fd, id 등) 관리

```c
if (!strcmp(msg_info->to, "ALLMSG"))
{
    for (i = 0; i < MAX_CLIENT; i++)
    {
        if ((first_client_info + i)->fd != -1)
            write((first_client_info + i)->fd, msg_info->msg, msg_info->len);
    }
}
```

- 수신자가 `ALLMSG`일 경우 서버에 접속한 모든 클라이언트에게 메세지 전송
- for문으로 등록된 모든 클라이언트 인덱스 순회
    - `first_client_info`는 클라이언트 정보 배열의 첫 번째 요소 포인터
    - `(first_client_info + i)`는 `i`번째 클라이언트 구조체 주소
    - `fd != -1`조건은 해당 클라이언트가 연결중임을 의미, `-1`이면 연결 끊어진 상태
    - 연결중인 클라이언트에 `write()`함수로 `msg_info->msg` 메세지를 길이만큼 전송

<br>

```c
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
    write(msg_info->fd, idlist, strlen(idlist));
    free(idlist);
}
```

- 메세지 수신자가 `IDLIST`일 때 실행

```C
char *idlist = (char *)malloc(ID_SIZE * MAX_CLIENT);
```
- ID 목록을 저장할 동적 버퍼 할당
- 크기는 최대 클라이언트 수 * ID 최대 길이

```c
msg_info->msg[strlen(msg_info->msg) - 1] = '\0';
strcpy(idlist, msg_info->msg);
```
- 메세지 끝(`\n`)을 널 문자로 변경해 끝을 자르고 `idlist`에 기존 메세지 복사

```c
for (i = 0; i < MAX_CLIENT; i++)
{
    if ((first_client_info + i)->fd != -1)
    {
        strcat(idlist, (first_client_info + i)->id);
        strcat(idlist, " ");
    }
}
```

- 접속 중인 클라이언트마다 ID를 `idlist` 문자열 뒤에 덧붙임, 각ID 뒤에 공백 추가

```c
strcat(idlist, "\n");
write(msg_info->fd, idlist, strlen(idlist));
free(idlist);
```

- 개행 문자 추가
- 요청한 클라이언트 소켓에 ID 리스트 문자열 전송
- 현재접속된 사용자 ID 리스트를 만들어 전달함

```c
else if (!strcmp(msg_info->to, "GETTIME"))
{
    sleep(1);
    get_localtime(msg_info->msg);
    write(msg_info->fd, msg_info->msg, strlen(msg_info->msg));
}
```

- 메세지 수신자가 `GETTIME` 일때 실행

```c
get_localtime(msg_info->msg);
```
- `get_localtime` 함수로 현재 시간을 문자열 형태로 `msg_info -> msg` 버퍼에 저장
- `get_localtime`함수는 시스템 시간 정보를 `[GETTIME]YY.MM.DD HH:MM:SS Wday` 형식 문자열로 반환


```c
write(msg_info->fd, msg_info->msg, strlen(msg_info->msg));
```

- 클라이언트 소켓(`msg_info->fd`)에 생성된 시간 문자열을 전송


```c
else
{
    for (i = 0; i < MAX_CLIENT; i++)
    {
        if (((first_client_info + i)->fd != -1) && (!strcmp(msg_info->to, (first_client_info + i)->id)))
        {
            write((first_client_info + i)->fd, msg_info->msg, msg_info->len);
        }
    }
}
```
- 특정 클라이언트 ID가 메세지 수신자일 때 그 대상 클라이언트에게 메세지를 전송
- 모든 클라이언트(`MAX_CLIENT`수만큼) 순회
- 각 클라이언트가 현재 접속 중인지 확인(`fd != -1`)
- 메세지 수신자(`msg_info->to`)와 클라이언트의 ID가 일치하는지 확인, 일치 시 해당 클라이언트 소켓으로 메세지 (`msg_info->msg`)전송

<br>

### `load_file`

```c
void load_file(const char *filename, CLIENT_INFO *client_info, int max_clients)
{
    FILE *fp = fopen(filename, "r");
    if(fp == NULL)
    {
        perror("파일 열기 실패");
        exit(1);
    }

    for(int i=0; i< max_clients; i++)
    {
        client_info[i].index = 0;
        client_info[i].fd = -1;
        strcpy(client_info[i].ip, "");
        strcpy(client_info[i].id, "");
        strcpy(client_info[i].pw, "");
    }

    int index = 0;
    while(1)
    {
        int return_fscanf = fscanf(fp, "%s %s", client_info[index].id, client_info[index].pw);

        if(return_fscanf == EOF || index >= MAX_CLIENT) break;

        index++;
    }
}
```
- 지정된 파일(`filename`)에서 사용자 ID와 비밀번호를 읽어와 `client_info` 배열에 저장하는 함수

```c
FILE *fp = fopen(filename, "r");
if(fp == NULL)
{
    perror("파일 열기 실패");
    exit(1);
}
```

- 파일을 읽기 모드로 열고 실패 시 에러 출력 후 프로그램 종료

```c
for(int i=0; i< max_clients; i++)
{
    client_info[i].index = 0;
    client_info[i].fd = -1;
    strcpy(client_info[i].ip, "");
    strcpy(client_info[i].id, "");
    strcpy(client_info[i].pw, "");
}
```

- 클라이언트 정보 배열 전체 초기화, 인덱스0, fd -1, 문자열 멤버들은 빈 문자열로 초기화

```c
int index = 0;
while(1)
{
    int return_fscanf = fscanf(fp, "%s %s", client_info[index].id, client_info[index].pw);

    if(return_fscanf == EOF || index >= MAX_CLIENT) break;

    index++;
}
```

- 파일 끝까지 또는 최대 클라이언트 수 까지 각 줄에서 ID와 PW를 읽어 배열에 저장
- `%s %s` 형식으로 공백 구분 문자열 2개 읽음

> txt 파일이 아닌 DB에서 꺼내오는걸로 바꿔보자~

<br>

### `get_localtime`

```c
void get_localtime(char *buf)
{
    struct tm *t;
    time_t tt;
    char wday[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    tt = time(NULL);
    if (errno == EFAULT) perror("time()");
    t = localtime(&tt);
    sprintf(buf, "[GETTIME]%02d.%02d.%02d %02d:%02d:%02d %s", t->tm_year + 1900 - 2000, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, wday[t->tm_wday]);
    return;
}
```

- 시스템 현재 시각을 읽고, 포맷팅하여 문자열 버퍼에저장, 클라이언트에 시간 정보 전송

<br>

# mariaDB(mySQL)
`idpasswd.txt`에서 데이터를 꺼내는것에서 DB에서 꺼내오는걸로

[mysql 함수](https://blog.naver.com/dgsw102/221058030471)

## mariaDB 설치

```bash
sudo apt install mariadb-server mariadb-client
```

라이브러리 설치
```bash
sudo apt-get install libmysqlclient-dev
```

컴파일 시 include 경로 지정
```bash
gcc -o myprog myprog.c -I/usr/include/mysql -lmysqlclient
```

mariadb 비밀번호 설정
```sql
SET PASSWORD FOR 'root'@'localhost' = PASSWORD('새비밀번호');
FLUSH PRIVILEGES;
```

사용자 목록 확인
```sql
SELECT user, host FROM mysql.user;
```
사용자 없으면 서버 접속 불가

사용자 생성
```sql
CREATE USER 'username'@'host' IDENTIFIED BY 'password(난 mariadb로 설정해둠)';
GRANT ALL PRIVILEGES ON dbname.* TO 'username(난 ubuntu로 해둠)'@'host';
FLUSH PRIVILEGES;
```
```c
if (mysql_real_connect(conn, "127.0.0.1", "ubuntu", "mariadb", "account", 0, NULL, 0) == NULL)
{
    sql_error(conn);
}
```
## `load_idpasswd_db`

```c
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

    mysql_num_rows(result);
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
```
<br>

- `client_info[]`를 한 번 초기화하고(fd=-1, 문자열 비움), 이후 MySQL 서버에 접속한다.
- `SELECT id, password FROM idpasswd` 실행 후 결과 집합을 fetch 하면서 최대 `MAX_CLIENT` 개 행을 `client_info[]`에 채운다.
- 완료되면 결과·커넥션을 해제한다. 실패 시 `sql_error()`가 에러 메시지를 출력하고 프로세스를 종료한다.



```c
for (int i = 0; i < max_clients; i++)
{
    client_info[i].index = 0;
    client_info[i].fd = -1;
    strcpy(client_info[i].ip, "");
    strcpy(client_info[i].id, "");
    strcpy(client_info[i].pw, "");
}
```
>**EBADF(Bad file descriptor) 방지**
>
>`CLIENT_INFO.client_info` 초기화 로직 빠지면, `load_idpasswd_db()`에서 가져온 DB 데이터 외의 나머지 슬롯(fd 등)이 쓰레기 값(garbage value)으로 남아있음
>- `fd`가 -1이 아닌 쓰레기 값으로 남음
>- `send_msg()`에서 `(first_client_info + i)->fd != -1` 조건이 잘못 참이 됨
>- 실제 연결되지 않은 `fd`로 `write()` 호출 → EBADF (Bad file descriptor) 발생