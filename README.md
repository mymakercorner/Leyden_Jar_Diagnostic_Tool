# The Leyden Jar Diagnostic Tool

![Leyden Jar Diagnostic Tool](images/Screeshot_F122_Device_List.png)

The Leyden Jar Diagnostic Tool is an application meant to perform troubleshooting and inspection on Leyden Jar controller firmwares.

## The Leyden Jar controller project

The [Leyden Jar](https://en.wikipedia.org/wiki/Leyden_jar) is a modernized controller PCB for the Brand New Model F and Beam Spring keyboards and is pin compatible with the good old XWhatsit controller we all know. 

This is an open source project, available on GitHub at the following link [here](https://github.com/mymakercorner/Leyden_Jar).

## Inspiration

Pandrew did a similar tool for the XWhatsit controller, his fundamental ideas have been applied to the Leyden Jar Diagnostic Tool with the addition of some new features and small improvements. 

## Benefits

Usage of this tool can benefit both firmware developers and end users.

### Firmware developers

Writing a firmware for a new Model F or Beam Spring keyboard is a rather involved process. This tool can help inspect various states of the keyboard including reading the analogic levels of each key.
This eases the process of tuning some firmware parameters. 
This can also help find software issues, and also can help find and understand hardware issues as well.

### End users

The program allow to easily make the controller go into bootloader mode (to flash a new firmware) or to erase the EEPROM that can contain VIA keys/layers configurations.  

It is not uncommon for keys to stop working after shipping or transportation or because of a bad keycap installation.  
If some keyboard keys don't work as intended the tool can help investigate the issue. 

## Features

* List all Leyden Jar compatible HID devices.
* EEPROM reset.
* Enter bootloader.
* Display various QMK/VIA/VIAL infos.
* Display various Leyden Jar related infos.
* Keypress monitor.
* Analog levels monitor.
* Different view types:
    * keyboard layout.
    * logical(QMK) matrix.
    * physical(controller) matrix.
* VIAL layouts management:
    * automatic detection.
    * in app selection/view of different VIAL layout configurations.

## License

This project is provided with the very permissive MIT license.

## Compatibility

For the moment the tool has been built and tested only on Windows platforms.  
But the project itself is cross-platform friendly and should be easily adapted to other OSes like Linux and MacOSX.

## Build

CMake utility is used for generating the build files, the CMakeLists.txt file checks that you have at least version 3.21 installed.

### Windows 

You will need to have Visual Studio 2022 installed on your computer, a community edition will work perfectly.  
Execute GenerateBuildForVS2022.bat script, the Visual Studio solution should then be created inside the build directory. 

### Linux(untested)

Execute GenerateBuildForUnix.sh to generate build files in the build directory.

### MacOSX(untested)

Execute the same GenerateBuildForUnix.sh shell script to generate build files in the build directory.

## Acknowlegments

This project uses several other software packages as GIT sub modules.

### SDL

A famous cross-platform windowing API under the ZLIB license: https://github.com/libsdl-org/SDL  

### minlzma

A very good minimal LZMA decoding library under the MIT license: https://github.com/ionescu007/minlzma

### jsoncpp

A widely used json parsing library provided under the MIT license: https://github.com/open-source-parsers/jsoncpp

### hidapi

A raw HID library, used under the original HIDAPI license: https://github.com/libusb/hidapi

### Dear ImGui

A stunning and easy to use immediate graphical user interface library provided under the MIT license: https://github.com/ocornut/imgui
