################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../STM32_AcousticBF_Library/Src/acoustic_bf.c \
../STM32_AcousticBF_Library/Src/acoustic_bf_cardoid.c \
../STM32_AcousticBF_Library/Src/acoustic_bf_speex.c \
../STM32_AcousticBF_Library/Src/acoustic_bf_speex_v1.2.1.c \
../STM32_AcousticBF_Library/Src/adaptive.c \
../STM32_AcousticBF_Library/Src/cardoid.c \
../STM32_AcousticBF_Library/Src/delay.c \
../STM32_AcousticBF_Library/Src/denoiser.c \
../STM32_AcousticBF_Library/Src/filterbank.c \
../STM32_AcousticBF_Library/Src/libBeamforming.c \
../STM32_AcousticBF_Library/Src/smallft.c 

OBJS += \
./STM32_AcousticBF_Library/Src/acoustic_bf.o \
./STM32_AcousticBF_Library/Src/acoustic_bf_cardoid.o \
./STM32_AcousticBF_Library/Src/acoustic_bf_speex.o \
./STM32_AcousticBF_Library/Src/acoustic_bf_speex_v1.2.1.o \
./STM32_AcousticBF_Library/Src/adaptive.o \
./STM32_AcousticBF_Library/Src/cardoid.o \
./STM32_AcousticBF_Library/Src/delay.o \
./STM32_AcousticBF_Library/Src/denoiser.o \
./STM32_AcousticBF_Library/Src/filterbank.o \
./STM32_AcousticBF_Library/Src/libBeamforming.o \
./STM32_AcousticBF_Library/Src/smallft.o 

C_DEPS += \
./STM32_AcousticBF_Library/Src/acoustic_bf.d \
./STM32_AcousticBF_Library/Src/acoustic_bf_cardoid.d \
./STM32_AcousticBF_Library/Src/acoustic_bf_speex.d \
./STM32_AcousticBF_Library/Src/acoustic_bf_speex_v1.2.1.d \
./STM32_AcousticBF_Library/Src/adaptive.d \
./STM32_AcousticBF_Library/Src/cardoid.d \
./STM32_AcousticBF_Library/Src/delay.d \
./STM32_AcousticBF_Library/Src/denoiser.d \
./STM32_AcousticBF_Library/Src/filterbank.d \
./STM32_AcousticBF_Library/Src/libBeamforming.d \
./STM32_AcousticBF_Library/Src/smallft.d 


# Each subdirectory must supply rules for building sources it contributes
STM32_AcousticBF_Library/Src/%.o STM32_AcousticBF_Library/Src/%.su STM32_AcousticBF_Library/Src/%.cyclo: ../STM32_AcousticBF_Library/Src/%.c STM32_AcousticBF_Library/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H755xx -DUSE_PWR_DIRECT_SMPS_SUPPLY -c -I../Core/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../DSP/Include -I../../shared/Inc -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I"C:/Users/MSI/Documents/00_KC_Project/07_speech_compass/01_work/RobotEar_odas/RobotEar_stm32h755_Tsst/00. Test/RobotEar_H755/CM7/STM32_AcousticBF_Library/Src" -I"C:/Users/MSI/Documents/00_KC_Project/07_speech_compass/01_work/RobotEar_odas/RobotEar_stm32h755_Tsst/00. Test/RobotEar_H755/CM7/STM32_AcousticEC_Library/Src" -I"C:/Users/MSI/Documents/00_KC_Project/07_speech_compass/01_work/RobotEar_odas/RobotEar_stm32h755_Tsst/00. Test/RobotEar_H755/CM7/STM32_AcousticBF_Library/Inc" -I"C:/Users/MSI/Documents/00_KC_Project/07_speech_compass/01_work/RobotEar_odas/RobotEar_stm32h755_Tsst/00. Test/RobotEar_H755/CM7/STM32_AcousticEC_Library/Inc" -I"C:/Users/MSI/Documents/00_KC_Project/07_speech_compass/01_work/RobotEar_odas/RobotEar_stm32h755_Tsst/00. Test/RobotEar_H755/CM7/STM32_Audio/Addons/PDM/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-STM32_AcousticBF_Library-2f-Src

clean-STM32_AcousticBF_Library-2f-Src:
	-$(RM) ./STM32_AcousticBF_Library/Src/acoustic_bf.cyclo ./STM32_AcousticBF_Library/Src/acoustic_bf.d ./STM32_AcousticBF_Library/Src/acoustic_bf.o ./STM32_AcousticBF_Library/Src/acoustic_bf.su ./STM32_AcousticBF_Library/Src/acoustic_bf_cardoid.cyclo ./STM32_AcousticBF_Library/Src/acoustic_bf_cardoid.d ./STM32_AcousticBF_Library/Src/acoustic_bf_cardoid.o ./STM32_AcousticBF_Library/Src/acoustic_bf_cardoid.su ./STM32_AcousticBF_Library/Src/acoustic_bf_speex.cyclo ./STM32_AcousticBF_Library/Src/acoustic_bf_speex.d ./STM32_AcousticBF_Library/Src/acoustic_bf_speex.o ./STM32_AcousticBF_Library/Src/acoustic_bf_speex.su ./STM32_AcousticBF_Library/Src/acoustic_bf_speex_v1.2.1.cyclo ./STM32_AcousticBF_Library/Src/acoustic_bf_speex_v1.2.1.d ./STM32_AcousticBF_Library/Src/acoustic_bf_speex_v1.2.1.o ./STM32_AcousticBF_Library/Src/acoustic_bf_speex_v1.2.1.su ./STM32_AcousticBF_Library/Src/adaptive.cyclo ./STM32_AcousticBF_Library/Src/adaptive.d ./STM32_AcousticBF_Library/Src/adaptive.o ./STM32_AcousticBF_Library/Src/adaptive.su ./STM32_AcousticBF_Library/Src/cardoid.cyclo ./STM32_AcousticBF_Library/Src/cardoid.d ./STM32_AcousticBF_Library/Src/cardoid.o ./STM32_AcousticBF_Library/Src/cardoid.su ./STM32_AcousticBF_Library/Src/delay.cyclo ./STM32_AcousticBF_Library/Src/delay.d ./STM32_AcousticBF_Library/Src/delay.o ./STM32_AcousticBF_Library/Src/delay.su ./STM32_AcousticBF_Library/Src/denoiser.cyclo ./STM32_AcousticBF_Library/Src/denoiser.d ./STM32_AcousticBF_Library/Src/denoiser.o ./STM32_AcousticBF_Library/Src/denoiser.su ./STM32_AcousticBF_Library/Src/filterbank.cyclo ./STM32_AcousticBF_Library/Src/filterbank.d ./STM32_AcousticBF_Library/Src/filterbank.o ./STM32_AcousticBF_Library/Src/filterbank.su ./STM32_AcousticBF_Library/Src/libBeamforming.cyclo ./STM32_AcousticBF_Library/Src/libBeamforming.d ./STM32_AcousticBF_Library/Src/libBeamforming.o ./STM32_AcousticBF_Library/Src/libBeamforming.su ./STM32_AcousticBF_Library/Src/smallft.cyclo ./STM32_AcousticBF_Library/Src/smallft.d ./STM32_AcousticBF_Library/Src/smallft.o ./STM32_AcousticBF_Library/Src/smallft.su

.PHONY: clean-STM32_AcousticBF_Library-2f-Src

