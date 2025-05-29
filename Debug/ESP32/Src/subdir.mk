################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ESP32/Src/esp32.c 

OBJS += \
./ESP32/Src/esp32.o 

C_DEPS += \
./ESP32/Src/esp32.d 


# Each subdirectory must supply rules for building sources it contributes
ESP32/Src/%.o ESP32/Src/%.su ESP32/Src/%.cyclo: ../ESP32/Src/%.c ESP32/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"../FreeRTOS/include" -I"../FreeRTOS/portable" -I"../FreeRTOS/portable/ARM_CM4F" -I"../Shell/Inc" -I../LCD/Inc -I../LED -I"../ESP32/Inc" -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -I"../WM8978/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-ESP32-2f-Src

clean-ESP32-2f-Src:
	-$(RM) ./ESP32/Src/esp32.cyclo ./ESP32/Src/esp32.d ./ESP32/Src/esp32.o ./ESP32/Src/esp32.su

.PHONY: clean-ESP32-2f-Src

