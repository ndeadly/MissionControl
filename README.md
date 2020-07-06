![GitHub All Releases](https://img.shields.io/github/downloads/ndeadly/MissionControl/total)
![GitHub](https://img.shields.io/github/license/ndeadly/MissionControl)

# MissionControl
Use Bluetooth controllers from other consoles on your Nintendo Switch.

### Features
* Supports all firmware versions
* Pair Bluetooth controllers natively via *`Change Grip/Order`* screen*
* Native button remapping (10.0.0+)

**Wii(U) controllers require console-specific patches to be generated before they can be paired.*

### Supported Controllers
 Currently, the following controllers are supported. If you have a third-party variant of one of the below, or a bluetooth controller that isn't listed, consider submitting an issue with the controller vid/pid to request support.

* __Sony DualShock4 Controller__
* __Microsoft Xbox One Controller__*
* __Nintendo Wii Remote__**
* __Nintendo WiiU Pro Controller__

**Only newer Xbox One controllers support Bluetooth. Older variants use a custom 2.4Ghz protocol and cannot be used wirelessly with the Switch. See [here](https://support.xbox.com/help/hardware-network/accessories/connect-and-troubleshoot-xbox-one-bluetooth-issues) for information on identifying the Bluetooth variant.*
** *Wiimote extension controllers eg. Nunchuck, Classic Controller etc. not supported yet*

### Requirements
* Hackable Nintendo Switch running [Atmosphère](https://github.com/Atmosphere-NX/Atmosphere/releases) CFW
* Compatible Bluetooth controller

### Installation
Grab the latest release .zip and extract to the root of your SD card.

### Building from source

### Planned features
* Wii extension controller support
* Controller configuration app
* Rumble support
* Overlay menu for quick access to controller configuration

### Known Issues
* Joy-Con controllers that have paired via the console rails store incorrect vid/pid information. This causes them to be unrecognised over Bluetooth. Clearing the pairing database via *`System Settings->Controllers and Sensors->Disconnect Controllers`* and re-pairing wirelessly in the *`Change Grip/Order`* screen should resolve the issue. 
*__(Note: Joy-Cons must be disconnected from the rails prior to attempting this or else they will instantly re-pair with the console)__*
* Xbox One controllers currently disconnect after pairing. Controller will connect normally after console is restarted.
* Controllers with player indicator LEDs always show as player 1.
* Battery level indicator always displays full battery.

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