# ======= 최소·안정 Makefile =======
.SUFFIXES:
MAKEFLAGS += --no-builtin-rules

# ---- 타깃 이름들 ----
APP      := ledkeyi2c_app           # 실행파일 이름
APP_SRC  := ledkeyi2c_app.c         # 앱 소스
MODS     := ledkey_dev ledkeyi2c_dev # 커널 모듈 2개 (파일명과 정확히 일치)

# ---- 교차 컴파일러 / 커널 트리 ----
ARCH          := arm
CROSS_COMPILE := arm-linux-gnueabihf-
CC            := $(CROSS_COMPILE)gcc

KDIR := /home/ubuntu/pi_bsp/kernel/linux
PWD  := $(shell pwd)

# obj-m 규칙
obj-m += $(addsuffix .o, $(MODS))

.PHONY: default all app module clean

default: all
all: app module
	@echo "Build done."

# ===== 사용자 앱 빌드 =====
app: $(APP)

$(APP): $(APP_SRC)
	$(CC) -O2 -Wall -o $@ $<
	cp $@ /srv/nfs_ubuntu

# ===== 커널 모듈 빌드 =====
module:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules
	@for m in $(MODS); do cp $$m.ko /srv/nfs_ubuntu; done

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f $(APP)
