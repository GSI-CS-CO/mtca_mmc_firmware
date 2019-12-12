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


# OpenOCD

We recommend to use OpenOCD version 0.9.0.

## OpenOCD and Olimex OpenOCD JTAG ARM-USB-OCD-H

In case you face problems like this: "Error: The specified debug interface was not found (ft2232)", you should try to build OpenOCD by yourself:

  1. Check out/get version 0.9.0
  2. $ ./configure --enable-ftdi
  3. $ make
  4. $ (sudo) make install
  5. (Optional, copy udev rules file) $ (sudo) cp ./contrib/99-openocd.rules /etc/udev/rules.d/

# The obsession with the -Werror compiler flag

In case you can't compile OpenOCD 0.9.0 because the -Werror flag is set, just disable it:
  1. $ ./configure --enable-ftdi --disable-werror

# Modification of MMC behaviour for compatibility with Libera

Following REQ 3.54 from AMC specification, when MMC gets Management Power and FW initialization ends it checks the state of the Hot-Swap Handle, sends Hot-Swap (HS) event to MCH and arms periodic check of the HS handle.
This means that MMC acts as master on the I2C IPMI-B bus.
Because Libera BCM does not support hot-plug/hot-swap it always acts as master on the IPMI-B bus and does not expect to receive any event messages from AMC MMCs but only responses to BCM requests, like sensor readings.
Sending events to Libera BCM might also cause lockup of the BCM.
On the other hand MCH in MTCA crates sends SET_EVENT_RECEIVER request to MMC when AMC is inserted or HS handle is closed. Libera BCM does not send this request.
Therefore SET_EVENT_RECEIVER request is used as a way for MMC to know if it is in a MTCA crate.

FW modification is implemented that changes MMC behaviour in the following way:
- after MMC gets Management Power it DOES NOT check HS handle state and DOES NOT send HS event to MCH and DOES NOT arm periodical checking of the HS handle state.
  Main point here is that MMC waits with checking of the HS handle until it is sure in what type of crate it is.

- if in MTCA crate with managed PSU, MCH is notified by PSU that FTRN is inserted (hot pluged) and then MCH would start to communicate with MMC. MCH then sends SET_EVENT_RECEIVER request to MMC
  and from that MMC definetly knows that it is in the MTCA crate and not in Libera because Libera does not send this request. Then ONLY after receiving SET_EVENT_RECEIVER request
  MMC checks HS handle state, sends HS event and arms periodical checking of the HS handle.

  IMPORTANT: If HS handle is just opened and BLUE led turned ON and then HS handle is closed again without removing FTRN from the slot (not removing Management Power) FTRN might not turn on again!
  To remove FTRN from crate open HS handle, wait until BLUE led stops blinking and turns ON. Then you have to remove FTRN from crate!

- in Libera, because SET_EVENT_RECEIVER request is not sent by Libera BCM, HS handle is never checked, periodical checking is never enabled and no HS event messages are sent out.
  Opening or closing HS handle has no effect.

- if in MTCA crate with NON-managed PSU: this case was not tested yet but it is expected that FTRN payload would not turn on because MCH would not be aware of the FTRN presence in the crate.



To enable modification code can be build with enabled LIBERA_HS_EVENT_HACK switch that is defined (uncommendted) in:
 ./LPC2136_FreeRTOS_CoreIPM/src/project_defs.h

By default this hack is enabled.


It can be checked in two ways if FW was build with Libera:
1. connect to MMC console on the FTRN front panel (see next section) and press 'i' to print out build info.

-- Build info -----------------------
Project     : FTRN AMC MMC
Build date  : Mon Mar 18 16:09:13 CET 2019-LIBERA-MOD
...

If FW is built with Libera hack then "LIBERA-MOD" text is present at the end of the build date line.


2. Use ipmitool to read out board info:

ipmitool -H [MCH IP] -A none fru print [FTRN FRU ID]

response should include line:

MMC_FW_INFO_3  "Build Date: Mon Mar 18 16:09:13 CET 2019-LIBERA-MOD"

If FW is built with Libera hack then "LIBERA-MOD" text is present at the end of the build date line.



# MMC console

To observe MMC activity there is console available. Connect to FTRN via USB cable to the MMC USB connector (the one on the opposite side of the SFP and Hot-Swap handle).
Device is visible on the USB bus as

    Bus xxx Device yyy: ID 0403:6015 Future Technology Devices International, Ltd Bridge(I2C/SPI/UART/FIFO)

