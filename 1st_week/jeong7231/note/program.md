# TCP/IP 기반 채팅 프로그램 

## 1. 프로그램 개요

* **언어**: C
* **통신 방식**: TCP/IP (SOCK\_STREAM)
* **기능**: 로그인, 회원가입, 전체 메시지, 개인 메시지, 명령어 처리(IDLIST, GETTIME)
* **DB 연동**: MariaDB 기반 사용자 인증
* **구조**: 서버-클라이언트, pthread 기반 멀티스레딩


## 2. 실행 방식

### 클라이언트

```bash
# 로그인
./client_jty <IP> <PORT> <ID>

# 회원가입 + 로그인
./client_jty <IP> <PORT> SIGNUP <ID> <PW>
```

### 서버

```bash
./server_jty <PORT>
```


## 3. 전체 흐름 요약

### 클라이언트 측

1. 실행 인자 해석 → 로그인 / 회원가입 판단
2. 소켓 연결 → 인증 메시지 전송
3. 로그인 성공 시 송신/수신 스레드 생성

### 서버 측

1. 시작 시 DB에서 사용자 정보 로드
2. 클라이언트 접속 처리

   * 회원가입 요청: 중복 검사 → INSERT
   * 로그인 요청: ID/PW 확인 → 접속자 등록
3. 클라이언트별 전용 스레드 생성
4. 메시지 파싱 후 전송 라우팅

## 4. 메시지 포맷

| 유형        | 예시                    | 설명                        |
| ----------- | ----------------------- | --------------------------- |
| 로그인      | `[ID:PASSWD]`           | 로그인 요청                 |
| 회원가입    | `[SIGNUP:ID:PW]`        | 회원가입 요청               |
| 전체 메시지 | `내용` → `[ALLMSG]내용` | 전체 사용자에게 전송        |
| 개인 메시지 | `[상대ID]내용`          | 특정 사용자에게만 전송      |
| 명령어      | `[IDLIST]`, `[GETTIME]` | 접속자 목록, 서버 시간 요청 |


## 5. 클라이언트 구성 (client\_jty.c)

### 주요 함수

* `send_msg()`

  * 사용자 입력 감지 (`select`)
  * 내용 전처리 후 `write()`

* `receive_msg()`

  * 서버로부터 메시지 수신 후 `stdout` 출력

### 특징

* 회원가입 성공 시 자동 로그인 처리
* 일반 메시지는 자동으로 `[ALLMSG]` 포맷 추가
* `strtok()`으로 ID 추출 가능


## 6. 서버 구성 (server\_jty.c)

### 초기화 흐름

```c
socket() → setsockopt() → bind() → listen()
```

* 포트 재사용 옵션 설정

### 클라이언트 처리 흐름

```c
accept() → read(첫 메시지)
```

* `[SIGNUP:ID:PW]`: 중복 검사 → INSERT → 로그인 상태
* `[ID:PASSWD]`: 정보 일치 시 접속 허용

### 스레드 동작

* `client_connection()`
  → 메시지 수신 → `MSG_INFO` 구성 → `send_msg()` 호출

* `send_msg()`
  → 메시지 종류에 따라 브로드캐스트, 단일 전송, IDLIST, GETTIME 응답


## 7. MariaDB 연동

### 사용 DB 정보

* DB명: `account`
* 테이블: `idpasswd(id VARCHAR, password VARCHAR)`

### 사용 함수

| 함수                   | 설명             |
| ---------------------- | ---------------- |
| `mysql_real_connect()` | DB 연결          |
| `mysql_query()`        | SQL 실행         |
| `mysql_store_result()` | SELECT 결과 확보 |
| `mysql_fetch_row()`    | 행 추출          |
| `mysql_close()`        | 연결 종료        |

### 처리 흐름

* 회원가입 시 `SELECT`로 중복 확인 후 `INSERT`
* 서버 기동 시 전체 사용자 정보를 메모리(`client_info[]`)에 로딩


## 8. 구조체 요약

```c
typedef struct {
    int fd;
    char ip[20];
    char id[ID_SIZE];
    char pw[ID_SIZE];
} CLIENT_INFO;

typedef struct {
    int fd;
    char *from;
    char *to;
    char *msg;
    char len;
} MSG_INFO;
```

* `CLIENT_INFO[]`로 전체 접속자 관리
* `MSG_INFO`를 통해 메시지 전송 대상 및 내용 추적


## 9. 멀티스레딩

* `pthread_create()`로 송수신 및 접속자별 처리 스레드 생성
* `pthread_detach()`로 리소스 자동 회수
* `pthread_mutex_t mutex`로 `client_info[]`, `client_count` 보호


## 10. 에러 처리

* 로그인 실패, 중복 로그인 → 거부 메시지 후 종료
* DB 오류 → `sql_error()` 호출 후 종료
* 잘못된 접근 시 `fd = -1`로 무효화하여 접근 방지



