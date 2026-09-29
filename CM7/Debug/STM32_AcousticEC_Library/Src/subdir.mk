################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../STM32_AcousticEC_Library/Src/Echo_library.c \
../STM32_AcousticEC_Library/Src/acousticEC.c \
../STM32_AcousticEC_Library/Src/echo.c \
../STM32_AcousticEC_Library/Src/filterbank.c \
../STM32_AcousticEC_Library/Src/preprocess.c \
../STM32_AcousticEC_Library/Src/smallft.c 

OBJS += \
./STM32_AcousticEC_Library/Src/Echo_library.o \
./STM32_AcousticEC_Library/Src/acousticEC.o \
./STM32_AcousticEC_Library/Src/echo.o \
./STM32_AcousticEC_Library/Src/filterbank.o \
./STM32_AcousticEC_Library/Src/preprocess.o \
./STM32_AcousticEC_Library/Src/smallft.o 

C_DEPS += \
./STM32_AcousticEC_Library/Src/Echo_library.d \
./STM32_AcousticEC_Library/Src/acousticEC.d \
./STM32_AcousticEC_Library/Src/echo.d \
./STM32_AcousticEC_Library/Src/filterbank.d \
./STM32_AcousticEC_Library/Src/preprocess.d \
./STM32_AcousticEC_Library/Src/smallft.d 


# Each subdirectory must supply rules for building sources it contributes
STM32_AcousticEC_Library/Src/%.o STM32_AcousticEC_Library/Src/%.su STM32_AcousticEC_Library/Src/%.cyclo: ../STM32_AcousticEC_Library/Src/%.c STM32_AcousticEC_Library/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H755xx -DUSE_PWR_DIRECT_SMPS_SUPPLY -c -I../Core/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../DSP/Include -I../../shared/Inc -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I"C:/Users/MSI/Documents/00_KC_Project/07_speech_compass/01_work/RobotEar_odas/RobotEar_stm32h755_Tsst/00. Test/RobotEar_H755/CM7/STM32_AcousticBF_Library/Src" -I"C:/Users/MSI/Documents/00_KC_Project/07_speech_compass/01_work/RobotEar_odas/RobotEar_stm32h755_Tsst/00. Test/RobotEar_H755/CM7/STM32_AcousticEC_Library/Src" -I"C:/Users/MSI/Documents/00_KC_Project/07_speech_compass/01_work/RobotEar_odas/RobotEar_stm32h755_Tsst/00. Test/RobotEar_H755/CM7/STM32_AcousticBF_Library/Inc" -I"C:/Users/MSI/Documents/00_KC_Project/07_speech_compass/01_work/RobotEar_odas/RobotEar_stm32h755_Tsst/00. Test/RobotEar_H755/CM7/STM32_AcousticEC_Library/Inc" -I"C:/Users/MSI/Documents/00_KC_Project/07_speech_compass/01_work/RobotEar_odas/RobotEar_stm32h755_Tsst/00. Test/RobotEar_H755/CM7/STM32_Audio/Addons/PDM/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-STM32_AcousticEC_Library-2f-Src

clean-STM32_AcousticEC_Library-2f-Src:
	-$(RM) ./STM32_AcousticEC_Library/Src/Echo_library.cyclo ./STM32_AcousticEC_Library/Src/Echo_library.d ./STM32_AcousticEC_Library/Src/Echo_library.o ./STM32_AcousticEC_Library/Src/Echo_library.su ./STM32_AcousticEC_Library/Src/acousticEC.cyclo ./STM32_AcousticEC_Library/Src/acousticEC.d ./STM32_AcousticEC_Library/Src/acousticEC.o ./STM32_AcousticEC_Library/Src/acousticEC.su ./STM32_AcousticEC_Library/Src/echo.cyclo ./STM32_AcousticEC_Library/Src/echo.d ./STM32_AcousticEC_Library/Src/echo.o ./STM32_AcousticEC_Library/Src/echo.su ./STM32_AcousticEC_Library/Src/filterbank.cyclo ./STM32_AcousticEC_Library/Src/filterbank.d ./STM32_AcousticEC_Library/Src/filterbank.o ./STM32_AcousticEC_Library/Src/filterbank.su ./STM32_AcousticEC_Library/Src/preprocess.cyclo ./STM32_AcousticEC_Library/Src/preprocess.d ./STM32_AcousticEC_Library/Src/preprocess.o ./STM32_AcousticEC_Library/Src/preprocess.su ./STM32_AcousticEC_Library/Src/smallft.cyclo ./STM32_AcousticEC_Library/Src/smallft.d ./STM32_AcousticEC_Library/Src/smallft.o ./STM32_AcousticEC_Library/Src/smallft.su

.PHONY: clean-STM32_AcousticEC_Library-2f-Src

