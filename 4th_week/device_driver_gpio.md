# BCM2711 GPIO 디바이스 드라이버 제작 가이드

## 1. 개요

본 문서는 라즈베리파이4의 BCM2711 프로세서에 연결된 LED와 버튼을 제어하는 리눅스 캐릭터 디바이스 드라이버 제작 과정을 기술한다. `p432_ledkey_poll_ksh` 예제 코드를 기반으로, `poll` 시스템 콜을 활용한 사용자 입력의 비동기적 처리 방법을 설명한다.

## 2. 사전 준비

- 라즈베리파이 4 (또는 BCM2711 기반 보드)
- 리눅스 커널 소스 트리 및 빌드 환경
- ARM 크로스 컴파일러 (`arm-linux-gnueabihf-`)
- LED 및 버튼 회로 (예제 코드 기준: LED 8개, 버튼 8개)

## 3. 핵심 개념

### 3.1. 캐릭터 디바이스 드라이버

리눅스 디바이스는 블록, 캐릭터, 네트워크 디바이스로 분류된다. GPIO와 같이 순차적 데이터 흐름을 갖는 하드웨어는 캐릭터 디바이스로 구현된다. `/dev` 디렉터리에 생성된 디바이스 파일을 통해 `open`, `read`, `write`, `close` 등 표준 파일 연산으로 제어된다.

### 3.2. GPIO 제어

커널은 GPIO 제어를 위한 표준 API를 제공한다.

- `gpio_request()`: 사용할 GPIO 핀을 시스템에 요청한다.
- `gpio_direction_input()` / `gpio_direction_output()`: GPIO 핀의 방향(입력/출력)을 설정한다.
- `gpio_set_value()`: 출력 핀의 값을 설정한다 (HIGH/LOW).
- `gpio_get_value()`: 입력 핀의 값을 읽는다.
- `gpio_to_irq()`: GPIO 핀 번호를 인터럽트 번호(IRQ)로 변환한다.
- `gpio_free()`: 사용 완료한 GPIO 핀을 시스템에 반환한다.

### 3.3. 인터럽트 (Interrupt)

버튼 입력과 같이 비정기적 이벤트는 인터럽트로 처리한다. 인터럽트 발생 시, CPU는 현재 작업을 중단하고 등록된 서비스 루틴(ISR)을 실행하여 효율적인 이벤트 처리가 가능하다.

- `request_irq()`: 특정 IRQ에 대한 인터럽트 핸들러(ISR)를 등록한다.
- `free_irq()`: 등록된 인터럽트 핸들러를 해제한다.

### 3.4. poll()을 이용한 비동기 I/O

사용자 공간 앱이 읽을 데이터가 생길 때까지 대기하는 블로킹(Blocking I/O) 방식은 비효율적이다. `poll` 시스템 콜은 여러 파일 디스크립터를 감시하여 특정 이벤트 발생 여부를 즉시 반환한다. 이를 통해 앱은 대기 시간 없이 여러 I/O를 효율적으로 관리할 수 있다.

- `wait_queue_head_t`: 이벤트 발생을 대기하는 프로세스들의 큐이다.
- `wake_up_interruptible()`: 대기 큐에서 대기 중인 프로세스를 깨운다.
- `poll_wait()`: `file_operations`의 `poll` 핸들러 내에서 호출된다. 특정 대기 큐에 현재 프로세스를 추가하여 이벤트 대기 목록에 등록한다.

## 4. 소스 코드 분석 (ledkey_poll_dev.c)

### 4.1. 초기화 및 종료

`module_init`과 `module_exit`는 커널 모듈 등록 및 해제에 사용된다.

```c
// ledkey_init: 모듈 로드 시 호출
static int __init ledkey_init(void)
{
    // 1. 문자 디바이스 등록
    result = register_chrdev(LEDKEY_DEV_MAJOR, LEDKEY_DEV_NAME, &ledkey_fops);
    
    // 2. GPIO 초기화 (LED, Key)
    gpioLedInit();
    gpioKeyInit();

    // 3. 인터럽트 초기화 및 핸들러 등록
    irqKeyInit();
    for(int i=0; i<GPIOCNT; i++) {
        request_irq(irqKey[i], keyIsr, IRQF_TRIGGER_RISING, ...);
    }
    
    // 4. 뮤텍스 초기화
    mutex_init(&keyMutex);
    
    return 0;
}

// ledkey_exit: 모듈 언로드 시 호출
static void __exit ledkey_exit(void)
{
    // 등록 해제 (초기화의 역순)
    unregister_chrdev(LEDKEY_DEV_MAJOR, LEDKEY_DEV_NAME);
    irqKeyFree();
    gpioLedFree();
    gpioKeyFree();
    mutex_destroy(&keyMutex);
}

module_init(ledkey_init);
module_exit(ledkey_exit);
```

### 4.2. File Operations 및 poll() 구현

`file_operations` 구조체는 디바이스 파일에 대한 연산을 정의한다. 본 예제에서는 `read`와 `poll`이 핵심이다.

