################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../gmock/gtest/fused-src/gtest/gtest-all.cc \
../gmock/gtest/fused-src/gtest/gtest_main.cc 

OBJS += \
./gmock/gtest/fused-src/gtest/gtest-all.o \
./gmock/gtest/fused-src/gtest/gtest_main.o 

CC_DEPS += \
./gmock/gtest/fused-src/gtest/gtest-all.d \
./gmock/gtest/fused-src/gtest/gtest_main.d 


# Each subdirectory must supply rules for building sources it contributes
gmock/gtest/fused-src/gtest/%.o: ../gmock/gtest/fused-src/gtest/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


