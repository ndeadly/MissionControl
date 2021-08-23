PROJECT_NAME := MissionControl
MC_MITM_TID := 010000000000bd00

GIT_BRANCH := $(shell git symbolic-ref --short HEAD | sed s/[^a-zA-Z0-9_-]/_/g)
GIT_HASH := $(shell git rev-parse --short HEAD)
GIT_TAG := $(shell git describe --tags `git rev-list --tags --max-count=1`)

VERSION := $(shell [[ $(GIT_TAG) =~ ^v([0-9]+).([0-9]+).([0-9]+) ]] && printf '0x%02X%02X%02X' $${BASH_REMATCH[1]} $${BASH_REMATCH[2]} $${BASH_REMATCH[3]})
BUILD_VERSION := $(GIT_TAG:v%=%)-$(GIT_BRANCH)-$(GIT_HASH)

TARGETS := mcmitm_version.cpp mc_mitm

all: $(TARGETS)

mcmitm_version.cpp: .git/HEAD .git/index
	echo "namespace ams::mitm { unsigned int mc_version = $(VERSION); const char *mc_build_name = \"$(BUILD_VERSION)\"; const char *mc_build_date = \"$$(date)\"; }" > mc_mitm/source/$@

mc_mitm:
	$(MAKE) -C $@

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

.PHONY: all clean dist $(TARGETS)
