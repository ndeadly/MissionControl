<p align="left">
<img alt="GitHub" src="https://img.shields.io/github/license/ndeadly/MissionControl">
<img alt="GitHub All Releases" src="https://img.shields.io/github/downloads/ndeadly/MissionControl/total">
</p>

# MissionControl [![ko-fi](https://www.ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/J3J01BZZ6)

Use controllers from other consoles natively on your Nintendo Switch via Bluetooth. No dongles or other external hardware neccessary.

### Features
* Supports all firmware versions
* Pair Bluetooth controllers natively via `Change Grip/Order` screen
* Native button remapping (firmware 10.0.0+)
* `btdrv-mitm` module also allows for interacting with the `bluetooth` process without interfering with/interference from the system. 

### Supported Controllers
 Currently, the following controllers are supported. If you have a third-party variant of one of the below, or a Bluetooth controller that isn't listed, consider submitting an issue with the controller details, including vid/pid, to request support.

* __Nintendo Wii Remote + extensions (Nunchuck, Classic Controller, Classic Controller Pro)__*
* __Nintendo WiiU Pro Controller__
* __Sony DualShock4 Controller__
* __Microsoft Xbox One S Controller__**

**Extension Controllers remain untested since I don't actually own any. Please report any issues. Motion Plus not currently supported.*
***Not all Xbox One wireless controllers support Bluetooth. Older variants use a proprietary 2.4Ghz protocol and cannot be used with the Switch. See [here](https://support.xbox.com/help/hardware-network/accessories/connect-and-troubleshoot-xbox-one-bluetooth-issues) for information on identifying the Bluetooth variant.*

### Requirements
* Hackable Nintendo Switch running the latest [Atmosphère](https://github.com/Atmosphere-NX/Atmosphere/releases) CFW
* Compatible Bluetooth controller

### Installation
Download the latest release .zip and extract to the root of your SD card. A console reboot is required in order for MissionControl to become active.

*__Note: Currently a modified boot2 is required to launch btdrv-mitm early enough to intercept Bluetooth initialisation. This will get overwritten any time Atmosphère is updated on SD, and will need to be replaced.__*

### Building from source
First, clone the repository to your local machine
```
git clone --recurse-submodules https://github.com/ndeadly/MissionControl.git
```

MissionControl uses a custom fork of `libnx` that adds Bluetooth service wrappers and type definitions. This needs to be built and installed first

```
cd libnx
make && make install
```

Next build `Atmosphere-libs`
```
cd ../Atmosphere-libs
make
```

Finally, build and package the distribution .zip. This will build a custom `boot2`, `btdrv-mitm` and package them up with bluetooth exefs patches. 
```
cd ..
make dist
```

The resulting package can be installed as described in the above section.

### Usage
Install MissionControl to your SD card, reboot the console and then pair controllers as you normally would via the `Controllers->Change Grip/Order` screen. Once paired, controllers will reconnect automatically when woken up.

Most native features *should* just work (with the exception of things like firmware update). If you find something that's broken please create an issue.

### How it works
MissionControl works by Man-In-The-Middling the `bluetooth` system module and intercepting its initialisation IPC commands and system events, and translating their data to convince the Switch that it's communicating with a Pro Controller.

To achieve this, the `btdrv-mitm` module obtains the handles to `bluetooth` system events and shared memory when the system attempts to initialise them over IPC via the `btm` and `hid` modules. It then creates its own secondary versions of these and passes their handles on instead of the original. This allows modifications to be made to any data buffers before notifying (or not) the system. Additionally, the `WriteHidData` IPC command is intercepted to translate or drop outgoing requests to the controller. In the case of the latter, fake responses can be written directly to the buffer in shared memory.

Intercepting initialisation IPC commands also allows homebrew to properly make use of the `bluetooth` service. Normally, calling any of the IPC commands that would initialise or finalise system events would either crash the console, or invalidate the event handles held by system processes. With `btdrv-mitm` we are able to hand out alternative event handles when homebrew attempts to initialise an interface, and redirect the real system events to those instead of the events held by system processes.

IPS patches to the `bluetooth` module are provided to (re)enable the passing of abitrary pincodes when Bluetooth legacy pairing is used (Nintendo hardcodes a value of `'0000'`, ignoring IPC arguments). This enables Wii(U) devices to be paired with the console.

### Planned Features
* Controller management application
* Rumble support
* Motion support
* Keyboard and mouse support

### Known Issues and limitations
* Non-Switch controllers cannot be used to wake the system from sleep.
* Wii(U) controllers can take a while to pair with the console. For some reason they are only detected at the end of a device discovery cycle. Be patient and re-press the sync button on the controller if neccessary.
* Xbox One controllers may disconnect and refuse to reconnect after initial pairing. The controller will connect normally after console is restarted.
* Layout of Xbox One input report appears to have changed at some point. Button mapping may be incorrect on newer controller firmwares. I no longer have access to a controller to test.

### Credits
* [__switchbrew__](https://switchbrew.org/wiki/Main_Page) for the extensive documention of the Switch OS.
* [__devkitPro__](https://devkitpro.org/) for the homebrew compiler toolchain.
* __SciresM__ for his dedicated work on the [Atmosphère](https://github.com/Atmosphere-NX) project, libstratosphere and general helpfulness with all things Switch related.
* Everyone else over at the __ReSwitched__ discord server for answering development related questions and helping with testing.
* __misson20000__ for his handy debug monitor [Twili](https://github.com/misson20000/twili) and IPC logger [Ilia](https://github.com/misson20000/ilia)
* __dekuNukem__, __CTCaer__, __shinyquagsire23__ and others for their work in reversing and documenting the switch controller communication protocol.
* __COVID-19__ for giving me the break from social commitments required to finish this project.
