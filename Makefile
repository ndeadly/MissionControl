
export PROJECT_NAME = MissionControl
export SYSMODULE_TID = 010000000000b100
export CONTROLLER_TID = 0100000000001003

TARGETS := sysmodule applet

.PHONY: all clean dist

all: $(TARGETS)

sysmodule:
	$(MAKE) -C $@

applet:
	$(MAKE) -C $@

clean:
	rm -rf dist
	$(MAKE) -C sysmodule clean
	$(MAKE) -C applet clean

dist:
	rm -rf dist

	mkdir -p dist/atmosphere/contents/$(SYSMODULE_TID)/flags
	cp sysmodule/sysmodule.nsp dist/atmosphere/contents/$(SYSMODULE_TID)/exefs.nsp
	touch dist/atmosphere/contents/$(SYSMODULE_TID)/flags/boot2.flag

	# controller applet replacement
	#mkdir -p dist/atmosphere/contents/$(CONTROLLER_TID)/flags
	#cp applet/applet.nsp dist/atmosphere/contents/$(CONTROLLER_TID)/exefs.nsp
	#cp -R applet/romfs dist/atmosphere/contents/$(CONTROLLER_TID)/romfs

	mkdir -p dist/switch
	cp applet/applet.nro dist/switch/$(PROJECT_NAME).nro
