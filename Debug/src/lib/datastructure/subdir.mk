################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/lib/datastructure/hypergraph_test.cc \
../src/lib/datastructure/kway_priority_queue_test.cc \
../src/lib/datastructure/priority_queue_test.cc 

OBJS += \
./src/lib/datastructure/hypergraph_test.o \
./src/lib/datastructure/kway_priority_queue_test.o \
./src/lib/datastructure/priority_queue_test.o 

CC_DEPS += \
./src/lib/datastructure/hypergraph_test.d \
./src/lib/datastructure/kway_priority_queue_test.d \
./src/lib/datastructure/priority_queue_test.d 


# Each subdirectory must supply rules for building sources it contributes
src/lib/datastructure/%.o: ../src/lib/datastructure/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


