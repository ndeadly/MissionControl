PROJECT_NAME := MissionControl
BLUETOOTH_MITM_TID := 010000000000bd00

TARGETS := bluetooth-mitm

all: $(TARGETS)

bluetooth-mitm:
	$(MAKE) -C $@

clean:
	$(MAKE) -C bluetooth-mitm clean
	rm -rf dist

dist: all
	rm -rf dist
	
	mkdir -p dist/atmosphere/contents/$(BLUETOOTH_MITM_TID)
	cp bluetooth-mitm/bluetooth-mitm.nsp dist/atmosphere/contents/$(BLUETOOTH_MITM_TID)/exefs.nsp
	echo "btdrv" >> dist/atmosphere/contents/$(BLUETOOTH_MITM_TID)/mitm.lst
	echo "btm" >> dist/atmosphere/contents/$(BLUETOOTH_MITM_TID)/mitm.lst

	mkdir -p dist/atmosphere/contents/$(BLUETOOTH_MITM_TID)/flags
	touch dist/atmosphere/contents/$(BLUETOOTH_MITM_TID)/flags/boot2.flag
	
	cp -r exefs_patches dist/atmosphere/
	
	cd dist; zip -r $(PROJECT_NAME).zip ./*; cd ../;
	
.PHONY: all clean dist $(TARGETS)
