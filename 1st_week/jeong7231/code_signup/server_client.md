# TCP/IP 기반 C 채팅 프로그램 상세 분석 문서

## 1. 개요

본 문서는 `client_jty.c`와 `server_jty.c` 소스 코드를 기반으로 TCP/IP 기반 채팅 시스템의 각 기능별 코드 흐름과 MariaDB 연동 처리 과정을 상세히 분석한다.

---

## 2. TCP/IP 개요

* **TCP/IP**는 네트워크 통신을 위한 기본 프로토콜 스택이며, 본 프로그램은 **TCP (SOCK\_STREAM)** 기반의 연결형 통신을 사용한다.
* 사용된 주요 함수:

  * `socket()`, `bind()`, `listen()`, `accept()` → 서버 초기화
  * `connect()` → 클라이언트 연결
  * `read()`, `write()` → 데이터 송수신

---

## 3. 클라이언트 코드 (`client_jty.c`)

### 3.1 실행 흐름 요약

1. 인자 검증 및 모드 판단 (로그인 / 회원가입)
2. TCP 소켓 생성 및 서버 연결
3. 로그인 또는 회원가입 처리
4. 송수신 스레드 생성 및 메시지 처리

### 3.2 로그인/회원가입 처리 흐름

```c
if (argc == 6 && strcmp(argv[3], "SIGNUP") == 0)
{
    sprintf(msg, "[SIGNUP:%s:%s]", argv[4], argv[5]);
    write(sock, msg, strlen(msg));
    read(sock, msg, BUF_SIZE - 1);
    if (strstr(msg, "failed")) 종료
    else 로그인 메시지 전송
}
else if (argc == 4)
{
    sprintf(msg, "[%s:PASSWD]", name);
    write(sock, msg, strlen(msg));
}
```

### 3.3 메시지 송신 스레드 (`send_msg`)

* `select()`로 사용자 입력 비동기 처리
* 입력 메시지가 `[ID]` 형식이 아니면 `[ALLMSG]`로 포맷
* `write()`를 통해 서버로 전송

### 3.4 메시지 수신 스레드 (`receive_msg`)

* 서버로부터 메시지를 수신하여 출력
* `read()` → `fputs()`로 터미널 출력

---

## 4. 서버 코드 (`server_jty.c`)

### 4.1 서버 초기화

```c
socket() → setsockopt() → bind() → listen()
```

* `setsockopt(SO_REUSEADDR)` 사용으로 포트 재사용 가능 설정

### 4.2 클라이언트 연결 처리 흐름

```c
client_socket = accept();
read(client_socket, idpasswd, ...);
```

* 첫 메시지 분석:

  * `[SIGNUP:id:pw]`: 회원가입 처리
  * `[id:PASSWD]`: 로그인 처리

### 4.3 회원가입 처리

```c
sscanf(idpasswd, "[SIGNUP:%9[^:]:%9[^]]]", signup_id, signup_pw);
mysql_real_connect() → SELECT로 중복 확인
→ INSERT INTO idpasswd
→ client_info[] 등록
→ pthread_create(client_connection)
```

### 4.4 로그인 처리

```c
strtok(idpasswd, "[:]") → pArray[0]: ID, pArray[1]: PW
for (i=0; i<MAX_CLIENT; i++)
{
    if (id/pw 일치)
        client_info[i].fd = client_socket;
        pthread_create(client_connection)
}
```

---

## 5. 클라이언트 핸들링 스레드 (`client_connection`)

### 흐름

1. `read()`로 메시지 수신
2. `strtok()`으로 `[to]msg` 파싱
3. `MSG_INFO` 구조체 생성
4. `send_msg()` 호출로 전송 처리

---

## 6. 메시지 전송 처리 (`send_msg`)

| 조건        | 동작               |
| --------- | ---------------- |
| `ALLMSG`  | 모든 접속자에게 전송      |
| `IDLIST`  | 현재 접속자 ID 리스트 반환 |
| `GETTIME` | 서버 시간 문자열 반환     |
| `ID`      | 지정된 사용자에게만 송신    |

```c
if (!strcmp(msg_info->to, "ALLMSG"))
    write() → 전체에게 전송
else if (!strcmp(..., "IDLIST"))
    현재 client_info[] 순회하며 ID 문자열 작성 후 송신
else if (!strcmp(..., "GETTIME"))
    get_localtime() 호출 후 반환
else
    지정된 ID를 가진 클라이언트에게만 write()
```

---

## 7. MariaDB 연동 처리

### 사용된 함수

| 함수                     | 설명          |
| ---------------------- | ----------- |
| `mysql_init()`         | 연결 핸들 초기화   |
| `mysql_real_connect()` | DB 서버 접속    |
| `mysql_query()`        | SQL 명령 실행   |
| `mysql_store_result()` | 결과 셋 확보     |
| `mysql_fetch_row()`    | 결과 1개 행씩 추출 |
| `mysql_close()`        | 연결 종료       |
| `mysql_error()`        | 에러 출력       |

### 사용자 정보 로딩 (`load_idpasswd_db`)

* `SELECT id, password FROM idpasswd` 실행
* 결과를 `client_info[]`에 초기화

### 회원가입 시 중복 검사 및 삽입

```sql
SELECT id FROM idpasswd WHERE id='%s';
INSERT INTO idpasswd(id, password) VALUES('%s','%s');
```

---

## 8. 메시지 포맷 정리

| 포맷               | 설명                    |
| ---------------- | --------------------- |
| `[ID:PASSWD]`    | 로그인 요청                |
| `[SIGNUP:ID:PW]` | 회원가입 요청               |
| `내용`             | → `[ALLMSG]내용` 으로 전송됨 |
| `[ID]내용`         | 특정 사용자에게 전송           |
| `[IDLIST]`       | 사용자 목록 요청             |
| `[GETTIME]`      | 서버 시간 요청              |

---

## 9. 멀티스레딩 및 동기화

* 클라이언트마다 `pthread_create()`로 별도 스레드
* `client_info[]`, `client_count` 공유자원 보호를 위해 `pthread_mutex_t mutex` 사용

---

## 10. 보안 및 에러 처리

* 잘못된 로그인/중복 접속은 `[ID] Already logged!` 등으로 차단
* DB 연결 실패 및 쿼리 실패 시 `sql_error()` 호출로 로그 출력 및 종료
* `fd = -1`로 해제되지 않은 소켓 접근 방지 처리 포함

---

## 11. 요약

| 항목     | 내용                               |
| ------ | -------------------------------- |
| 통신 방식  | TCP/IP (SOCK\_STREAM)            |
| 메시지 형식 | `[TO]Message` 형태, `[ALLMSG]` 지원  |
| 사용자 인증 | MariaDB 연동                       |
| 데이터 구조 | `MSG_INFO`, `CLIENT_INFO` 구조체 활용 |
| 스레드 구조 | 클라이언트별 전용 스레드 생성                 |
| 기능 확장  | 파일 전송, 채팅방, 명령어 확장 가능함           |
