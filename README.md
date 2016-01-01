# wwsr

A fork of the fowsr or wwsr program for reading the WS1080 weather station

I got the original program from the fowsr guys. I didn't like the fact that it was written in one huge C file, 
so i went about the process of spreading it out and re-doing it so that it made more sense to me. There are still things 
that i think could be done better, even when i did them i still think that. So it's a bit of a work in progress. 

Jim Easterbrook also has some excellent information about the storage of the device at http://www.jim-easterbrook.me.uk/weather/ew/

## Requirements
The program requires the a WS1080 or WS1081 weather station and the additional packages
* libusb-dev
* libpq-dev
* libcurl4-gnutls-dev 

do apt-get install libusb-dev libpq-dev libcurl4-gnutls-dev to get these

## Installing the program
Unpack the files to a directory of your choice, then run the make file.

## Usage
The device might get claimed by the kernel, add the following udev rule so that we can access it without being root. 
Add in /etc/udev/rules.d/<rule_name>

```bash

# WH-1081 Weather Station
SUBSYSTEM=="usb", ENV{DEVTYPE}=="usb_device", ATTRS{idVendor}=="1941", ATTRS{idProduct}=="8021", GROUP="plugdev", MODE="660"

LABEL="weather_station_end"

```

After doing that, you should be able to do ./wwsr -h and be presented with the help screen
