################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../portable/heap_1.c \
../portable/port.c \
../portable/portISR.c 

OBJS += \
./portable/heap_1.o \
./portable/port.o \
./portable/portISR.o 

C_DEPS += \
./portable/heap_1.d \
./portable/port.d \
./portable/portISR.d 


# Each subdirectory must supply rules for building sources it contributes
portable/%.o: ../portable/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DNDEBUG -D__CODE_RED -I"../include" -I"../portable" -Os -g -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mcpu=arm7tdmi -D__REDLIB__ -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


