# pthread(POSIX Threads)

## 기본 개념

1. `pthread`는 POSIX 시스템에서 사용하는 **스레드 라이브러리**이다.
2. 하나의 프로세스 내에서 여러 \*\*작업 단위(스레드)\*\*를 병렬로 실행할 수 있게 한다.
3. **병렬성 향상, 자원 공유, 응답성 개선** 등을 목적으로 사용된다.



## 주요 특징

1. 경량 스레드(Lightweight Thread)

   * 프로세스보다 생성/전환 비용이 작음

2. 자원 공유

   * 같은 프로세스 내 스레드끼리는 **코드, 데이터, 힙, 열린 파일** 등을 공유함

3. 병렬 처리

   * 여러 작업을 동시에 처리할 수 있음 (예: 서버에서 다수의 클라이언트 처리)

4. 동기화 필요

   * 공유 자원을 접근할 때 **mutex, condition variable** 등을 통해 동기화가 필요함



## 스레드 생성과 제어 함수

| 함수명             | 설명                                        |
| ------------------ | ------------------------------------------- |
| `pthread_create()` | 스레드를 생성하고 실행                      |
| `pthread_join()`   | 다른 스레드의 종료를 기다림                 |
| `pthread_exit()`   | 현재 스레드를 종료                          |
| `pthread_cancel()` | 대상 스레드를 강제로 종료                   |
| `pthread_detach()` | 스레드 리소스를 자동으로 회수 (join 불필요) |

### pthread\_create()

```c
int pthread_create(pthread_t *thread,
                   const pthread_attr_t *attr,
                   void *(*start_routine)(void *),
                   void *arg);
```

* `thread`: 생성된 스레드 ID가 저장될 변수
* `attr`: 스레드 속성 (NULL이면 기본값)
* `start_routine`: 실행할 함수
* `arg`: 함수에 전달할 인자 (void\* 타입)

> 반환값: 성공 시 0, 실패 시 에러 번호



### pthread\_join()

```c
int pthread_join(pthread_t thread, void **retval);
```

* `thread`: 기다릴 스레드 ID
* `retval`: 스레드가 반환한 값 포인터

> `join`하지 않으면 좀비 스레드가 발생할 수 있음



### pthread\_exit()

```c
void pthread_exit(void *retval);
```

* 현재 스레드를 종료하고 `retval`을 호출한 측에 반환

> `main()` 함수에서 호출되면, 해당 스레드만 종료되고 프로세스는 유지됨



### pthread\_cancel()

```c
int pthread_cancel(pthread_t thread);
```

* 지정한 스레드에 **취소 요청**을 보냄 (즉시 종료되지 않음)

> 취소 지점에서만 반응함 (`read`, `write`, `pthread_testcancel()` 등)



### pthread\_detach()

```c
int pthread_detach(pthread_t thread);
```

* 스레드를 **detach 상태**로 만들면, 종료 시 자동으로 리소스 정리됨
* `pthread_join()` 호출 필요 없음



## 동기화 관련 함수

| 함수                      | 설명                 |
| ------------------------- | -------------------- |
| `pthread_mutex_init()`    | 뮤텍스 생성          |
| `pthread_mutex_lock()`    | 뮤텍스 잠금 (lock)   |
| `pthread_mutex_unlock()`  | 뮤텍스 해제 (unlock) |
| `pthread_mutex_destroy()` | 뮤텍스 제거          |



## pthread 헤더 구조

```c
#include <pthread.h>
```

* POSIX 스레드 API는 `pthread.h`에서 정의됨
* 컴파일 시 반드시 `-pthread` 옵션 필요

```sh
gcc example.c -o example -pthread
```



## 동작 흐름 요약

1. `pthread_create()`로 스레드 생성
2. 스레드는 `void* func(void* arg)` 형태의 함수 실행
3. `pthread_join()` 또는 `pthread_detach()`로 종료 대기 혹은 자동 종료 처리
4. 필요한 경우 `pthread_mutex_*()` 계열로 공유 자원 접근 보호





## 사용 예시

