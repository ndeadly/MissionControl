PROJECT_NAME := MissionControl
BTDRVMITM_TID := 010000000000bd00

TARGETS := boot2 btdrv-mitm

all: $(TARGETS)

boot2:
	$(MAKE) -C Atmosphere/libraries all
	$(MAKE) -C Atmosphere/stratosphere/boot2 all

btdrv-mitm:
	$(MAKE) -C $@

clean:
	$(MAKE) -C Atmosphere/libraries clean
	$(MAKE) -C Atmosphere/stratosphere/boot2 clean
	$(MAKE) -C btdrv-mitm clean
	rm -rf dist

dist: all
	rm -rf dist
	
	mkdir -p dist/atmosphere/contents/$(BTDRVMITM_TID)
	cp btdrv-mitm/btdrv-mitm.nsp dist/atmosphere/contents/$(BTDRVMITM_TID)/exefs.nsp
	cp Atmosphere/stratosphere/boot2/boot2.nsp dist/atmosphere/contents/0100000000000008/exefs.nsp
	#mkdir -p dist/atmosphere/contents/$(BTDRVMITM_TID)/flags
	#touch dist/atmosphere/contents/$(BTDRVMITM_TID)/flags/boot2.flag
	#echo "btdrv" > mitm.lst
	
	cp -r exefs_patches dist/atmosphere/
	
	cd dist; zip -r $(PROJECT_NAME).zip ./*; cd ../;
	
.PHONY: all clean dist $(TARGETS)
