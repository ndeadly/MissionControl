<p align="left">
<img alt="GitHub" src="https://img.shields.io/github/license/ndeadly/MissionControl">
<img alt="GitHub release (latest by date)" src="https://img.shields.io/github/v/release/ndeadly/MissionControl">
<img alt="GitHub All Releases" src="https://img.shields.io/github/downloads/ndeadly/MissionControl/total">
<img alt="GitHub Releases" src="https://img.shields.io/github/downloads/ndeadly/MissionControl/latest/total">
<br>
<img alt="Donate Bitcoin" src="https://img.shields.io/static/v1?label=donate&message=bc1q4rh4vmqaujmewrswes303nms5mj3p80j7lqat0&color=yellow&style=flat&logo=bitcoin">
<img alt="Donate Ethereum" src="https://img.shields.io/static/v1?label=donate&message=fd28c8680416d5c706ad8e404955e0a3a2aa7124&color=yellow&style=flat&logo=ethereum">
</p>

<p align="left">

</p>

# Mission Control

Use controllers from other consoles natively on your Nintendo Switch via Bluetooth. No dongles or other external hardware neccessary.

### Features

* Supports all Switch firmware versions.
* Connect up to 8 non-switch Bluetooth controllers simultaneously without any additional hardware.
* Make use of native HOS menus for controller pairing, button remapping (firmware 10.0.0+) etc.
* Rumble support*
* Low input lag.
* File-based virtual controller memory allowing for data such as analog stick calibration to be stored and retrieved.
* Spoofing of host Bluetooth adapter name and address.
* `mc.mitm` module adds extension IPC commands that can be used to interact with the `bluetooth` process without interfering with the state of the system.

**Rumble not currently implemented for all compatible controllers*

### Supported Controllers

 Currently, the following controllers are supported. If you have a third-party variant of one of the below, or a Bluetooth controller that isn't listed, consider submitting an issue with the controller details, including vid/pid, to request support.

