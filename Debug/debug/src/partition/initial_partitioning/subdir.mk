################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../debug/src/partition/initial_partitioning/initial_partitioner_base_test.cc 

OBJS += \
./debug/src/partition/initial_partitioning/initial_partitioner_base_test.o 

CC_DEPS += \
./debug/src/partition/initial_partitioning/initial_partitioner_base_test.d 


# Each subdirectory must supply rules for building sources it contributes
debug/src/partition/initial_partitioning/%.o: ../debug/src/partition/initial_partitioning/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


