```c
// 클라이언트가 보낸 문자열이 "[SIGNUP:"으로 시작하는지 확인하는 조건문이다.
// strncmp 함수는 두 문자열을 앞에서부터 일정 글자 수만큼 비교하는 함수이다.
// 여기서는 앞 8글자, 즉 "[SIGNUP:" 와 같은지 비교한다.
if (strncmp(idpasswd, "[SIGNUP:", 8) == 0)
{
    // 만약 "[SIGNUP:" 으로 시작하는 문자열이 맞다면, 회원가입 요청으로 처리한다.

    // 회원가입 요청에서 아이디와 비밀번호를 추출해 저장할 변수 선언
    // ID_SIZE는 미리 정의된 숫자 (예: 10), 즉 최대 10글자까지만 받겠다는 의미
    // {0}은 배열을 0으로 초기화하는 문법
    char signup_id[ID_SIZE] = {0}; // 아이디를 저장할 공간
    char signup_pw[ID_SIZE] = {0}; // 비밀번호를 저장할 공간

    // 문자열 파싱: 클라이언트가 보낸 "[SIGNUP:아이디:비밀번호]"에서
    // 아이디와 비밀번호만 뽑아낸다
    // sscanf 함수는 문자열에서 일정한 형식(포맷)을 기반으로 값을 분리하는 함수이다.
    // 예시: "[SIGNUP:abc:1234]" 에서 "abc" → signup_id, "1234" → signup_pw 에 저장
    // 포맷 "%9[^:]" → ':' 문자가 나오기 전까지 최대 9글자 저장
    sscanf(idpasswd, "[SIGNUP:%9[^:]:%9[^]]]", signup_id, signup_pw);

    // 이제 데이터베이스에 연결해서 아이디 중복 여부를 검사한다.

    // MySQL 데이터베이스와 연결하기 위한 구조체 포인터를 생성
    // mysql_init(NULL)는 내부적으로 메모리를 할당하고 초기화
    MYSQL *conn = mysql_init(NULL);
    if (conn == NULL)
        error_handling("mysql_init() failed"); // 연결 준비 실패시 에러 출력

    // 실제로 MySQL 서버와 연결 시도
    // 인자: IP주소, 사용자명, 비밀번호, DB명, 포트번호 등
    // 성공하면 conn 포인터가 연결된 상태가 됨
    if (mysql_real_connect(conn, "127.0.0.1", "ubuntu", "mariadb", "account", 0, NULL, 0) == NULL)
        sql_error(conn); // 연결 실패시 에러 처리

    // 이미 존재하는 아이디인지 확인하기 위해 SELECT 쿼리 작성
    // query는 SQL문을 담을 문자열
    char query[256];
    snprintf(query, sizeof(query), "SELECT id FROM idpasswd WHERE id='%s'", signup_id);
    // snprintf: 문자열을 안전하게 생성 (버퍼 오버플로 방지)

    // SQL문 실행 요청
    if (mysql_query(conn, query))
        sql_error(conn); // SQL 실행 중 오류 시 처리

    // 결과를 받아옴: SELECT 결과를 메모리로 가져옴
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL)
        sql_error(conn); // 결과 가져오기 실패 시 처리

    // 결과 행 수가 0보다 크면 이미 존재하는 아이디
    if (mysql_num_rows(result) > 0)
    {
        // 실패 메시지를 클라이언트에게 전송
        char fail_msg[] = "SIGNUP failed (ID already exists)\n";
        write(client_socket, fail_msg, strlen(fail_msg)); // 소켓을 통해 메시지 전송
        shutdown(client_socket, SHUT_WR); // 더 이상 쓰지 않겠다고 소켓 종료 (쓰기 방향만)

        mysql_free_result(result); // 쿼리 결과 메모리 해제
        mysql_close(conn);         // 데이터베이스 연결 종료
        close(client_socket);      // 클라이언트와의 소켓 연결 종료
        continue; // 다음 클라이언트 처리로 이동
    }

    // 중복이 아니면, 결과 메모리만 해제
    mysql_free_result(result);

    // 새로운 아이디와 비밀번호를 데이터베이스에 INSERT
    snprintf(query, sizeof(query), "INSERT INTO idpasswd(id, password) VALUES('%s','%s')", signup_id, signup_pw);
    if (mysql_query(conn, query))
        sql_error(conn); // 삽입 실패시 에러

    // DB 연결 닫기
    mysql_close(conn);

    // 이제 서버 내부 구조에 클라이언트 정보를 등록해야 한다.

    pthread_mutex_lock(&mutex); // 동시에 접근하면 안되므로 뮤텍스로 잠금

    int idx = -1; // 클라이언트 배열에서 빈 자리를 찾기 위한 변수

    for (i = 0; i < MAX_CLIENT; i++)
    {
        if (client_info[i].fd == -1) // 이 위치가 비어 있다면 (연결된 클라이언트 없음)
        {
            idx = i;
            break;
        }
    }

    if (idx == -1)
    {
        // 빈 슬롯이 없을 경우 클라이언트 연결 거부
        char fail_msg[] = "No slot for new client\n";
        write(client_socket, fail_msg, strlen(fail_msg));
        shutdown(client_socket, SHUT_WR);
        close(client_socket);
        pthread_mutex_unlock(&mutex);
        continue;
    }

    // 빈 슬롯을 찾았으면 클라이언트 정보 등록
    client_info[idx].fd = client_socket;
    client_info[idx].index = idx;

    // 접속한 클라이언트의 IP 주소 저장
    strncpy(client_info[idx].ip, inet_ntoa(client_address.sin_addr), sizeof(client_info[idx].ip) - 1);
    client_info[idx].ip[sizeof(client_info[idx].ip) - 1] = '\0'; // 문자열 끝에 NULL 보장

    // 아이디와 비밀번호 복사 (로그인 확인 등에 사용)
    strncpy(client_info[idx].id, signup_id, ID_SIZE - 1);
    client_info[idx].id[ID_SIZE - 1] = '\0';

    strncpy(client_info[idx].pw, signup_pw, ID_SIZE - 1);
    client_info[idx].pw[ID_SIZE - 1] = '\0';

    client_count++; // 전체 접속자 수 증가
    pthread_mutex_unlock(&mutex); // 잠금 해제

    // 클라이언트에게 성공 메시지 전송
    snprintf(msg, sizeof(msg), "SIGNUP success. [%s] login OK\n", signup_id);
    write(client_socket, msg, strlen(msg));
    log_file(msg); // 서버 로그 파일에 성공 기록

    // 클라이언트 전용 스레드 생성하여 연결 유지
    pthread_create(thread_id + idx, NULL, client_connection, (void *)(client_info + idx));
    pthread_detach(thread_id[idx]); // 생성된 스레드를 분리 (자동 종료)
    continue; // 다음 클라이언트 요청 처리
}
```