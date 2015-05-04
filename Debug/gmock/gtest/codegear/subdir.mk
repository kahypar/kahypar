################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../gmock/gtest/codegear/gtest_all.cc \
../gmock/gtest/codegear/gtest_link.cc 

OBJS += \
./gmock/gtest/codegear/gtest_all.o \
./gmock/gtest/codegear/gtest_link.o 

CC_DEPS += \
./gmock/gtest/codegear/gtest_all.d \
./gmock/gtest/codegear/gtest_link.d 


# Each subdirectory must supply rules for building sources it contributes
gmock/gtest/codegear/%.o: ../gmock/gtest/codegear/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


