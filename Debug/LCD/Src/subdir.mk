################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../LCD/Src/i2c_lcd.c \
../LCD/Src/lcd2004.c 

OBJS += \
./LCD/Src/i2c_lcd.o \
./LCD/Src/lcd2004.o 

C_DEPS += \
./LCD/Src/i2c_lcd.d \
./LCD/Src/lcd2004.d 


# Each subdirectory must supply rules for building sources it contributes
LCD/Src/%.o LCD/Src/%.su LCD/Src/%.cyclo: ../LCD/Src/%.c LCD/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"../FreeRTOS/include" -I"../FreeRTOS/portable" -I"../FreeRTOS/portable/ARM_CM4F" -I"../Shell/Inc" -I../LCD/Inc -I../LED -I"../ESP32/Inc" -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -I"../WM8978/Inc" -I"../SD/Inc" -I../Timer/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-LCD-2f-Src

clean-LCD-2f-Src:
	-$(RM) ./LCD/Src/i2c_lcd.cyclo ./LCD/Src/i2c_lcd.d ./LCD/Src/i2c_lcd.o ./LCD/Src/i2c_lcd.su ./LCD/Src/lcd2004.cyclo ./LCD/Src/lcd2004.d ./LCD/Src/lcd2004.o ./LCD/Src/lcd2004.su

.PHONY: clean-LCD-2f-Src

