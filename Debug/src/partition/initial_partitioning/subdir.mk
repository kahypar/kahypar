################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/partition/initial_partitioning/initial_partitioner_base_test.cc \
../src/partition/initial_partitioning/initial_partitioner_test.cc 

OBJS += \
./src/partition/initial_partitioning/initial_partitioner_base_test.o \
./src/partition/initial_partitioning/initial_partitioner_test.o 

CC_DEPS += \
./src/partition/initial_partitioning/initial_partitioner_base_test.d \
./src/partition/initial_partitioning/initial_partitioner_test.d 


# Each subdirectory must supply rules for building sources it contributes
src/partition/initial_partitioning/%.o: ../src/partition/initial_partitioning/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


