################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../card.cpp \
../check_deck.cpp \
../deck.cpp \
../main.cpp \
../random.cpp \
../stack_of_cards.cpp 

OBJS += \
./card.o \
./check_deck.o \
./deck.o \
./main.o \
./random.o \
./stack_of_cards.o 

CPP_DEPS += \
./card.d \
./check_deck.d \
./deck.d \
./main.d \
./random.d \
./stack_of_cards.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	c++ -I../../cift -O2 -g3 -Wall -c -fmessage-length=0 -finstrument-functions -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


