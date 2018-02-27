freertos:
	cd FreeRTOS_LPC2136/Release && $(MAKE)

freertos-clean:
	cd FreeRTOS_LPC2136/Release && $(MAKE) clean

freertos_debug:
	cd FreeRTOS_LPC2136/Debug && $(MAKE)

freertos_debug-clean:
	cd FreeRTOS_LPC2136/Debug && $(MAKE) clean

mmc: freertos_debug
	cd LPC2136_FreeRTOS_CoreIPM/Debug && $(MAKE)

mmc-clean: freertos_debug-clean
	cd LPC2136_FreeRTOS_CoreIPM/Debug && $(MAKE) clean

debug: freertos_debug
	cd LPC2136_FreeRTOS_CoreIPM/Debug && $(MAKE)

debug-clean: freertos_debug-clean
	cd LPC2136_FreeRTOS_CoreIPM/Debug && $(MAKE) clean

# Program MMC using LPC-Link
flash: mmc
	crt_emu_a7_nxp -flash-load-partial ./LPC2136_FreeRTOS_CoreIPM/Debug/LPC2136_FreeRTOS_CoreIPM.axf -g -2 -vendor=NXP -pLPC2138  -wire=winUSB -s100

flash-debug: debug
	crt_emu_a7_nxp -flash-load-partial ./LPC2136_FreeRTOS_CoreIPM/Debug/LPC2136_FreeRTOS_CoreIPM.axf   -g -2 -vendor=NXP -pLPC2138  -wire=winUSB -s100

# Init Philips (or NXP) LPCXpresso LPC-Link [0001] as LPC-Link Probe
lpclink-init: 
	dfu-util -d 0x471:0xdf55 -c 0 -t 2048 -R -D /usr/local/lpcxpresso_8.2.2_650/lpcxpresso/bin/LPCXpressoWIN.enc
	-@echo " "
	-@echo "Power cycle the target then run 'make lpclink-test'"

# test LPC-Link Probe
lpclink-test:
	crt_emu_a7_nxp -info-emu -wire=winusb
	crt_emu_a7_nxp -info-target -pLPC2138 -wire=winusb -4
