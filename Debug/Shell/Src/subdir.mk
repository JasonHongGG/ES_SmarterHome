################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Shell/Src/cmd.c \
../Shell/Src/readLine.c \
../Shell/Src/shell.c 

OBJS += \
./Shell/Src/cmd.o \
./Shell/Src/readLine.o \
./Shell/Src/shell.o 

C_DEPS += \
./Shell/Src/cmd.d \
./Shell/Src/readLine.d \
./Shell/Src/shell.d 


# Each subdirectory must supply rules for building sources it contributes
Shell/Src/%.o Shell/Src/%.su Shell/Src/%.cyclo: ../Shell/Src/%.c Shell/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"../FreeRTOS/include" -I"../FreeRTOS/portable" -I"../FreeRTOS/portable/ARM_CM4F" -I"../Shell/Inc" -I../LCD/Inc -I../LED -I"../ESP32/Inc" -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -I"../WM8978/Inc" -I"../SD/Inc" -I../Timer/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Shell-2f-Src

clean-Shell-2f-Src:
	-$(RM) ./Shell/Src/cmd.cyclo ./Shell/Src/cmd.d ./Shell/Src/cmd.o ./Shell/Src/cmd.su ./Shell/Src/readLine.cyclo ./Shell/Src/readLine.d ./Shell/Src/readLine.o ./Shell/Src/readLine.su ./Shell/Src/shell.cyclo ./Shell/Src/shell.d ./Shell/Src/shell.o ./Shell/Src/shell.su

.PHONY: clean-Shell-2f-Src

