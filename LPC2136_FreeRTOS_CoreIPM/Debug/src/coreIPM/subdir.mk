################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/coreIPM/crp.c \
../src/coreIPM/debug.c \
../src/coreIPM/event.c \
../src/coreIPM/gpio.c \
../src/coreIPM/i2c.c \
../src/coreIPM/ipmi.c \
../src/coreIPM/mmc.c \
../src/coreIPM/module.c \
../src/coreIPM/picmg.c \
../src/coreIPM/sensor.c \
../src/coreIPM/strings.c \
../src/coreIPM/timer.c \
../src/coreIPM/wd.c \
../src/coreIPM/ws.c 

OBJS += \
./src/coreIPM/crp.o \
./src/coreIPM/debug.o \
./src/coreIPM/event.o \
./src/coreIPM/gpio.o \
./src/coreIPM/i2c.o \
./src/coreIPM/ipmi.o \
./src/coreIPM/mmc.o \
./src/coreIPM/module.o \
./src/coreIPM/picmg.o \
./src/coreIPM/sensor.o \
./src/coreIPM/strings.o \
./src/coreIPM/timer.o \
./src/coreIPM/wd.o \
./src/coreIPM/ws.o 

C_DEPS += \
./src/coreIPM/crp.d \
./src/coreIPM/debug.d \
./src/coreIPM/event.d \
./src/coreIPM/gpio.d \
./src/coreIPM/i2c.d \
./src/coreIPM/ipmi.d \
./src/coreIPM/mmc.d \
./src/coreIPM/module.d \
./src/coreIPM/picmg.d \
./src/coreIPM/sensor.d \
./src/coreIPM/strings.d \
./src/coreIPM/timer.d \
./src/coreIPM/wd.d \
./src/coreIPM/ws.d 


# Each subdirectory must supply rules for building sources it contributes
src/coreIPM/%.o: ../src/coreIPM/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DDEBUG -D__CODE_RED -I"../../FreeRTOS_LPC2136/include" -I"../../FreeRTOS_LPC2136/portable" -O0 -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mcpu=arm7tdmi -D__REDLIB__ -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


