################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/D131MWServo.c \
../Core/Src/Feedback360AngleTest.c \
../Core/Src/Feedback360Servo.c \
../Core/Src/LED.c \
../Core/Src/MX28AR.c \
../Core/Src/SRP_PHAT.c \
../Core/Src/SoundMotorController.c \
../Core/Src/audio_aec_bf_pcm.c \
../Core/Src/dma.c \
../Core/Src/gpio.c \
../Core/Src/main.c \
../Core/Src/stm32h7xx_hal_msp.c \
../Core/Src/stm32h7xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/tim.c \
../Core/Src/usart.c 

OBJS += \
./Core/Src/D131MWServo.o \
./Core/Src/Feedback360AngleTest.o \
./Core/Src/Feedback360Servo.o \
./Core/Src/LED.o \
./Core/Src/MX28AR.o \
./Core/Src/SRP_PHAT.o \
./Core/Src/SoundMotorController.o \
./Core/Src/audio_aec_bf_pcm.o \
./Core/Src/dma.o \
./Core/Src/gpio.o \
./Core/Src/main.o \
./Core/Src/stm32h7xx_hal_msp.o \
./Core/Src/stm32h7xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/tim.o \
./Core/Src/usart.o 

C_DEPS += \
./Core/Src/D131MWServo.d \
./Core/Src/Feedback360AngleTest.d \
./Core/Src/Feedback360Servo.d \
./Core/Src/LED.d \
./Core/Src/MX28AR.d \
./Core/Src/SRP_PHAT.d \
./Core/Src/SoundMotorController.d \
./Core/Src/audio_aec_bf_pcm.d \
./Core/Src/dma.d \
./Core/Src/gpio.d \
./Core/Src/main.d \
./Core/Src/stm32h7xx_hal_msp.d \
./Core/Src/stm32h7xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/tim.d \
./Core/Src/usart.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H755xx -DUSE_PWR_DIRECT_SMPS_SUPPLY -c -I../Core/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../DSP/Include -I../../shared/Inc -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I"C:/Users/MSI/Documents/00_KC_Project/07_speech_compass/01_work/RobotEar_odas/RobotEar_stm32h755_Tsst/00. Test/RobotEar_H755/CM7/STM32_AcousticBF_Library/Src" -I"C:/Users/MSI/Documents/00_KC_Project/07_speech_compass/01_work/RobotEar_odas/RobotEar_stm32h755_Tsst/00. Test/RobotEar_H755/CM7/STM32_AcousticEC_Library/Src" -I"C:/Users/MSI/Documents/00_KC_Project/07_speech_compass/01_work/RobotEar_odas/RobotEar_stm32h755_Tsst/00. Test/RobotEar_H755/CM7/STM32_AcousticBF_Library/Inc" -I"C:/Users/MSI/Documents/00_KC_Project/07_speech_compass/01_work/RobotEar_odas/RobotEar_stm32h755_Tsst/00. Test/RobotEar_H755/CM7/STM32_AcousticEC_Library/Inc" -I"C:/Users/MSI/Documents/00_KC_Project/07_speech_compass/01_work/RobotEar_odas/RobotEar_stm32h755_Tsst/00. Test/RobotEar_H755/CM7/STM32_Audio/Addons/PDM/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/D131MWServo.cyclo ./Core/Src/D131MWServo.d ./Core/Src/D131MWServo.o ./Core/Src/D131MWServo.su ./Core/Src/Feedback360AngleTest.cyclo ./Core/Src/Feedback360AngleTest.d ./Core/Src/Feedback360AngleTest.o ./Core/Src/Feedback360AngleTest.su ./Core/Src/Feedback360Servo.cyclo ./Core/Src/Feedback360Servo.d ./Core/Src/Feedback360Servo.o ./Core/Src/Feedback360Servo.su ./Core/Src/LED.cyclo ./Core/Src/LED.d ./Core/Src/LED.o ./Core/Src/LED.su ./Core/Src/MX28AR.cyclo ./Core/Src/MX28AR.d ./Core/Src/MX28AR.o ./Core/Src/MX28AR.su ./Core/Src/SRP_PHAT.cyclo ./Core/Src/SRP_PHAT.d ./Core/Src/SRP_PHAT.o ./Core/Src/SRP_PHAT.su ./Core/Src/SoundMotorController.cyclo ./Core/Src/SoundMotorController.d ./Core/Src/SoundMotorController.o ./Core/Src/SoundMotorController.su ./Core/Src/audio_aec_bf_pcm.cyclo ./Core/Src/audio_aec_bf_pcm.d ./Core/Src/audio_aec_bf_pcm.o ./Core/Src/audio_aec_bf_pcm.su ./Core/Src/dma.cyclo ./Core/Src/dma.d ./Core/Src/dma.o ./Core/Src/dma.su ./Core/Src/gpio.cyclo ./Core/Src/gpio.d ./Core/Src/gpio.o ./Core/Src/gpio.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/stm32h7xx_hal_msp.cyclo ./Core/Src/stm32h7xx_hal_msp.d ./Core/Src/stm32h7xx_hal_msp.o ./Core/Src/stm32h7xx_hal_msp.su ./Core/Src/stm32h7xx_it.cyclo ./Core/Src/stm32h7xx_it.d ./Core/Src/stm32h7xx_it.o ./Core/Src/stm32h7xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/tim.cyclo ./Core/Src/tim.d ./Core/Src/tim.o ./Core/Src/tim.su ./Core/Src/usart.cyclo ./Core/Src/usart.d ./Core/Src/usart.o ./Core/Src/usart.su

.PHONY: clean-Core-2f-Src

