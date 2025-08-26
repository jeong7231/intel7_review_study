# p238_ledkey
key로 led 제어하는 예제
## call_ledkey_app.c

### 1. 디바이스 파일 open
```c++
    if(access(DEVICE_FILENAME,F_OK) != 0)   // 파일 없을 시
    {
        int ret = mknod("/dev/ledkey", S_IRWXU|S_IRWXG|S_IFCHR, (230<<8) | 0);
        if(ret < 0)
            perror("mknod()");
    }
	
	dev = open(DEVICE_FILENAME, O_RDWR | O_NDELAY); 
	//dev = open(DEVICE_FILENAME, O_RDWR); 
	if(dev < 0)
	{
		perror("open");
		return 2;
	}
```

### 2. read & write
```c++
	do{
		usleep(100000);	//100msec
		ret = read(dev, &buff, sizeof(buff));
		if(ret < 0)		//에러 처리
		{
			perror("read()");
			return 3;
		}

		ret = write(dev, &key, sizeof(key));
		if(ret < 0)		//에러 처리
		{
			perror("write()");
			return 4;
		}

		key_data = (int)buff;

		if(key_data < 0)	//에러 처리
		{
			printf("key_data: %d\n", key_data);
			perror("syscall()");
		}
		if(key_data_old == 0x80)
			break;
```
read : 키 값 read  
write : led에 write  

## 3. O X 출력 처리
```c++
		if(key_data != key_data_old)
		{
			key_data_old = key_data;
			if(key_data)
			{
				key = key_data;
				printf("key_data : %d\n", key_data);

				printf("0:1:2:3:4:5:6:7\n");
				for(int i = 0; i < 8; i++)
				{
					if((key_data >> i) & 0x01)
						putchar('O');
					else
						putchar('X');
					if(i != 7)
						putchar(':');
					else
						putchar('\n');
				}
				putchar('\n');
			}
		}
	}while(1);	
```

### 4. 디바이스 파일 닫기
```c++
	ret = close(dev);
	printf("ret = %08X\n", ret);

	return 0;
}
```