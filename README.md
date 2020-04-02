# MissionControl
Enable the use of Bluetooth controllers from other consoles on your Nintendo Switch.

## Controller Support
 Currently, the following controllers are supported. If you have unsupported controller or a third-party variant of one of the below, consider submitting an issue with the controller vid/pid to request support.

* __Sony DualShock4 Controller__
* __Microsoft Xbox One Controller__*
* __Nintendo Wii Remote__**

**Only newer controllers from Xbox One S and X consoles support Bluetooth*
** *Wiimote extension controllers eg. Nunchuck, Classic Controller etc. not supported yet*

MissionControl can also be used to pair Switch Pro controllers. This is of no real benefit at the moment (you will lose rumble support), but may be useful when button rebinding is added in the future.

## Requirements
* Nintendo Switch running [Atmosphère](https://github.com/Atmosphere-NX/Atmosphere/releases) CFW
* Compatible Bluetooth controller

Controller pairing should work on any firmware version, but you will need firmware 5.0.0+ to actually use the controller with the console.

## Installation
Grab the latest release zip and extract to the root of your SD card.

## Known Issues
* Console crashes on sleep on firmware prior to 9.x.x. The btm module doesn't seem to like us talking to other controllers behind its back
* Console is slow to wake from sleep on 9.x.x. I have no idea why
* No Rumble support. This appears to be a limitation of the virtual controller interface

## Planned features
* Button rebinding and setting custom analog stick deadzones
* Wii extension controller support (includes WiiU Pro Controller)
* Setting controller LEDs
* Overlay menu for quick switching of controller profiles

## Support
If you like this project, please consider supporting its development.

[![ko-fi](https://www.ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/J3J01BZZ6)

## Credits
* __switchbrew__
* __devkitPro__
* __ReSwitched__
* __misson20000__ for his handy debugging tools [Twili](https://github.com/misson20000/twili) and [Ilia](https://github.com/misson20000/ilia)
* __COVID-19__ for giving me the break from social commitments required to finish this project.