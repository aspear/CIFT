################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/cift-dump2.cpp \
../src/nm-symbol-extractor.cpp 

OBJS += \
./src/cift-dump2.o \
./src/nm-symbol-extractor.o 

CPP_DEPS += \
./src/cift-dump2.d \
./src/nm-symbol-extractor.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I../../cift -I../src/re2 -I../ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


