################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/drivers/auxI2C.c \
../src/drivers/iopin.c \
../src/drivers/lm73.c \
../src/drivers/m24eeprom.c \
../src/drivers/mmcio.c \
../src/drivers/retarget.c \
../src/drivers/uart.c 

OBJS += \
./src/drivers/auxI2C.o \
./src/drivers/iopin.o \
./src/drivers/lm73.o \
./src/drivers/m24eeprom.o \
./src/drivers/mmcio.o \
./src/drivers/retarget.o \
./src/drivers/uart.o 

C_DEPS += \
./src/drivers/auxI2C.d \
./src/drivers/iopin.d \
./src/drivers/lm73.d \
./src/drivers/m24eeprom.d \
./src/drivers/mmcio.d \
./src/drivers/retarget.d \
./src/drivers/uart.d 


# Each subdirectory must supply rules for building sources it contributes
src/drivers/%.o: ../src/drivers/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DNDEBUG -D__CODE_RED -I"../../FreeRTOS_LPC2136/include" -I"../../FreeRTOS_LPC2136/portable" -Os -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mcpu=arm7tdmi -D__REDLIB__ -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