* 멀티 클라이언트 서버 (TCP 다중 접속 처리)
* 병렬 연산 (멀티코어 연산 가속)
* UI/백그라운드 작업 분리
* 생산자-소비자 문제, 쓰레드 풀 구현 등



## pthread vs fork 비교

| 항목        | pthread          | fork                         |
| ----------- | ---------------- | ---------------------------- |
| 자원 공유   | 메모리/파일 공유 | 완전한 복사 (독립 주소 공간) |
| 생성 비용   | 낮음 (가볍다)    | 높음                         |
| 병렬성      | 매우 좋음        | 병렬은 가능하지만 무겁다     |
| 프로세스 ID | 동일             | 각각의 PID 가짐              |

<br>

# Mutex

## 1. 기본 개념

1.1 `mutex`는 **공유 자원에 대한 접근을 하나의 스레드만 허용**하도록 하는 **락(lock)** 개념이다.
1.2 한 스레드가 `mutex`를 획득하면, 다른 스레드는 해당 자원이 **해제될 때까지 대기**한다.
1.3 `pthread_mutex_t` 구조체를 통해 사용한다.



## 2. 사용 목적

* **데이터 레이스(Data Race)** 방지
* **비일관성 상태**(shared 변수 변경 도중 접근) 방지
* **멀티스레드 환경에서의 안전성 확보**



## 3. 주요 함수

| 함수                      | 설명                        |
| ------------------------- | --------------------------- |
| `pthread_mutex_init()`    | mutex 초기화                |
| `pthread_mutex_destroy()` | mutex 자원 해제             |
| `pthread_mutex_lock()`    | mutex 잠금 (자원 사용 시작) |
| `pthread_mutex_trylock()` | mutex 잠금 시도 (비차단)    |
| `pthread_mutex_unlock()`  | mutex 해제 (자원 사용 종료) |



## 4. 예제 코드

```c
#include <pthread.h>
#include <stdio.h>

int shared_data = 0;
pthread_mutex_t lock;

void* thread_func(void* arg)
{
    for (int i = 0; i < 100000; i++) {
        pthread_mutex_lock(&lock);     // 잠금
        shared_data++;                 // 공유 자원 접근
        pthread_mutex_unlock(&lock);   // 해제
    }
    return NULL;
}

int main()
{
    pthread_t t1, t2;
    pthread_mutex_init(&lock, NULL);   // mutex 초기화

    pthread_create(&t1, NULL, thread_func, NULL);
    pthread_create(&t2, NULL, thread_func, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("최종 값: %d\n", shared_data);  // 예상: 200000

    pthread_mutex_destroy(&lock);      // mutex 제거
    return 0;
}
```



## 5. mutex 동작 원리

1. 스레드 A가 `pthread_mutex_lock()` 호출 시 mutex 소유
2. 그 상태에서 스레드 B가 같은 mutex를 `lock`하려 하면 **block(대기)** 상태
3. A가 `pthread_mutex_unlock()` 하면 B가 그 mutex를 획득



## 6. 주의 사항

1. **락을 걸고 반드시 해제해야 함** (락 해제하지 않으면 데드락 발생 가능)
2. `pthread_mutex_lock()`은 블로킹, `pthread_mutex_trylock()`은 비블로킹
3. `mutex`는 공유자원별로 **하나씩** 정의해야 효과 있음
4. **재귀 락 불가능 (같은 스레드에서 두 번 lock 시 데드락 발생)**
   → 필요 시 `PTHREAD_MUTEX_RECURSIVE` 속성 사용



## 7. 기타

* **데드락(Deadlock)**: 두 개 이상의 스레드가 서로 자원의 락을 기다리며 영원히 멈춤
* **경쟁 조건(Race Condition)**: 두 스레드가 동시에 자원을 변경하여 예기치 않은 결과 발생


## 결론

`mutex`는 멀티스레드 환경에서 **공유 자원의 동기화**를 보장하기 위한 핵심 메커니즘이다.
적절히 사용하면 **데이터 일관성 유지**와 **스레드 안전성 확보**가 가능하다.

