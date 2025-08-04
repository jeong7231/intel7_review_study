```c
// 프로그램 실행 시 인자의 수가 6개이고, 세 번째 인자가 "SIGNUP"이면 회원가입 시도
// argc: 명령줄에서 받은 인자(argument)의 개수
// argv[]: 명령줄에서 받은 인자 목록 (argv[0]은 실행 파일 이름)
if (argc == 6 && strcmp(argv[3], "SIGNUP") == 0)
{
    // sprintf: 문자열을 형식대로 만들어서 msg에 저장
    // 예: argv[4]가 "abc", argv[5]가 "1234"라면
    // msg = "[SIGNUP:abc:1234]"
    sprintf(msg, "[SIGNUP:%s:%s]", argv[4], argv[5]);

    // 서버에 회원가입 요청을 보냄
    // write 함수: sock(서버와 연결된 소켓)에 msg 문자열을 전송
    write(sock, msg, strlen(msg));

    // 서버로부터 응답을 읽음
    // read 함수: sock 소켓으로부터 최대 BUF_SIZE - 1 바이트만큼 데이터를 읽어 msg에 저장
    int recv_len = read(sock, msg, BUF_SIZE - 1);

    // 문자열의 끝을 명시적으로 '\0'으로 닫아서 안전한 C 문자열로 만듦
    msg[recv_len] = 0;

    // strstr: 받은 메시지 안에 "SIGNUP failed" 라는 문장이 있는지 확인
    if (strstr(msg, "SIGNUP failed") != NULL)
    {
        // 실패 메시지가 있으면 출력하고 종료
        fputs(msg, stdout); // 표준 출력(화면)에 메시지 출력
        close(sock);        // 소켓 연결 종료
        exit(1);            // 프로그램 비정상 종료
    }

    // 회원가입이 성공했다면, 바로 로그인 시도
    // strcpy: name 변수에 입력받은 ID를 복사
    strcpy(name, argv[4]);

    // msg에 "[아이디:비밀번호]" 형식의 문자열 생성
    sprintf(msg, "[%s:%s]", name, argv[5]);

    // 로그인 정보 전송
    write(sock, msg, strlen(msg));
}
else if (argc == 4)
{
    // 인자가 4개면 회원가입이 아닌 단순 로그인 시도라고 가정
    // 예: ./client 127.0.0.1 8080 user1

    // argv[3]은 ID, 이를 name에 복사
    strcpy(name, argv[3]);

    // "[user1:PASSWD]" 형식의 메시지 생성
    // 서버는 이 메시지를 보고 "비밀번호를 보내라"는 의미로 이해함
    sprintf(msg, "[%s:PASSWD]", name);

    // 로그인 요청 메시지 전송
    ssize_t ret = write(sock, msg, strlen(msg));

    // write 실패하면 에러 메시지 출력
    if (ret == -1)
        perror("write error");
}
```