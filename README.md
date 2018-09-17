# mtca_mmc_firmware
Module Management Controller Firmware (MicroTCA Form Factor)

## Tools Installation

Following tools need to be installed in your system in order to compile the firmware:

- **LPCXpresso**
- **32-bit libraries**

Free version can be downloaded from NXP:
https://www.nxp.com/products/processors-and-microcontrollers/arm-based-processors-and-mcus/lpc-cortex-m-mcus/lpc1100-cortex-m0-plus-m0/lpcxpresso-ide-v8.2.2:LPCXPRESSO?tab=Design_Tools_Tab#

In order to use flashing features LPCXpresso needs to be activated.

LPCXpresso also needs addidional 32-bit packages. For Ubuntu 13.10 or later (including Debian) run:

    sudo apt-get install libgtk2.0-0:i386 libxtst6:i386 libpangox-1.0-0:i386 \
         libpangoxft-1.0-0:i386 libidn11:i386 libglu1-mesa:i386 \
         libncurses5:i386 libudev1:i386 libusb-1.0:i386 libusb-0.1:i386 \
    	 gtk2-engines-murrine:i386 libnss3-1d:i386

More information can be found in the INSTALL.txt file that is included in the LPCXpresso installer package.


Add folder paths of the LPCXpresso binaries to the PATH variable:

    export PATH=$PATH:/usr/local/lpcxpresso_8.2.2_650/lpcxpresso/bin:/usr/local/lpcxpresso_8.2.2_650/lpcxpresso/tools/bin



# Clone MMC firmware sources


Next step is to clone this repository into your workspace.

	git clone https://github.com/GSI-CS-CO/mtca_mmc_firmware.git

Move to the downloaded folder

    cd mtca_mmc_firmware


# Compilation

In the cloned folder there is Makefile used for:
- **building firmware**
- **initializing LPC-Link debugger**
- **testing LPC-Link debugger**
- **flashing firmware to MMC**


To compile sources just run:

    make mmc


To clean the compilation files run:

    make mmc-clean


# Debugger initialization and test

To load firmware to MMC use LPC-Link debugger. Before downloading firmware to MMC debugger needs to be initialized.
First check that debugger is visible in the system:

    $ lsusb | grep LPC-Link
    Bus 001 Device 051: ID 0471:df55 Philips (or NXP) LPCXpresso LPC-Link

To initialize debugger just run:

    $ make lpclink-init
    ...
    Starting download: [##################################################] finished!
    ...
    Done!
    ...
    Power cycle the target then run 'make lpclink-test'


Power cycle the target then check that new device (NXP Semiconductors, Code Red Technologies LPC-Link Probe v1.3 [0100]) is present on the USB bus:

    $ lsusb | grep NXP
    Bus 001 Device 052: ID 1fc9:0009 NXP Semiconductors


To test LPC-Link run:

    $ make lpclink-test
    crt_emu_a7_nxp -info-emu -wire=winusb
    Ni: LPCXpresso Debug Driver v8.2 (Aug 31 2016 11:00:14 - crt_emu_a7_nxp build 360)
    1 Emulators available:
    0. WIN64HS12	LPC-Link Probe v1.3 (NXP - LPC-Link)
    ...
    1 devices:
    !0. IN_USE	Device is connected already (NXP - LPC-Link)


# Flashing firmware to MMC with LPC-Link

To load new firmware to MMC run:

    $ make flash
    ...
    Nt: Writing 99436 bytes to address 0x00000000 in Flash
    Pb: 1 of 2 (  0) Writing pages 0-8 at 0x00000000 with 65536 bytes
    Ps: (  0) Page  0 at 00000000
    Ps: (  6) Page  0 at 00000000: 4096 bytes
    ...
    Nt: Verified-same page 0-8 with 65536 bytes in 25850msec
    Pb: 2 of 2 ( 65) Writing pages 9-10 at 0x00010000 with 33900 bytes
    Ps: (  0) Page  9 at 00010000
    Ps: ( 12) Page  9 at 00010000: 4096 bytes
    ...
    Nt: Erased/Wrote page  9-10 with 33900 bytes in 24207msec
    Pb: (100) Finished writing Flash successfully.
    Nt: Flash Write Done
    Nt: Loaded 0x1846C bytes in 50943ms (about 1kB/s)
    ...

NOTE: You might get an error after this part of flash-utility output, but this does not seem to have any influence on the actuall flashing procedure.


# Flashing firmware to MMC with OpenOCD (Olimex OpenOCD JTAG)

To load new firmware to MMC with OpenOCD:

    $ make flash-mmc-openocd

    ...auto erase enabled
    auto unlock enabled
    wrote 131072 bytes from file ./LPC2136_FreeRTOS_CoreIPM/Debug/LPC2136_FreeRTOS_CoreIPM.bin in 19.077818s (6.709 KiB/s)
    ...


NOTE: This is still experimental. Flashing ends with error but MMC is flashed and works.
When ARM-USB-OCD is connected to PC for the first time then repeat command "make flash-mmc-openocd" twice.


Test was done using Olimex ARM-USB-OCD and Olimex ARM-JTAG-20-10 adapter

https://www.olimex.com/Products/ARM/JTAG/ARM-USB-OCD/

https://www.olimex.com/Products/ARM/JTAG/ARM-JTAG-20-10/


# MMC console

To observe MMC activity there is console available. Connect to FTRN via USB cable to the MMC USB connector (the one on the opposite side of the SFP and HotSwap handle).
Device is visible on the USB bus as

    Bus xxx Device yyy: ID 0403:6015 Future Technology Devices International, Ltd Bridge(I2C/SPI/UART/FIFO)

and in /dev as ttyUSBx

Open device in terminal, example with minicom:

    minicom -b 115200 -D ttyUSB0


Console outputs various information about MMC state and actions and currently supports this inputs:

- h     : list commands
- 0-9   : enables/disables debug prints, 0-debug prints disabled, 1-9 - debug print level enabled
- i, I  : print out firmware build info (similar as eb-info)
- p     : disable PCIe port (asserts PCIe reset)
- P     : enable PCIe port (if FTRN used outside crate, without MCH, on AMC>PCIe adapter)
- l     : enable MMC LED test, turn all MMC LEDs OFF
- L     : enable MMC LED test, turn all MMC LEDs ON
- o     : disable MMC LED test (use this after done with L or l)
- s     : display Freertos tasks and task statistics

# OpenOCD

We recommend to use OpenOCD version 0.9.0.

## OpenOCD and Olimex OpenOCD JTAG ARM-USB-OCD-H

In case you face problems like this: "Error: The specified debug interface was not found (ft2232)", you should try to build OpenOCD by yourself:

  1. Check out/get version 0.9.0
  2. $ ./configure --enable-ftdi
  3. $ make
  4. $ (sudo) make install
  5. (Optional, copy udev rules file) $ (sudo) cp ./contrib/99-openocd.rules /etc/udev/rules.d/
