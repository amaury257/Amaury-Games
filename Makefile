# Makefile — River Raid (Fase 0: gate de toolchain)
# Template "application" do Theos. Build fecha em WSL2/Linux, sem Mac/Xcode.
# "latest" seleciona automaticamente o SDK mais novo presente em $THEOS/sdks;
# a versão detectada é registrada pelo scripts/fase0-gate.sh em docs/fase0-resultados.md.
TARGET := iphone:clang:latest:26.0
ARCHS := arm64

INSTALL_TARGET_PROCESSES = RiverRaid

include $(THEOS)/makefiles/common.mk

APPLICATION_NAME = RiverRaid

RiverRaid_FILES = $(wildcard Sources/App/*.m)
RiverRaid_FRAMEWORKS = UIKit
RiverRaid_CFLAGS = -fobjc-arc -Wall

include $(THEOS)/makefiles/application.mk

APP_STAGE_DIR = $(THEOS_STAGING_DIR)/Applications/$(APPLICATION_NAME).app

# Fase 0 — compila a dylib dummy e a embarca em Frameworks/ para o teste de
# dlopen no aparelho (decide D3-A vs D3-B). A assinatura local via ldid é
# opcional: o AltStore reassina o app e os frameworks embarcados na instalação.
after-stage::
	@mkdir -p "$(APP_STAGE_DIR)/Frameworks"
	$(TARGET_CC) -dynamiclib -arch arm64 -isysroot "$(SYSROOT)" \
		-miphoneos-version-min=26.0 \
		-install_name @rpath/libdummy.dylib \
		-o "$(APP_STAGE_DIR)/Frameworks/libdummy.dylib" \
		Sources/Fase0/dummy.c
	-ldid -S "$(APP_STAGE_DIR)/Frameworks/libdummy.dylib"
