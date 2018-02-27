################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
../src/cr_startup_lpc21.s 

C_SRCS += \
../src/main.c \
../src/mmc_config.c \
../src/payload.c 

OBJS += \
./src/cr_startup_lpc21.o \
./src/main.o \
./src/mmc_config.o \
./src/payload.o 

C_DEPS += \
./src/main.d \
./src/mmc_config.d \
./src/payload.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.s
	@echo 'Building file: $<'
	@echo 'Invoking: MCU Assembler'
	arm-none-eabi-gcc -c -x assembler-with-cpp -D__REDLIB__ -DDEBUG -D__CODE_RED -g3 -mcpu=arm7tdmi -D__REDLIB__ -specs=redlib.specs -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DDEBUG -D__CODE_RED -I"../../FreeRTOS_LPC2136/include" -I"../../FreeRTOS_LPC2136/portable" -O0 -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mcpu=arm7tdmi -D__REDLIB__ -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


