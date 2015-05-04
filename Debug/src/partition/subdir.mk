################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/partition/Partitioner.cc \
../src/partition/metrics_test.cc \
../src/partition/partitioner_test.cc 

OBJS += \
./src/partition/Partitioner.o \
./src/partition/metrics_test.o \
./src/partition/partitioner_test.o 

CC_DEPS += \
./src/partition/Partitioner.d \
./src/partition/metrics_test.d \
./src/partition/partitioner_test.d 


# Each subdirectory must supply rules for building sources it contributes
src/partition/%.o: ../src/partition/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


