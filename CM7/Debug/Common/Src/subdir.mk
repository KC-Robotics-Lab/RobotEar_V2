################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/MSI/Documents/00_KC_Project/07_speech_compass/01_work/RobotEar_odas/RobotEar_stm32h755_Tsst/00.\ Test/RobotEar_H755/Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.c 

OBJS += \
./Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.o 

C_DEPS += \
./Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.d 


# Each subdirectory must supply rules for building sources it contributes
Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.o: C:/Users/MSI/Documents/00_KC_Project/07_speech_compass/01_work/RobotEar_odas/RobotEar_stm32h755_Tsst/00.\ Test/RobotEar_H755/Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.c Common/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H755xx -DUSE_PWR_DIRECT_SMPS_SUPPLY -c -I../Core/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../DSP/Include -I../../shared/Inc -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I"C:/Users/MSI/Documents/00_KC_Project/07_speech_compass/01_work/RobotEar_odas/RobotEar_stm32h755_Tsst/00. Test/RobotEar_H755/CM7/STM32_AcousticBF_Library/Src" -I"C:/Users/MSI/Documents/00_KC_Project/07_speech_compass/01_work/RobotEar_odas/RobotEar_stm32h755_Tsst/00. Test/RobotEar_H755/CM7/STM32_AcousticEC_Library/Src" -I"C:/Users/MSI/Documents/00_KC_Project/07_speech_compass/01_work/RobotEar_odas/RobotEar_stm32h755_Tsst/00. Test/RobotEar_H755/CM7/STM32_AcousticBF_Library/Inc" -I"C:/Users/MSI/Documents/00_KC_Project/07_speech_compass/01_work/RobotEar_odas/RobotEar_stm32h755_Tsst/00. Test/RobotEar_H755/CM7/STM32_AcousticEC_Library/Inc" -I"C:/Users/MSI/Documents/00_KC_Project/07_speech_compass/01_work/RobotEar_odas/RobotEar_stm32h755_Tsst/00. Test/RobotEar_H755/CM7/STM32_Audio/Addons/PDM/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Common-2f-Src

clean-Common-2f-Src:
	-$(RM) ./Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.cyclo ./Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.d ./Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.o ./Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.su

.PHONY: clean-Common-2f-Src

