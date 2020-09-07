PROJECT_NAME := MissionControl
BTDRVMITM_TID := 010000000000bd00

TARGETS := btdrv-mitm

all: $(TARGETS)

btdrv-mitm:
	$(MAKE) -C $@

clean:
	$(MAKE) -C Atmosphere/libraries clean
	$(MAKE) -C btdrv-mitm clean
	rm -rf dist

dist: all
	rm -rf dist
	
	mkdir -p dist/atmosphere/contents/$(BTDRVMITM_TID)
	cp btdrv-mitm/btdrv-mitm.nsp dist/atmosphere/contents/$(BTDRVMITM_TID)/exefs.nsp
	echo "btdrv" > dist/atmosphere/contents/$(BTDRVMITM_TID)/mitm.lst

	mkdir -p dist/atmosphere/contents/$(BTDRVMITM_TID)/flags
	touch dist/atmosphere/contents/$(BTDRVMITM_TID)/flags/boot2.flag
	
	cp -r exefs_patches dist/atmosphere/
	
	cd dist; zip -r $(PROJECT_NAME).zip ./*; cd ../;
	
.PHONY: all clean dist $(TARGETS)
