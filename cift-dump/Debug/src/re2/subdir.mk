################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/re2/memory.c \
../src/re2/regerror.c \
../src/re2/regexp.c \
../src/re2/regsub.c \
../src/re2/report.c 

OBJS += \
./src/re2/memory.o \
./src/re2/regerror.o \
./src/re2/regexp.o \
./src/re2/regsub.o \
./src/re2/report.o 

C_DEPS += \
./src/re2/memory.d \
./src/re2/regerror.d \
./src/re2/regexp.d \
./src/re2/regsub.d \
./src/re2/report.d 


# Each subdirectory must supply rules for building sources it contributes
src/re2/%.o: ../src/re2/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I../src/re2 -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


