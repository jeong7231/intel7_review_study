# 1st_week
2025.07.29 ~ 2025.08.05 

# 목표

- 다중 사용자가 메시지와 파일을 전송할 수 있는 채팅 프로그램 구현
- 리눅스 환경을 사용해 TCP/IP 소켓 통신으로 Server, Client 프로그램을 구현

# 조건

- 기존의 Server, Client 구조와 데이터의 형식은 유지 ( [Recv_ID]command@data1@data2 )
- 기존의 ID_Password를 저장하는 txt 파일을 제거하고 MariaDB를 사용
- 공통으로 있어야 하는 기능
    1. 메시지 전송
    2. 파일 전송 (미구현)
    3. ID, Password 생성, 변경, 삭제

---

## 프로그램 구조 및 파일

### 1. 서버 프로그램(server.c)

#### 주요 기능
- 여러 클라이언트 동시 접속 및 통신 지원 (멀티스레드)
- MariaDB에서 계정 로딩, 생성, 삭제
- 메시지 라우팅 (전체/개별 대상)
- 메시지 형식: `[Recv_ID]command@data1@data2`
- **파일 전송 기능은 현재 미구현**

#### 주요 구조/함수
- `CLIENT_INFO`, `MSG_INFO` 구조체: 클라이언트 정보/메시지 정보 관리
- `main()`
    - 서버 소켓 생성 및 바인드, 대기
    - 클라이언트 접속 처리, 인증, 스레드 생성
- `clnt_connection()`
    - 각 클라이언트와 독립적 통신 담당 (pthread)
    - 메시지 파싱 및 커맨드별 처리
        - `SIGNUP@ID@PW` : 계정 생성(DB)
        - `DELACC@ID` : 계정 삭제(DB)
        - 그 외 일반 메시지: 목적지로 라우팅
- `send_msg()`
    - 대상이 ALLMSG면 전체 전송, 특정 ID면 해당 클라이언트만 전송
- DB 연동 함수:
    - `load_accounts_from_db()` : DB에서 ID/PW 정보 로딩
    - `create_account_in_db()` : 신규 계정 추가
    - `delete_account_in_db()` : 특정 ID 계정 삭제

#### MariaDB account 테이블 구조 예시
| id    | pw       |
|-------|----------|
| user1 | pass1    |

---

### 2. 클라이언트 프로그램(client.c)

#### 주요 기능
- 서버 접속 및 인증
- 사용자 입력 메시지 송신 (`[ID]메시지` 또는 그냥 메시지 입력시 [ALLMSG] 자동 지정)
- 서버로부터의 메시지 수신 및 출력
- 파일 전송 커맨드는 구현되지 않음(추후 개발 필요)

#### 주요 구조/함수
- `main()`
    - 서버와 TCP 연결, 인증 패킷 전송
    - 메시지 송수신 스레드 생성
- `send_msg()`
    - 사용자의 명령어 또는 메시지 입력 처리
    - `quit` 입력시 연결 종료
    - 메시지 형식 자동 변환 (`[ALLMSG]` 자동 지정)
- `recv_msg()`
    - 서버로부터 전달받은 메시지 출력
    - 패킷 형식과 상관없이 일괄 출력

---

## 미구현: 파일 전송

- [ALLMSG]FILE@파일명 전송 → 이후 실 데이터 전송
- 서버는 파일 전송 패킷을 중계만 수행, 실제 파일 입출력은 클라이언트가 담당

---

## 코드별 상세 설명

### server.c 주요 흐름
```plaintext
main()
 ├─ load_accounts_from_db()         // 계정 정보 불러오기
 ├─ 소켓 생성/바인드/리스닝
 └─ while(1)
      ├─ accept()                   // 클라이언트 접속
      ├─ 인증/로그인 체크
      └─ pthread_create()           // clnt_connection 스레드 생성

clnt_connection()
 ├─ 클라이언트와 메시지 송수신 반복
 ├─ 명령어별 분기 (SIGNUP, DELACC 등)
 └─ send_msg()로 메시지 중계