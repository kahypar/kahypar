################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../gmock/gtest/src/gtest-all.cc \
../gmock/gtest/src/gtest-death-test.cc \
../gmock/gtest/src/gtest-filepath.cc \
../gmock/gtest/src/gtest-port.cc \
../gmock/gtest/src/gtest-printers.cc \
../gmock/gtest/src/gtest-test-part.cc \
../gmock/gtest/src/gtest-typed-test.cc \
../gmock/gtest/src/gtest.cc \
../gmock/gtest/src/gtest_main.cc 

OBJS += \
./gmock/gtest/src/gtest-all.o \
./gmock/gtest/src/gtest-death-test.o \
./gmock/gtest/src/gtest-filepath.o \
./gmock/gtest/src/gtest-port.o \
./gmock/gtest/src/gtest-printers.o \
./gmock/gtest/src/gtest-test-part.o \
./gmock/gtest/src/gtest-typed-test.o \
./gmock/gtest/src/gtest.o \
./gmock/gtest/src/gtest_main.o 

CC_DEPS += \
./gmock/gtest/src/gtest-all.d \
./gmock/gtest/src/gtest-death-test.d \
./gmock/gtest/src/gtest-filepath.d \
./gmock/gtest/src/gtest-port.d \
./gmock/gtest/src/gtest-printers.d \
./gmock/gtest/src/gtest-test-part.d \
./gmock/gtest/src/gtest-typed-test.d \
./gmock/gtest/src/gtest.d \
./gmock/gtest/src/gtest_main.d 


# Each subdirectory must supply rules for building sources it contributes
gmock/gtest/src/%.o: ../gmock/gtest/src/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