and in /dev as ttyUSBx

Open device in terminal, example with minicom:

    minicom -b 115200 -D /dev/ttyUSB0


Console outputs various information about MMC state and actions and currently supports this inputs:

- h     : list commands
- i, I  : print out firmware build info (similar as eb-info)

- R0-9  : enables/disables debug prints, 0-debug prints disabled, 1-9 - debug print level enabled.
          To change debug print level first press "R" (SHIFT+r) then number 0-9
- p     : disable PCIe port (asserts PCIe reset)
- P     : enable PCIe port (if FTRN used outside crate, without MCH, on AMC>PCIe adapter)
- l     : enable MMC LED test, turn all MMC LEDs OFF
- L     : enable MMC LED test, turn all MMC LEDs ON
- o     : disable MMC LED test (use this after done with L or l)

Debug tools
- t     : display Freertos task list with status and stack info
- q     : check IPMI bus I2C pin state
- D     : change IPMI bus I2C pins to GPIO inputs (this way it can be seen if someone else is pulling SCL or SDA line low)
- d     : change IPMI bus I2C pins to I2C (connect back I2C controller to pins after 'D' was used)
- e     : disable IPMI bus I2C controller
- E     : enable IPMI bus I2C controller
- r     : reset IPMI bus I2C controller
- b     : set IPMI bus I2C0 SCL frequency to 60k
- B     : set IPMI bus I2C0 SCL frequency to 100k
- w     : print ws_array states
- u     : unclog ws array, remove all entries (removes all pending request, responses and events)
- s     : list I2C0/IPMI-B bus activity log
- c     : print out Callout Queue array



When "s" was pressed to print out I2C0/IPMI-B bus activity log, three columns will be printed where columns are:
1. index of the log ring buffer (last write to log is in the last row of the printed table)
2. code of the I2CSTAT register
3. if 00 then this write to log happened on I2C IRQ therefore see what is in column 2,
   otherwise this was write to I2CONSET, I2CONCLR or I2C0DAT register (see also column 2)

Codes in column 2 are (see also ./LPC2136_FreeRTOS_CoreIPM/src/coreIPM/i2c.h):

- FF 	I2STAT_NADDR_SLAVE_MODE
- FE 	I2STAT_START_MASTER
- 08 	I2STAT_START_SENT
- 10 	I2STAT_REP_START_SENT
- 38 	I2STAT_ARBITRATION_LOST
- 18 	I2STAT_SLAW_SENT_ACKED
- 20 	I2STAT_SLAW_SENT_NOT_ACKED
- 28 	I2STAT_MASTER_DATA_SENT_ACKED
- 30 	I2STAT_MASTER_DATA_SENT_NOT_ACKED
- 40 	I2STAT_SLAR_SENT_ACKED
- 48 	I2STAT_SLAR_SENT_NOT_ACKED
- 50 	I2STAT_MASTER_DATA_RCVD_ACKED
- 58 	I2STAT_MASTER_DATA_RCVD_NOT_ACKED
- 06 	I2STAT_SLAW_RCVD_ACKED
- 68 	I2STAT_ARB_LOST_SLAW_RCVD_ACKED
- 70 	I2STAT_GENERAL_CALL_RCVD_ACKED
- 78 	I2STAT_ARB_LOST_GENERAL_CALL_RCVD_ACKED
- 80 	I2STAT_SLAVE_DATA_RCVD_ACKED
- 88 	I2STAT_SLAVE_DATA_RCVD_NOT_ACKED
- 90 	I2STAT_GENERAL_CALL_DATA_RCVD_ACKED
- 98 	I2STAT_GENERAL_CALL_DATA_RCVD_NOT_ACKED
- A0 	I2STAT_STOP_START_RCVD
- A8 	I2STAT_SLAR_RCVD_ACKED
- B0 	I2STAT_ARB_LOST_SLAR_RCVD_ACKED
- B8 	I2STAT_SLAVE_DATA_SENT_ACKED
- C0 	I2STAT_SLAVE_DATA_SENT_NOT_ACKED
- C8 	I2STAT_LAST_BYTE_SENT_ACKED
- F8 	I2STAT_NO_INFO
- 00 	I2STAT_BUS_ERROR
- 01 	I2CONSET_WRITE
- 02 	I2CONCLR_WRITE
- 04 	I2C0DAT_WRITE
