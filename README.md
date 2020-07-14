![GitHub All Releases](https://img.shields.io/github/downloads/ndeadly/MissionControl/total)
![GitHub](https://img.shields.io/github/license/ndeadly/MissionControl)

# MissionControl
Use Bluetooth controllers from other consoles natively on your Nintendo Switch.

### Features
* Supports all firmware versions
* Pair Bluetooth controllers natively via *`Change Grip/Order`* screen
* Native button remapping (10.0.0+)

### Supported Controllers
 Currently, the following controllers are supported. If you have a third-party variant of one of the below, or a bluetooth controller that isn't listed, consider submitting an issue with the controller vid/pid to request support.

* __Sony DualShock4 Controller__
* __Microsoft Xbox One Controller__*
* __Nintendo Wii Remote__**
* __Nintendo WiiU Pro Controller__

**Not all Xbox One wireless controllers support Bluetooth. Older variants use a custom 2.4Ghz protocol and cannot be used with the Switch. See [here](https://support.xbox.com/help/hardware-network/accessories/connect-and-troubleshoot-xbox-one-bluetooth-issues) for information on identifying the Bluetooth variant.*
** *Wiimote extension controllers eg. Nunchuck, Classic Controller etc. not supported yet*

### Requirements
* Hackable Nintendo Switch running [Atmosphère](https://github.com/Atmosphere-NX/Atmosphere/releases) CFW
* Compatible Bluetooth controller

### Installation
Grab the latest release .zip and extract to the root of your SD card.

### Building from source

### Known Issues
* Xbox One controllers currently disconnect and refuse to reconnect after pairing. The controller will connect normally after console is restarted.
* Wii(U) controllers can take a while to pair with the console. For some reason they are only detected at the end of a device descovery cycle. Be patient and re-press the sync button on the controller if neccessary.
* Controllers with player indicator LEDs always show as player 1.

### Support
If you like this project, please consider supporting its development.

[![ko-fi](https://www.ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/J3J01BZZ6)

### Credits
* __switchbrew__
* __devkitPro__
* __ReSwitched__
* __misson20000__ for his handy debug monitor [Twili](https://github.com/misson20000/twili) and IPC logger [Ilia](https://github.com/misson20000/ilia)
* __dekuNukem__, __CTCaer__, __shinyquagsire23__ and others for their work in reversing and documenting the switch controller communication protocol.
* __COVID-19__ for giving me the break from social commitments required to finish this project.