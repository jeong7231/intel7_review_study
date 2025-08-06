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
    2. 파일 전송
    3. ID, Password 생성, 변경, 삭제
