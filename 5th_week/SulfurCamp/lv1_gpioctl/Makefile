# ======= 최소·안정 Makefile =======
.SUFFIXES:                 # 내장 접미사 규칙 전부 제거
MAKEFLAGS += --no-builtin-rules

APP := ledkey_app          # 생성될 실행파일 이름
APP_SRC := ledkey_app.c    # ← 실제 소스 파일명으로 바꾸세요 (예: app.c)

MOD := ledkey_dev
obj-m := $(MOD).o

ARCH := arm
CROSS_COMPILE := arm-linux-gnueabihf-
CC := $(CROSS_COMPILE)gcc

KDIR := /home/ubuntu/pi_bsp/kernel/linux
PWD := $(shell pwd)

.PHONY: default all app module clean

default: all

all: app module
	@echo "Build done."

# ===== 사용자 앱 빌드(명시적 규칙) =====
app: $(APP)

$(APP): $(APP_SRC)
	$(CC) $(APP_SRC) -o $(APP)
	cp $(APP) /srv/nfs_ubuntu

# ===== 커널 모듈 빌드 =====
module:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules
	cp $(MOD).ko /srv/nfs_ubuntu

clean:
	rm -rf *.ko *.mod.* .*.cmd *.o modules.order Module.symvers .tmp_versions
	rm -rf $(MOD).mod
	rm -rf $(APP)
