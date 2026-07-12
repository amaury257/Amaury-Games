# Makefile — River Raid (Fase 0: gate de toolchain)
# Template "application" do Theos. Build fecha em WSL2/Linux, sem Mac/Xcode.
# "latest" seleciona automaticamente o SDK mais novo presente em $THEOS/sdks;
# a versão detectada é registrada pelo scripts/fase0-gate.sh em docs/fase0-resultados.md.
TARGET := iphone:clang:latest:26.0
ARCHS := arm64

INSTALL_TARGET_PROCESSES = RiverRaid

include $(THEOS)/makefiles/common.mk

APPLICATION_NAME = RiverRaid

RiverRaid_FILES = $(wildcard Sources/App/*.m) \
                  $(wildcard Sources/Core/*.m) \
                  $(wildcard Sources/Game/*.m) \
                  $(wildcard Sources/Shell/*.m) \
                  $(wildcard Sources/Game/engine/*.c)
RiverRaid_FRAMEWORKS = UIKit QuartzCore Metal AVFoundation GameController CoreHaptics
RiverRaid_CFLAGS = -fobjc-arc -Wall -O2 \
                   -ISources/App -ISources/Core -ISources/Game -ISources/Game/engine -ISources/Shell

include $(THEOS)/makefiles/application.mk

APP_STAGE_DIR = $(THEOS_STAGING_DIR)/Applications/$(APPLICATION_NAME).app

# Toolchain/SDK detectados sem depender de variáveis internas do Theos
RR_CLANG := $(shell ls "$(THEOS)/toolchain/linux/iphone/bin/clang" 2>/dev/null || command -v clang)
RR_SDK   := $(shell ls -d "$(THEOS)"/sdks/iPhoneOS*.sdk 2>/dev/null | sort -V | tail -n1)

# Fase 0 — compila a dylib dummy e a embarca em Frameworks/ para o teste de
# dlopen no aparelho (decide D3-A vs D3-B). A assinatura local via ldid é
# opcional: o AltStore reassina o app e os frameworks embarcados na instalação.
after-stage::
	@mkdir -p "$(APP_STAGE_DIR)/Frameworks"
	"$(RR_CLANG)" -target arm64-apple-ios26.0 -isysroot "$(RR_SDK)" \
		-dynamiclib \
		-install_name @rpath/libdummy.dylib \
		-o "$(APP_STAGE_DIR)/Frameworks/libdummy.dylib" \
		Sources/Fase0/dummy.c
	-ldid -S "$(APP_STAGE_DIR)/Frameworks/libdummy.dylib"
