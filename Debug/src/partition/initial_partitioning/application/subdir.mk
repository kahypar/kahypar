################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/partition/initial_partitioning/application/HypergraphBenchmarkTests.cc \
../src/partition/initial_partitioning/application/InitialPartitioning.cc 

OBJS += \
./src/partition/initial_partitioning/application/HypergraphBenchmarkTests.o \
./src/partition/initial_partitioning/application/InitialPartitioning.o 

CC_DEPS += \
./src/partition/initial_partitioning/application/HypergraphBenchmarkTests.d \
./src/partition/initial_partitioning/application/InitialPartitioning.d 


# Each subdirectory must supply rules for building sources it contributes
src/partition/initial_partitioning/application/%.o: ../src/partition/initial_partitioning/application/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


