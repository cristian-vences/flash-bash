# Flash-BASH
Flash-BASH is a Raspberry Pi based tool that uses a multiplexer to "glitch" a target device into a privileged open shell.

## Usage

See our blog post at https://www.riverloopsecurity.com/blog/2021/09/introducing-flash-bash/ for information on usage.

## Install/Compile
The easiest way to use this tool without any tweaks is with a Raspberry Pi Type 3 (Model A+, B+, Pi Zero, Pi Zero W, Pi2B, Pi3B, Pi4B).
However, with minor tweaks to the code, any Raspberry Pi, Arduino, or micro-controller should work fine.

Secondly, this tool is based on the dependency pigpio which should be automatically installed on latest version of Raspberry Pi OS but can be installed with:
```
sudo apt-get install pigpio python-pigpio python3-pigpio
```
Please visit https://abyz.me.uk/rpi/pigpio for more information on the library.

Third, you will need the Pi Hat and components that are included in the CAD folder in this repository. You could also wire this with jumpers and a breadboard or a prototype-hat.

Last, you will need to compile this pointing to the Wiring Pi library like this
```
gcc -Wall -pthread -o flash_bash flash_bash.c -lpigpio -lrt
```
or you can just run make in this directory.

That should get you started all you have to do now is run the program!
```
sudo ./flash_bash
```
