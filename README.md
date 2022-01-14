

<a href="https://scan.coverity.com/projects/rodan-sigdup">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/23935/badge.svg"/>
</a>

## sigdup

This project converts logic signals captured via [sigrok](https://www.sigrok.org/) or [pulseview](https://www.sigrok.org/wiki/PulseView) (.sr files), allows the user to select the channels of interest and finally reproduces the signals on the P3.0-P3.7 pins of an [MSP-EXP430FR5994 development board](https://www.ti.com/tool/MSP-EXP430FR5994). currently signals slower than 50kHz are supported.

```
 source:       https://github.com/rodan/sigdup
 author:       Petre Rodan <2b4eda@subdimension.ro>
 license:      GNU GPLv3

 zmodem and crc support based on
 source:       https://github.com/gavanfantom/hello-firmware
 author:       Gavan Fantom
 license:      MIT
```

### Components

a small linux program located in **./software** is used to convert the .sr file into a highly-optimized stream of signal transitions and timing information. the output of the application is a file that contains the following information:

 * a header with packet counts and sizes but most importantly a timer clock divider which will be assigned to a dedicated timer on the target microcontroller (MSP430FR5994)
 * a stream of 3 byte packets that contain the P3 output in 1 byte and a CCR timer register for the next packet in the remaining two.
 * packets in this stream are only generated for a signal transition on any of the channels of interest or a timer counter overflow.

a series of simulations are run on the input file with all supported clock dividers and the best divider wins based on a multi-criteria score system.

this output file is uploaded to the microcontroller's high FRAM area via the zmodem protocol (minicom CTRL+A+S), the signal is verified and replayed either with a P5.5 active low trigger or the **go** command available on the devboard's serial-over-USB console (57600baud 8n1)

the source of the microcontroller firmware is located in the **./firmware** directory.

this firmware is fully interrupt-driven to ensure the (otherwise sleeping) uC is always ready to generate precise output on P3.x.

it takes up to about 10us to wake up from the timer interrupt, get the signal packet from FRAM, set P3 and prepare the timer for the next transition. so this project is great for clocks up to 50kHz, a 57600 serial uart or any signal that does not have consecutive transitions faster then 10us. if at least one such hight frequency artefact is found in the input signal then a non-zero count of 'blk' (blackout) errors is reported during simulation and generation of output using that particular divider is not allowed.

### Build requirements

first for the PC application, you only need the standard gcc toolchain, libzip and a handful of tools that come with any linux distro. 

compilation is simple, one only needs to
```
cd ./software
make
```

the .sr file parsing algorithm was written from scratch, so libsigrok is not currently needed.


as for the firmware, you will need TI's excelent [GCC toolchain](https://www.ti.com/tool/MSP430-GCC-OPENSOURCE) or Code Composer Studio for linux and my [reference libraries for msp430 micros](https://github.com/rodan/atlas430) cloned in /opt/.

```
cd ./firmware
make
make install
```

the devboard has been modified by adding a 16MHz high frequency crystal and two load capacitors. if these components are missing from your board then make sure to disable the *USE_XT2* define from the Makefile or CCS.

more details in [./firmware/README](./firmware/README).

### Usage

manual pages are provided for both the [application](./doc/sigdup.1) and the [firmware](./doc/sigdup-firmware.1)

### Testing

most subsystems are separately tested either via [unit](./testsuite/zmodem/zmodem_unit_test.sh) [tests](./testsuite/crc) or [library per-module firmware](https://github.com/rodan/atlas430/tree/master/tests).

the code itself is static-scanned by [llvm's scan-build](https://clang-analyzer.llvm.org/), [cppcheck](http://cppcheck.net/) and [coverity](https://scan.coverity.com/projects/rodan-sigdup?tab=overview). Dynamic memory allocation in the PC applications is checked with [valgrind](https://valgrind.org/).

the physical signals generated are analyzed with an 8 channel Saleae and a 16 channel DreamSourceLab DSLogicPlus device.

