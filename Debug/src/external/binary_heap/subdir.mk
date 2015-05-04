################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/external/binary_heap/HeapStatisticData.cpp 

OBJS += \
./src/external/binary_heap/HeapStatisticData.o 

CPP_DEPS += \
./src/external/binary_heap/HeapStatisticData.d 


# Each subdirectory must supply rules for building sources it contributes
src/external/binary_heap/%.o: ../src/external/binary_heap/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


