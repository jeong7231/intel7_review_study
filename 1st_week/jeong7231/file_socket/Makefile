CC:=gcc
#CC:=arm-linux-gcc

TARGET_SRV=file_server
OBJECT_SRV=$(TARGET_SRV).o 

TARGET_CLN=file_client
OBJECT_CLN=$(TARGET_CLN).o 

LDFLAGS=-D_REENTRANT -pthread

all : $(TARGET_SRV) $(TARGET_CLN)

$(TARGET_SRV):$(OBJECT_SRV)
	$(CC) -o $@ $(OBJECT_SRV) $(LDFLAGS)
$(TARGET_CLN):$(OBJECT_CLN)
	$(CC) -o $@ $(OBJECT_CLN) $(LDFLAGS)
%.o:%.c
	$(CC) -c -o $@ $<
clean:
	rm -f *.o $(TARGET_SRV) $(TARGET_CLN)