* __Nintendo Wii Remote + extensions (Nunchuck, Classic Controller, Classic Controller Pro, SNES Mini, TaTaCon (Taiko drum))__
* __Nintendo WiiU Pro Controller__
* __Sony DualShock4 Controller__
* __Sony Dualsense Controller__
* __Microsoft Xbox One S/X Controller (not to be confused with Series S/X controllers - these use Bluetooth LE, which isn't currently supported)__*
* __Microsoft Xbox Elite Wireless Controller Series 2__
* __NVidia Shield Controller (2017 Model)__
* __Ouya Controller__
* __Gamestick Controller__
* __Gembox Controller__
* __iCade Controller__
* __Ipega Controllers (9017s, 9023, 9055, 9062S, 9076, 9078, 9087 9096 confirmed working. Others may also work)__
* __Xiaomi Mi Controller__
* __Steelseries Free__
* __Steelseries Nimbus__
* __Steelseries Stratus Duo__
* __GameSir G3s__
* __GameSir G4s__
* __GameSir T1s__
* __GameSir T2a__
* __Hori Onyx__
* __8bitDo SN30 Pro Xbox Cloud Gaming Edition__
* __8BitDo ZERO (Most other 8BitDo controllers have a Switch mode available. May require firmware update)__
* __PowerA Moga Hero__
* __PowerA Moga Pro__
* __PowerA Moga Pro 2__
* __Mad-Catz C.T.R.L.R__
* __Razer Raiju Tournament__
* __Razer Serval__
* __Mocute 050__
* __Gen Game S3__
* __AtGames Legends Pinball Controller__
* __Hyperkin Scout__

**Not all Xbox One wireless controllers support Bluetooth. Older variants use a proprietary 2.4Ghz protocol and cannot be used with the Switch. See [here](https://support.xbox.com/help/hardware-network/accessories/connect-and-troubleshoot-xbox-one-bluetooth-issues) for information on identifying the Bluetooth variant.*

### Requirements

* Hackable Nintendo Switch running an up-to-date version of [Atmosphère](https://github.com/Atmosphere-NX/Atmosphere/releases) custom firmware. Other custom firmwares are ***not supported***.
* Compatible Bluetooth controller

### Installation

Download the [latest release](https://github.com/ndeadly/MissionControl/releases) .zip and extract to the root of your SD card, allowing the folders to merge and overwriting any existing files. A console reboot is required in order for Mission Control to become active.

***IMPORTANT: 
Atmosphère >= 1.2.5 is required to run the latest release of Mission Control. Using an older Atmosphère version will cause Mission Control to crash or freeze the system on boot.
Users upgrading from version 0.1.0 of Mission Control are also advised to wipe their pairing database and re-pair controllers running the latest version. Failure to wipe the old entries may result in non-switch controllers being detected incorrectly.***

### Usage

Install Mission Control to your SD card and reboot the console.

Mission Control runs as a background process and makes use of the system's native UI to handle controllers. The controller pairing dialog can be accessed from the home screen via  `Controllers->Change Grip/Order`. While sitting on this screen, place your controller in pairing mode (see below) and wait for it to connect to the console. Once paired, controllers will reconnect automatically when woken up. There is no need to re-pair them every time.

Controllers that successfully pair but haven't been supported yet will display with red buttons on the `Controllers` menu to indicate their controls are not being mapped. Please open an issue to request support for such controllers.

Your controller's buttons are mapped as closely as possible to the physical layout of a Switch Pro controller. This means that A/B and X/Y buttons will be swapped for controllers that use an Xbox style button layout rather than using what is printed on the button. The button combos `MINUS + DPAD_UP` and `MINUS + DPAD_DOWN` are provided for all controllers to function as an alternative for `CAPTURE` and `HOME` buttons in cases where there are not enough face buttons available.

Most native features *should* just work (with the exception of things like firmware update). If you find something that's broken please open a support issue on this github page.

### Pairing controllers
The supported controllers each have their own methods to enter pairing/sync mode. Below are instructions on entering this mode for each supported type.

***Nintendo Wii(U) Controllers***
Press the red sync button on the back of the controller. The controller LEDs will blink rapidly.

***Sony Dualshock4/Dualsense Controllers***
Press and hold the `PS` + `share` buttons simultaneously until the lightbar starts blinking. When done correctly the blink pattern will resemble a heartbeat, otherwise it will blink periodically.

If you have difficulty getting the controller to pair to the console, press the reset button on the back of the controller and keep trying. It should eventually connect.

***Microsoft Xbox One Controllers***
Press and hold the `guide`(`X`) button until the LED starts blinking. Then press and hold the small sync button on the back near the charging port until the LED starts blinking more rapidly.
You should also ensure your controller firmware is up to date, as old firmwares were known to have flakey Bluetooth.

***Other Controllers***
Please refer to your controller's user manual for information on how to put it in sync mode. Note that many generic Bluetooth controllers can be started in one of several modes. Usually you want to choose something like HID, PC or Android mode for it to work correctly.

### Module configuration

A template for the config .ini file will be installed to `/config/MissionControl/missioncontrol.ini.template`. To modify the default module settings, copy the template to `/config/MissionControl/missioncontrol.ini` and uncomment/modify any settings you want to change. The file is only parsed on startup, so any changes you make will require a reboot to take effect. Currently there is only a small set of configuration settings, but this will be expanded upon in future releases.

- `[general]`
These are general settings for mission control features. 
	- `enable_rumble` Enables/disables rumble support for unofficial controllers.
	- `enable_motion` Enables/disables motion controls support. Not currently used
	- `left_stick_deadzone` Sets an inner deadzone for the left stick in percent (100 means that the stick will be deactivated)
	- `left_stick_outer_deadzone` Sets an outer deadzone for the left stick in percent (value must not exceed 100 minus `left_stick_deadzone`)
	- `right_stick_deadzone` Sets an inner deadzone for the right stick in percent (100 means that the stick will be deactivated)
	- `right_stick_outer_deadzone` Sets an outer deadzone for the right stick in percent (value must not exceed 100 minus `right_stick_deadzone`)

- `[bluetooth]`
These settings can be used to spoof your switch bluetooth to appear as another device. This may be useful (in conjunction with a link key) if you want to use your controller across multiple devices without having to re-pair every time you switch. Note that changing these settings will invalidate your console information stored in any previously paired controllers and will require re-pairing.
	- `host_name` Override the bluetooth host adapter name
	- `host_address` Override the bluetooth host adapter address

### Removal

To functionally uninstall Mission Control and its components, all that needs to be done is to delete the following directories from your SD card and reboot your console.

* `/atmosphere/contents/010000000000bd00`
* `/atmosphere/exefs_patches/bluetooth_patches`

If you wish to completely remove all traces of the software ever having been installed (telemetry excepted), you may also want to follow these additional steps

* Remove the following directory from your SD card
    * `/config/MissionControl`

* Wipe the Bluetooth pairing database via `System Settings->Controllers and Sensors->Disconnect Controllers`

### Planned/In-progress Features
Below is a list of features I am currently working on or would like to look into in the future, roughly in descending order of priority. Requests are welcome if you have an idea you think would make a useful feature.

* ~~Rumble support~~
* Motion controls support
* Bluetooth LE support
* Per-controller configurations
    * Rumble on/off
    * Motion controls on/off
    * Identify as Pro Controller or Joycon
    * Set controller body/button colours
    * Invert analog stick axes
    * Analog stick deadzones
* Companion application
    * Pairing database management
        * View paired controller information
        * Clear database
        * Export database to file
        * Import existing database from file
    * Controller management/diagnostics
        * Manage controller configurations
        * View raw HID report data
        * Test buttons/analog sticks
        * Dump SPI flash (official controllers only)
    * Controller discovery/pairing reimplementation
* Tesla overlay
* Keyboard and mouse support
* Gamecube mode (analog trigger support)
* USB wired controllers

### Known Issues and Limitations

* Motion controls currently unsupported.
* Non-Switch controllers cannot be used to wake the system from sleep.
* Controllers using the Bluetooth LE (BLE) standard are currently not supported and will not connect to the system.
* Xbox One, Wii/WiiU and (especially) some Dualshock v1 controllers can take some time to be detected and subsequently pair with the Console. Be patient and re-enter the sync mode of the controller several times if neccessary. Once synced, controllers should work as usual.
* Xbox One controllers cannot be shut off and will attempt to reconnect to the console after being disconnected. This is a limitation of the controller's bluetooth firmware.
* ~~Xbox One button layout was changed at some point in a firmware update. Please ensure your controller firmware is up to date if you have issues with incorrect button mappings.~~ Both layouts are now supported.
* Reported controller battery levels may not be correct. I'm relying entirely on reverse engineering notes found on the internet for these. In many cases I don't own the controller and there is simply no information available, so these controllers will always show full battery. Any help in improving this is most welcome.
* Several users have reported knockoff WiiU and Dualshock4 controllers that cannot be detected by the console during Bluetooth discovery. Unfortunately I think they're using an incompatible Bluetooth chipset.

### Frequently Asked Questions

***Does this run on \<insert CFW here\>?***
No. Mission Control will only run under Atmosphère. This is not some attempt to lock out users of other CFW, Atmosphere is simply the only one (currently) providing the extensions neccessary to MITM Bluetooth communications that make this sysmodule possible.

***Will using this get me banned from online?***
Currently there haven't been any confirmed cases of bans as a result of running Mission Control. That said, running any unofficial software under CFW will always carry a non-zero risk of ban, and Nintendo could change their ban criteria at any point. While this should be relatively safe, it would certainly be possible to detect that you had connected foreign controllers to the console if they were interested in doing so. Use at your own discretion.

***Does this support USB controllers?***
No, Mission Control is currently Bluetooth-only. For now you can use cathery's [sys-con](https://github.com/cathery/sys-con) for USB controllers.

***Does this work with sys-con installed?***
Yes, the two can be run simultaneously without issue.

***My console is crashing on boot because of this sysmodule. What can I do?***
If you're seeing crashes on boot with Mission Control's title ID (`010000000000bd00`), it is likely either you have updated recently to a new Atmosphere release and an update to Mission Control is required, or you are running an old version of Atmosphere which is not compatible with the latest version of Mission Control.
Crashes in `sm` (title ID `0100000000000004`) can indicate version incompatibilities between your current Atmosphere and one or more of your homebrew sysmodules. This may be caused by Mission Control, or it could be another custom sysmodule that uses libstratosphere, even if it looks as though Mission Control is to blame (`ldn_mitm` and `emuiibo` are common offenders). If you've just updated Atmosphere you should always check if there have also been updates released for the sysmodules you use.
qlaunch errors (title ID `0100000000001000`) can be a sign that you have too many custom sysmodules running and are depleting the limited system resources available to them. Check your `/atmosphere/contents` folder and verify you actually need everything in there. If you don't know what you're doing, it may be easier to just delete this folder entirely, reinstall atmosphere, and then explicitly reinstall only the sysmodules you need.

***How can I use this with multiple sysNAND/emuMMC launch configs?***
Pairing controllers across multiple unique HOS installations requires multiple pairing databases and is essentially the same as pairing with two different consoles. The only exception being the case where you paired the controller(s) prior to making sysNAND copies. For now you will have to re-pair your controllers when switching back and forth. In the future I hope to include an option to load/store the database on the SD card to avoid this issue.

***My Xbox One Controller buttons are mapped incorrectly. Can you fix it?***
~~You didn't read the [Known Issues and Limitations](https://github.com/ndeadly/MissionControl#known-issues-and-limitations) section, did you? You need to update your controller firmware to the latest version. This can be done on Windows using the [Xbox Accessories](https://www.microsoft.com/en-us/p/xbox-accessories/9nblggh30xj3?activetab=pivot:overviewtab) app. You can also do this on the Xbox itself.~~ Both report formats are now supported. If you're still having issues with button mapping you're using an outdated version of Mission Control.

***Can you add support for PS3 controllers?***
~~It's on my list of things to look into. The pairing process is non-standard and may require modifications to the Bluetooth driver. If it can be done non-destructively I will add support eventually.~~ Having looked into this a bit, it appears it's going to be a lot of work given the constraints of HOS (if it can even be done without breaking support for other Bluetooth controllers). I won't rule out a solution in the future, but this is not high priority for me right now. Sorry DS3 owners.

***Can you add support for Xbox 360 controllers?***
No. These don't use Bluetooth. Try sys-con with a wireless USB adapter.

***Can you add support for wake from sleep?***
Probably not. As far as I know, wake from sleep involves a controller sending a special control command to the Switch Bluetooth hardware. There is no way to make a non-switch controller send the command recognised by the Switch without modifying its hardware/firmware.

***Can you add bluetooth audio support?***
~~No. The bluetooth module on the switch only implements a small set of services required to make hid controllers work. Of this small set of services, only a handful of high-level functions are exposed to the rest of the system. Adding audio support would require implementing the services neccessary for audio transport, for which any sane person would require an open-source re-implementation of the bluetooth module in order to have access the low-level functions required to pull it off.~~ As of firmware 12.0.0 Nintendo have added functions supporting Bluetooth audio. While the feature hasn't been enabled in official software, plutooo has created an experimental sysmodule called [nx-btred](https://github.com/plutooo/nx-btred) that enables Bluetooth audio in games that support recording. There isn't much benefit to me adding its functionality to Mission Control.

### How it works

Mission Control works by Man-In-The-Middling the `bluetooth` system module and intercepting its initialisation IPC commands and system events, and translating incoming/outgoing data to convince the Switch that it's communicating with an official Pro Controller.

To achieve this, the `btdrv.mitm` module obtains the handles to `bluetooth` system events and shared memory when the system attempts to initialise them over IPC via the `btm` and `hid` modules. It then creates its own secondary versions of these and passes their handles on instead of the original. This allows modifications to be made to any data buffers before notifying (or not) the system. Additionally, the `WriteHidData` IPC command is intercepted to translate or drop outgoing requests to the controller. In the case of the latter, fake responses can be written directly to the buffer in shared memory.

Intercepting initialisation IPC commands also allows homebrew to properly make use of the `bluetooth` service. Normally, calling any of the IPC commands that would initialise or finalise system events would either crash the console, or invalidate the event handles held by system processes. With `btdrv.mitm` we are able to hand out alternative event handles when homebrew attempts to initialise an interface, and redirect the real system events to those instead of the events held by system processes.

IPS patches to the `bluetooth` module are provided to (re)enable the passing of abitrary pincodes when Bluetooth legacy pairing is used (Nintendo hardcodes a value of `'0000'`, ignoring IPC arguments). This enables Wii(U) devices to be paired with the console.

The `btm` service is now also MITM'd, allowing for faking controller names on the fly while retaining the original names in the pairing database.

### Building from source

First, clone the repository to your local machine and switch to the newly cloned directory
```
git clone --recurse-submodules https://github.com/ndeadly/MissionControl.git
cd MissionControl
```

~~Mission Control currently uses a custom fork of `libnx` that adds Bluetooth service wrappers and type definitions.~~ Official libnx master is now used to build Mission Control. At the time of writing, the libnx distributed by devkitPro can be used without the need to build it yourself. This may change if `Atmosphere-libs` updates to use bleeding edge `libnx` commits not present in the official release. In any case, you can build the included `libnx` submodule from source with the following commands.

```
cd lib/libnx
make && make install
```

Next build `Atmosphere-libs`
```
cd ../Atmosphere-libs
make
```

Finally, build and package the distribution .zip. This will build the `mc.mitm` sysmodule and package it up with bluetooth exefs patches. 
```
cd ../..
make dist
```

The resulting package can be installed as described above.

### Credits

* [__switchbrew__](https://switchbrew.org/wiki/Main_Page) for the extensive documention of the Switch OS.
* [__devkitPro__](https://devkitpro.org/) for the homebrew compiler toolchain.
* __SciresM__ for his dedicated work on the [Atmosphère](https://github.com/Atmosphere-NX) project, libstratosphere and general helpfulness with all things Switch related.
* __misson20000__ for his handy debug monitor [Twili](https://github.com/misson20000/twili) and IPC logger [Ilia](https://github.com/misson20000/ilia)
* __dekuNukem__, __CTCaer__, __shinyquagsire23__ and others for their work in reversing and documenting the switch controller communication protocol.
* __friedkeenan__ for helping to test Wii extension controller support.
* __DatenThielt__ for helping debug the bluetooth service remotely with his Xbox Elite Series 2 controller in order for me to develop patches to enable it to be successfully paired with the console.
* Everyone else over at the __ReSwitched__ discord server who helped answering technical questions.

### Support

If you like this project, please consider supporting me to continue its development :)


<a href="https://ko-fi.com/J3J01BZZ6">
    <img border="0" alt="ko-fi" src="https://www.ko-fi.com/img/githubbutton_sm.svg" align="left">
</a>
