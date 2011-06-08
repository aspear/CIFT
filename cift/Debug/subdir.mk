################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../cift.c \
../cift_file_dump.c 

OBJS += \
./cift.o \
./cift_file_dump.o 

C_DEPS += \
./cift.d \
./cift_file_dump.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DCFT_DEFAULT_BUFFER_MODE=CFT_BUFFER_MODE_FILL_AND_STOP -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