```c
// file_operations 구조체 정의
struct file_operations ledkey_fops =
{
    .open     = ledkey_open,     
    .read     = ledkey_read,     
    .write    = ledkey_write,    
	.poll	  = ledkey_poll,
    .release  = ledkey_release,  
};

// 대기 큐 선언
static DECLARE_WAIT_QUEUE_HEAD(waitQueueRead);
static int keyNum = 0; // ISR이 기록하는 눌린 버튼 번호

// 인터럽트 서비스 루틴 (ISR)
static irqreturn_t keyIsr(int irq, void * data)
{
    // 1. 눌린 버튼 식별 후 keyNum에 저장
    keyNum = i + 1;

    // 2. 대기 큐에서 잠든 프로세스를 깨움
	wake_up_interruptible(&waitQueueRead);
	return IRQ_HANDLED;
}

// read 핸들러
static ssize_t ledkey_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    // 블로킹 모드일 경우, keyNum이 0이 아닐 때까지 대기
	if(!(filp->f_flags & O_NONBLOCK))
	{
		wait_event_interruptible(waitQueueRead, keyNum);
	}
    
    // keyNum 값을 사용자 공간으로 복사
    put_user((char)keyNum, buf);
    keyNum = 0; // 값 전달 후 초기화
    
    return count;
}

// poll 핸들러
static __poll_t ledkey_poll(struct file * filp, struct poll_table_struct *wait)
{
	unsigned int mask = 0;

    // 1. waitQueueRead 대기 큐에 현재 프로세스를 등록
	poll_wait(filp, &waitQueueRead, wait);

    // 2. 읽을 데이터(keyNum > 0)가 있으면 POLLIN 마스크를 리턴
	if(keyNum > 0)
		mask = POLLIN;

	return mask;	
}
```

**동작 흐름:**
1.  사용자 앱의 `poll()` 호출 시, 커널의 `ledkey_poll()` 함수가 실행된다.
2.  `poll_wait()`는 `waitQueueRead` 대기 큐를 감시하도록 시스템에 등록한다. `keyNum`에 읽을 데이터가 없으므로 `ledkey_poll`은 0을 반환하고 즉시 종료하며, 앱은 다른 작업을 계속 수행한다.
3.  버튼 입력 시 하드웨어 인터럽트가 발생하고, 등록된 `keyIsr()`이 실행된다.
4.  `keyIsr()`은 눌린 버튼 번호를 `keyNum`에 저장하고, `wake_up_interruptible()`을 호출하여 `waitQueueRead`에서 대기 중인 프로세스를 깨운다.
5.  `poll()`을 호출했던 앱은 `poll()`로부터 `POLLIN` 이벤트를 반환받고, `read()`를 호출하여 `keyNum` 값을 읽어간다.

### 4.3. 사용자 공간 애플리케이션 (ledkey_poll_app.c)

사용자 앱은 `poll()`을 사용하여 표준 입력(키보드)과 디바이스 드라이버(/dev/ledkey)를 동시에 감시한다.

```c
#include <poll.h>

int main(void)
{
    struct pollfd Events[2];
    int dev = open("/dev/ledkey", O_RDWR);
    
    // 1. pollfd 구조체 설정
    // Events[0]: 표준 입력 (stdin)
    Events[0].fd = fileno(stdin);
    Events[0].events = POLLIN; // 감시 이벤트: 읽을 데이터 유무
    
    // Events[1]: 디바이스 드라이버
    Events[1].fd = dev;
    Events[1].events = POLLIN;

    while(1)
    {
        // 2. poll() 호출로 두 파일 디스크립터를 감시 (2초 타임아웃)
        int ret = poll(Events, 2, 2000);
        
        if (ret == 0) {
            printf("poll time out
");
            continue;
        }

        // 3. 이벤트 발생 확인
        if (Events[0].revents & POLLIN) { // 표준 입력 이벤트
            // 키보드 입력 처리
        }
        else if (Events[1].revents & POLLIN) { // 디바이스 드라이버 이벤트
            char buff;
            read(dev, &buff, sizeof(buff));
            printf("key_no : %d
", buff);
        }
    }
    close(dev);
    return 0;
}
```

## 5. 빌드 및 테스트

### 5.1. Makefile 분석

`Makefile`은 드라이버 모듈(`.ko`)과 테스트 앱을 빌드하는 역할을 한다.

```makefile
APP := ledkey_poll_app
MOD := ledkey_poll_dev
obj-m := $(MOD).o

# ARM 크로스 컴파일러 및 커널 소스 경로 설정
CROSS = ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
KDIR := /home/ubuntu/pi_bsp/kernel/linux
PWD := $(shell pwd)

default: clean $(APP)
	$(MAKE) -C $(KDIR) M=$(PWD) modules $(CROSS) # 커널 모듈 빌드
	cp $(MOD).ko /srv/nfs_ubuntu # NFS로 복사

$(APP):
	$(CC) $(APP).c -o $(APP) # 테스트 앱 빌드
	cp $(APP) /srv/nfs_ubuntu # NFS로 복사

clean:
	# ... (빌드 결과물 삭제) ...
```

### 5.2. 빌드

Makefile 위치에서 `make` 명령을 실행한다.

```sh
make
```

### 5.3. 테스트 (라즈베리파이)

1.  **디바이스 노드 생성**: 드라이버에 명시된 주 번호(230)로 디바이스 파일을 생성한다.
    ```sh
    sudo mknod /dev/ledkey c 230 0
    ```

2.  **커널 모듈 로드**:
    ```sh
    sudo insmod ledkey_poll_dev.ko
    ```
    `dmesg` 명령으로 모듈 로드 상태를 확인할 수 있다.

3.  **테스트 앱 실행**:
    ```sh
    ./ledkey_poll_app 0x0f # LED 1~4 점등
    ```
    - 앱 실행 후 버튼 입력 시, `key_no: [번호]` 형식으로 콘솔에 출력된다.
    - 터미널에 `q` 입력 후 Enter 시 앱이 종료된다.

4.  **커널 모듈 언로드**:
    ```sh
    sudo rmmod ledkey_poll_dev
    ```

