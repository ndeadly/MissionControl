PROJECT_NAME := MissionControl
MC_MITM_TID := 010000000000bd00

GIT_BRANCH := $(shell git symbolic-ref --short HEAD)
GIT_HASH := $(shell git rev-parse --short HEAD)
GIT_TAG := $(shell git describe --tags `git rev-list --tags --max-count=1`)
BUILD_VERSION := $(GIT_TAG:v%=%)-$(GIT_BRANCH)-$(GIT_HASH)

TARGETS := mc_mitm

all: $(TARGETS)

mc_mitm:
	$(MAKE) -C $@

clean:
	$(MAKE) -C mc_mitm clean
	rm -rf dist

dist: all
	rm -rf dist
	
	mkdir -p dist/atmosphere/contents/$(MC_MITM_TID)
	cp mc_mitm/mc_mitm.nsp dist/atmosphere/contents/$(MC_MITM_TID)/exefs.nsp
	echo "btdrv" >> dist/atmosphere/contents/$(MC_MITM_TID)/mitm.lst
	echo "btm" >> dist/atmosphere/contents/$(MC_MITM_TID)/mitm.lst

	mkdir -p dist/atmosphere/contents/$(MC_MITM_TID)/flags
	touch dist/atmosphere/contents/$(MC_MITM_TID)/flags/boot2.flag
	
	cp mc_mitm/toolbox.json dist/atmosphere/contents/$(MC_MITM_TID)/toolbox.json
	
	cp -r exefs_patches dist/atmosphere/
	
	cd dist; zip -r $(PROJECT_NAME)-$(BUILD_VERSION).zip ./*; cd ../;
	
.PHONY: all clean dist $(TARGETS)
