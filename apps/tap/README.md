# TAP User-space Application<br><br>
## 1. Introduction
The user-space application serves as a medium to facilitate the exchange of ethernet frames between different cores on the SoC.<br> To achieve this, a TAP device is used to read from and write to, the Linux Network Stack.<br> Ethernet frames are copied from/to the shared memory region to allow other cores to access it.<br><br>
## 2. Installation
In the directory containing the following files:<br>
J7ic.conf    launch_tap.service    Makefile    tapfirmware.c    tapfirmware.h    tapif.c    tapif.sh<br>
run the following command to compile on the EVM and install the files on the EVM:<br>
```shell
    $ make install
```
To cross compile for the EVM run:<br>
```shell
    $ make CROSS_COMPILE=aarch64-none-linux-gnu-
```
Then, copy the following files:<br>
cleantapif.sh    J7ic.conf    launch_tap.service    tapif    tapif.sh    Makefile<br>
to the EVM and run:<br>
```shell
    $ make install
```
on the EVM to install the files in their installation directories.<br>
Next, run:<br>
```shell
    $ systemctl enable launch_tap.service
```
to ensure that the systemd service launch_tap.service starts up automatically<br>
on boot. With this, on the next boot, the user-space application should be running<br>
automatically in the background.<br><br>
## 3. Debugging
By default, the systemd service: launch_tap.service will run the shell script tapif.sh during boot up. <br>
However, it is possible to relaunch the application either for testing purposes or in <br>
case of errors during automatic startup. <br>
To manually launch the application, navigate to the directory containing the tapif.sh file and the tapif executable.<br>
Both tapif.sh and tapif should be present in the same directory as per the installation.<br> Then, run:<br>
```shell
    $ bash tapif.sh
```
to run the shell script which shall initialize the TAP device and launch the user-space application.
