# 코드 설명

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