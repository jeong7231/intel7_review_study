
#CC:=arm-linux-gcc
# 사용할 컴파일러 지정
CC := gcc

# TARGET_SRV : 서버 프로그램 이름 (server)
TARGET_SRV = server
# OBJECT_SRV : 서버 오브젝트 파일 (server.o)	
OBJECT_SRV = $(TARGET_SRV).o

# TARGET_CLN : 클라이언트 프로그램 이름 (client)
TARGET_CLN = client
# OBJECT_CLN : 클라이언트 오브텍트 파일 (client.o)
OBJECT_CLN = $(TARGET_CLN).o

#LDFLAGS=-D_REENTRANT -pthread -lmysqlclient
# -lmysqlclient : MySQL 라이브러리 연결
# -D_REENTRANT : 멀티스레드 프로그래밍에서 필요한 매크로
# -pthread : pthread 라이브러리(스레드 관련) 사용 선언
LDFLAGS = -D_REENTRANT -pthread -lmysqlclient

# make만 쳤을 때 server와 client 두 실행파일을 모두 만듬
all: $(TARGET_SRV) $(TARGET_CLN)
	rm -f *.o

# 서버, 클라이언트 실행파일을 만드는 규칙
# $@ : 타겟 이름 (실행파일 이름)
# 각 실행파일을 만들기 위해서는 대응하는 오브젝트 파일 필요
# ex)
# server : server.o -> gcc -o server
# server.o -D_REENTRANT -pthread
# --------------------------------------------------------
# 실제 동작
# 1. 소스파일(.c)이 오브젝트(.o)로 컴파일
# 2. 오브젝트(.o) 파일을 링크해서 실행파일이 만들어짐
$(TARGET_SRV): $(OBJECT_SRV)
	$(CC) -o $@ $(OBJECT_SRV) $(LDFLAGS)

$(TARGET_CLN): $(OBJECT_CLN)
	$(CC) -o $@ $(OBJECT_CLN) $(LDFLAGS)

# 모든 .c파일을 대응하는 .o 파일로 바꾸는 패턴 규칙
# $< : 첫 번째 의존성(여기선 소스파일, 예시 : server.c)
# $@ : 타겟(결과 파일, 예 : server.o)
# 실제로는 gcc -c -o server.o server.c로 해석
%.o: %.c
	$(CC) -c -o $@ $<

# make clean을 입력하면 생성된 오브젝트 파일, 서버 실행 파일, 클라이언트 실행파일 모두 삭제
clean:
	rm -f *.o $(TARGET_SRV) $(TARGET_CLN)
