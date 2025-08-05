# C 서버-클라이언트 코드 헤더파일 

## 1. 표준 C 헤더

| 헤더 파일        | 사용 예시/함수                                | 역할 및 사용 목적                |
| ------------ | --------------------------------------- | ------------------------- |
| `<stdio.h>`  | `printf`, `sprintf`, `fputs`, 등         | 표준 입출력 함수, 파일 입출력         |
| `<stdlib.h>` | `exit`, `malloc`, `free`                | 동적 메모리, 프로그램 종료 등         |
| `<string.h>` | `strcpy`, `strncpy`, `strcmp`, `strtok` | 문자열 처리, 토큰 분할 등           |
| `<errno.h>`  | `errno`                                 | 에러 코드 관리                  |
| `<time.h>`   | `time`, `localtime`, `strftime`         | 시스템 시간 및 날짜 관리            |
| `<unistd.h>` | `close`, `read`, `write`, `sleep`       | 유닉스/리눅스 시스템 콜 (파일, 입출력 등) |

## 2. 시스템/OS 헤더

| 헤더 파일            | 사용 예시/함수                                                       | 역할 및 사용 목적            |
| ---------------- | -------------------------------------------------------------- | --------------------- |
| `<arpa/inet.h>`  | `inet_ntoa`, `inet_addr`, `htons`                              | 네트워크 주소 변환, 바이트 오더 처리 |
| `<netinet/in.h>` | `sockaddr_in` 구조체                                              | 인터넷 주소 체계, IP/포트 구조   |
| `<sys/socket.h>` | `socket`, `bind`, `listen`, `accept`, `setsockopt`, `shutdown` | 소켓 프로그래밍              |
| `<sys/types.h>`  | 데이터 타입 정의 (`socklen_t` 등)                                      | 시스템 정의 타입             |
| `<pthread.h>`    | `pthread_create`, `pthread_mutex_*`                            | POSIX 스레드 및 뮤텍스 관리    |
| `<sys/stat.h>`   | 파일 상태 정보                                                       | 파일 정보 확인 등            |
| `<fcntl.h>`      | 파일 디스크립터 제어                                                    | 파일 열기/잠금 등            |
| `<dirent.h>`     | 디렉토리 탐색                                                        | 디렉토리 접근 등             |
| `<sys/time.h>`   | `struct timeval`, 타이머 관련                                       | 시간 측정 및 select() 등    |

## 3. 데이터베이스 헤더

| 헤더 파일             | 사용 예시/함수                                                                                   | 역할 및 사용 목적                  |
| ----------------- | ------------------------------------------------------------------------------------------ | --------------------------- |
| `<mysql/mysql.h>` | `mysql_init`, `mysql_real_connect`, `mysql_query`, `mysql_store_result`, `mysql_fetch_row` | MariaDB/MySQL 연동, 질의, 결과 처리 |



## 4. 각 헤더별 함수 실제 사용 예시

### 4.1 표준 입출력/에러/문자열

* `sprintf(buf, "...", ...)`, `fputs`, `fputc`, `perror` : 메시지 생성/출력, 에러 출력
* `strcpy`, `strncpy`, `strcmp`, `strtok`, `strcat` : 문자열 복사, 비교, 파싱

### 4.2 시스템 콜 및 네트워크

* `socket()`, `bind()`, `listen()`, `accept()`, `connect()`
  소켓 생성, 바인딩, 리스닝, 클라이언트/서버 연결
* `setsockopt()`
  소켓 옵션 설정 (재사용 옵션 등)
* `read()`, `write()`, `close()`, `shutdown()`
  데이터 송수신, 소켓/파일 종료
* `inet_ntoa()`, `inet_addr()`, `htons()`
  IP 주소/포트 변환
* `struct sockaddr_in`
  서버/클라이언트 주소 구조체
* `pthread_create`, `pthread_detach`, `pthread_mutex_*`
  스레드 생성, 리소스 관리, 뮤텍스 동기화
* `select()`
  다중 입출력 감시

### 4.3 시간/로깅

* `time()`, `localtime()`
  현재 시간 획득 및 변환
* `strftime()`
  시간 포맷 변환

### 4.4 데이터베이스 연동

* `mysql_init()`, `mysql_real_connect()`
  MySQL/MariaDB 접속
* `mysql_query()`, `mysql_store_result()`, `mysql_fetch_row()`
  쿼리 실행 및 결과 획득
* `mysql_free_result()`, `mysql_close()`
  결과 해제, 연결 종료



## 5. 주요 헤더별 요약표

| 헤더                | 주요 역할   | 코드 내 사용 예시                    |
| ----------------- | ------- | ----------------------------- |
| `<stdio.h>`       | 입출력     | `fputs`, `sprintf`            |
| `<stdlib.h>`      | 종료/메모리  | `exit`, `malloc`, `free`      |
| `<string.h>`      | 문자열/파싱  | `strcpy`, `strncpy`, `strtok` |
| `<errno.h>`       | 에러번호    | `errno`, `perror`             |
| `<time.h>`        | 시간      | `time`, `localtime`           |
| `<unistd.h>`      | 시스템콜    | `read`, `write`, `close`      |
| `<arpa/inet.h>`   | 네트워크 변환 | `inet_addr`, `inet_ntoa`      |
| `<netinet/in.h>`  | 네트워크 구조 | `sockaddr_in`                 |
| `<sys/socket.h>`  | 소켓      | `socket`, `bind`, `listen`    |
| `<sys/types.h>`   | 데이터타입   | `socklen_t`                   |
| `<pthread.h>`     | 스레드/동기화 | `pthread_create`, `mutex`     |
| `<mysql/mysql.h>` | 데이터베이스  | `mysql_*` 함수                  |



## 6. 참고: 컴파일 명령

* pthread와 mysql 연동 시 컴파일 예시

```sh
gcc server_jty.c -o server_jty -lpthread -lmysqlclient
gcc client_jty.c -o client_jty -lpthread
```



## 7. 각 헤더별 핵심 포인트

1. 네트워크 통신/소켓: `<arpa/inet.h>`, `<netinet/in.h>`, `<sys/socket.h>`, `<sys/types.h>`
2. 멀티스레드/동기화: `<pthread.h>`
3. 데이터베이스 연동: `<mysql/mysql.h>`
4. 표준 입출력/문자열/에러: `<stdio.h>`, `<stdlib.h>`, `<string.h>`, `<errno.h>`
5. 시간 처리: `<time.h>`
6. 유닉스/리눅스 기본 시스템 콜: `<unistd.h>`

