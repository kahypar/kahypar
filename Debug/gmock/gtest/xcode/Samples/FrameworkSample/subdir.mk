################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../gmock/gtest/xcode/Samples/FrameworkSample/widget.cc \
../gmock/gtest/xcode/Samples/FrameworkSample/widget_test.cc 

OBJS += \
./gmock/gtest/xcode/Samples/FrameworkSample/widget.o \
./gmock/gtest/xcode/Samples/FrameworkSample/widget_test.o 

CC_DEPS += \
./gmock/gtest/xcode/Samples/FrameworkSample/widget.d \
./gmock/gtest/xcode/Samples/FrameworkSample/widget_test.d 


# Each subdirectory must supply rules for building sources it contributes
gmock/gtest/xcode/Samples/FrameworkSample/%.o: ../gmock/gtest/xcode/Samples/FrameworkSample/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


