################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/util/subdir.mk
-include src/drivers/subdir.mk
-include src/coreIPM/subdir.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: LPC2136_FreeRTOS_CoreIPM.axf

# Tool invocations
LPC2136_FreeRTOS_CoreIPM.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -L"../../FreeRTOS_LPC2136/Release" -Xlinker -Map="LPC2136_FreeRTOS_CoreIPM.map" -Xlinker --gc-sections -mcpu=arm7tdmi -T "LPC2136_FreeRTOS_CoreIPM_Release.ld" -o "LPC2136_FreeRTOS_CoreIPM.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) LPC2136_FreeRTOS_CoreIPM.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "LPC2136_FreeRTOS_CoreIPM.axf" ; arm-none-eabi-objcopy -v -O binary "LPC2136_FreeRTOS_CoreIPM.axf" "LPC2136_FreeRTOS_CoreIPM.bin" ; checksum -p LPC2138 -d "LPC2136_FreeRTOS_CoreIPM.bin" ; arm-none-eabi-objcopy -v -O ihex "LPC2136_FreeRTOS_CoreIPM.axf" "LPC2136_FreeRTOS_CoreIPM.hex"
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets
