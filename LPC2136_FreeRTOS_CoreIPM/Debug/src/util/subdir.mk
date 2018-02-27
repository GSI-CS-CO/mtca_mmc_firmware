################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/util/callbackqueue.c \
../src/util/report.c 

OBJS += \
./src/util/callbackqueue.o \
./src/util/report.o 

C_DEPS += \
./src/util/callbackqueue.d \
./src/util/report.d 


# Each subdirectory must supply rules for building sources it contributes
src/util/%.o: ../src/util/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DDEBUG -D__CODE_RED -I"../../FreeRTOS_LPC2136/include" -I"../../FreeRTOS_LPC2136/portable" -O0 -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mcpu=arm7tdmi -D__REDLIB__ -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


