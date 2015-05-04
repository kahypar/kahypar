################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/tools/GraphToHgrConverter.cc \
../src/tools/HypergraphStats.cc \
../src/tools/MtxToHgrConversion.cc \
../src/tools/MtxToHgrConverter.cc \
../src/tools/RandomFunctions.cc \
../src/tools/mtx_to_hgr_converter_test.cc 

OBJS += \
./src/tools/GraphToHgrConverter.o \
./src/tools/HypergraphStats.o \
./src/tools/MtxToHgrConversion.o \
./src/tools/MtxToHgrConverter.o \
./src/tools/RandomFunctions.o \
./src/tools/mtx_to_hgr_converter_test.o 

CC_DEPS += \
./src/tools/GraphToHgrConverter.d \
./src/tools/HypergraphStats.d \
./src/tools/MtxToHgrConversion.d \
./src/tools/MtxToHgrConverter.d \
./src/tools/RandomFunctions.d \
./src/tools/mtx_to_hgr_converter_test.d 


# Each subdirectory must supply rules for building sources it contributes
src/tools/%.o: ../src/tools/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


