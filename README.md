

<a href="https://scan.coverity.com/projects/rodan-sigdup">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/23935/badge.svg"/>
</a>

## sigdup

This signal duplicator project converts logic signals captured via sigrok or pulseview (.sr files), allows the user to select the channels of interest and finally reproduces the signals on the P3.0-P3.7 pins of the MSP-EXP430FR5994 development board.


 source:       https://github.com/rodan/sigdup
 author:       Petre Rodan <2b4eda@subdimension.ro>
 license:      GNU GPLv3

 zmodem and crc support based on
 source:       https://github.com/gavanfantom/hello-firmware
 author:       Gavan Fantom
 license:      MIT

### Components

a small linux program located in **./software** is used to convert the .sr file into a highly-optimized stream of signal transitions and timing information.

the output file is uploaded to the microcontroller via the zmodem protocol (minicom CTRL+A+S), the signal is verified and replayed either a P5.5 active low trigger or the **go** command available on the devboard's serial over USB console (57600baud 8n1)

the source of the microcontroller firmware is located in the **./firmware** directory.

### Build requirements

Tfirst for the PC application, you only need the standard gcc toolchain, libzip and a handful of tools that come with any linux distro. 

compilation is simple, one only needs to
```
cd ./software
make
```

the .sr file parsing algorithm was written from scratch, so libsigrok is not currently needed.


as for the firmware, you will need TI's excelent GCC toolchain [] or code composer studio for linux and my reference libraries for msp430 micros [] cloned in /opt/.

```
cd ./firmware
make
make install
```

