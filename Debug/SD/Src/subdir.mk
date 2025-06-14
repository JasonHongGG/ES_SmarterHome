################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../SD/Src/log.c \
../SD/Src/sd.c 

OBJS += \
./SD/Src/log.o \
./SD/Src/sd.o 

C_DEPS += \
./SD/Src/log.d \
./SD/Src/sd.d 


# Each subdirectory must supply rules for building sources it contributes
SD/Src/%.o SD/Src/%.su SD/Src/%.cyclo: ../SD/Src/%.c SD/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"../FreeRTOS/include" -I"../FreeRTOS/portable" -I"../FreeRTOS/portable/ARM_CM4F" -I"../Shell/Inc" -I../LCD/Inc -I../LED -I"../ESP32/Inc" -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -I"../WM8978/Inc" -I"../SD/Inc" -I../Timer/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-SD-2f-Src

clean-SD-2f-Src:
	-$(RM) ./SD/Src/log.cyclo ./SD/Src/log.d ./SD/Src/log.o ./SD/Src/log.su ./SD/Src/sd.cyclo ./SD/Src/sd.d ./SD/Src/sd.o ./SD/Src/sd.su

.PHONY: clean-SD-2f-Src

