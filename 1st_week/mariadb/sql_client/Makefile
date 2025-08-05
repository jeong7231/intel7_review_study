#CC:=arm-linux-gcc
CC:=gcc

TARGET_CLN_SENSOR=iot_client_sensor
OBJECT_CLN_SENSOR=$(TARGET_CLN_SENSOR).o 

LDFLAGS=-D_REENTRANT -pthread
LDFLAGS_SQL=-D_REENTRANT -pthread -lmysqlclient

all : $(TARGET_CLN_SENSOR) 

$(TARGET_CLN_SENSOR):$(OBJECT_CLN_SENSOR)
	$(CC) -o $@ $(OBJECT_CLN_SENSOR) $(LDFLAGS_SQL)
%.o:%.c
	$(CC) -c -o $@ $<
clean:
	rm -f *.o $(TARGET_CLN_SENSOR)
