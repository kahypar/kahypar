################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../gmock/gtest/samples/sample1.cc \
../gmock/gtest/samples/sample10_unittest.cc \
../gmock/gtest/samples/sample1_unittest.cc \
../gmock/gtest/samples/sample2.cc \
../gmock/gtest/samples/sample2_unittest.cc \
../gmock/gtest/samples/sample3_unittest.cc \
../gmock/gtest/samples/sample4.cc \
../gmock/gtest/samples/sample4_unittest.cc \
../gmock/gtest/samples/sample5_unittest.cc \
../gmock/gtest/samples/sample6_unittest.cc \
../gmock/gtest/samples/sample7_unittest.cc \
../gmock/gtest/samples/sample8_unittest.cc \
../gmock/gtest/samples/sample9_unittest.cc 

OBJS += \
./gmock/gtest/samples/sample1.o \
./gmock/gtest/samples/sample10_unittest.o \
./gmock/gtest/samples/sample1_unittest.o \
./gmock/gtest/samples/sample2.o \
./gmock/gtest/samples/sample2_unittest.o \
./gmock/gtest/samples/sample3_unittest.o \
./gmock/gtest/samples/sample4.o \
./gmock/gtest/samples/sample4_unittest.o \
./gmock/gtest/samples/sample5_unittest.o \
./gmock/gtest/samples/sample6_unittest.o \
./gmock/gtest/samples/sample7_unittest.o \
./gmock/gtest/samples/sample8_unittest.o \
./gmock/gtest/samples/sample9_unittest.o 

CC_DEPS += \
./gmock/gtest/samples/sample1.d \
./gmock/gtest/samples/sample10_unittest.d \
./gmock/gtest/samples/sample1_unittest.d \
./gmock/gtest/samples/sample2.d \
./gmock/gtest/samples/sample2_unittest.d \
./gmock/gtest/samples/sample3_unittest.d \
./gmock/gtest/samples/sample4.d \
./gmock/gtest/samples/sample4_unittest.d \
./gmock/gtest/samples/sample5_unittest.d \
./gmock/gtest/samples/sample6_unittest.d \
./gmock/gtest/samples/sample7_unittest.d \
./gmock/gtest/samples/sample8_unittest.d \
./gmock/gtest/samples/sample9_unittest.d 


# Each subdirectory must supply rules for building sources it contributes
gmock/gtest/samples/%.o: ../gmock/gtest/samples/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


