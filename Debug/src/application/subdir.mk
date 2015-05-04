################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/application/KaHyPar.cc 

OBJS += \
./src/application/KaHyPar.o 

CC_DEPS += \
./src/application/KaHyPar.d 


# Each subdirectory must supply rules for building sources it contributes
src/application/%.o: ../src/application/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


