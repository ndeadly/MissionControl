PROJECT_NAME := MissionControl
MC_MITM_TID := 010000000000bd00

export LIBNX ?= $(CURDIR)/build_libnx/libnx
LIBNX_REVISION := $(shell git -C lib/libnx rev-parse HEAD)

GIT_BRANCH := $(shell git symbolic-ref --short HEAD | sed s/[^a-zA-Z0-9_-]/_/g)
GIT_HASH := $(shell git rev-parse --short HEAD)
GIT_TAG := $(shell git describe --tags `git rev-list --tags --max-count=1`)

VERSION := $(shell printf "0x%02X%02X%02X" $(shell echo "$(GIT_TAG)" | sed -E 's/^v([0-9]+).([0-9]+).([0-9]+)/\1 \2 \3/g'))
BUILD_VERSION := $(GIT_TAG:v%=%)-$(GIT_BRANCH)-$(GIT_HASH)
BUILD_DATE := $(shell date)

GIT_HEAD := $(shell git rev-parse --git-path HEAD)
GIT_INDEX := $(shell git rev-parse --git-path index)

TARGETS := mcmitm_version.cpp mc_mitm

all: $(TARGETS)

mcmitm_version.cpp: $(GIT_HEAD) $(GIT_INDEX)
	echo "namespace ams::mc { unsigned int mc_version = $(VERSION); const char *mc_build_name = \"$(BUILD_VERSION)\"; const char *mc_build_date = \"$(BUILD_DATE)\"; }" > mc_mitm/source/$@

mc_mitm: mcmitm_version.cpp prepare_libnx
	$(MAKE) -C $@

# The system libnx package does not supply this build's staged headers/library.
# Refresh them together when the pinned source changes; keep external LIBNX overrides.
prepare_libnx:
ifeq ($(abspath $(LIBNX)),$(CURDIR)/build_libnx/libnx)
	@if test "$$(cat $(LIBNX)/.mc-revision 2>/dev/null)" != "$(LIBNX_REVISION)" || \
	    ! test -f $(LIBNX)/lib/libnx.a || ! git -C lib/libnx diff --quiet HEAD; then \
	    rm -rf $(CURDIR)/build_libnx/stage && \
	    $(MAKE) -C lib/libnx/nx install DESTDIR=$(CURDIR)/build_libnx/stage && \
	    rm -rf $(LIBNX) && \
	    mkdir -p $(LIBNX) && \
	    cp -a $(CURDIR)/build_libnx/stage$(DEVKITPRO)/libnx/. $(LIBNX)/ && \
	    echo "$(LIBNX_REVISION)" > $(LIBNX)/.mc-revision; \
	fi
endif

clean:
	$(MAKE) -C mc_mitm clean
	rm mc_mitm/source/mcmitm_version.cpp
	rm -rf dist

dist: all
	rm -rf dist

	

	mkdir -p dist/atmosphere/contents/$(MC_MITM_TID)
	cp mc_mitm/out/nintendo_nx_arm64_armv8a/release/mc_mitm.nsp dist/atmosphere/contents/$(MC_MITM_TID)/exefs.nsp
	echo "btdrv" >> dist/atmosphere/contents/$(MC_MITM_TID)/mitm.lst
	echo "btm" >> dist/atmosphere/contents/$(MC_MITM_TID)/mitm.lst

	mkdir -p dist/atmosphere/contents/$(MC_MITM_TID)/flags
	touch dist/atmosphere/contents/$(MC_MITM_TID)/flags/boot2.flag

	cp mc_mitm/toolbox.json dist/atmosphere/contents/$(MC_MITM_TID)/toolbox.json

	cp -r exefs_patches dist/atmosphere/

	mkdir -p dist/config/MissionControl
	mkdir -p dist/config/MissionControl/controllers
	cp mc_mitm/config.ini dist/config/MissionControl/missioncontrol.ini.template

	cd dist; zip -r $(PROJECT_NAME)-$(BUILD_VERSION).zip ./*; cd ../;

.PHONY: all clean dist prepare_libnx $(TARGETS)
