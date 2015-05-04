################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../gmock/fused-src/gmock-gtest-all.cc \
../gmock/fused-src/gmock_main.cc 

OBJS += \
./gmock/fused-src/gmock-gtest-all.o \
./gmock/fused-src/gmock_main.o 

CC_DEPS += \
./gmock/fused-src/gmock-gtest-all.d \
./gmock/fused-src/gmock_main.d 


# Each subdirectory must supply rules for building sources it contributes
gmock/fused-src/%.o: ../gmock/fused-src/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


