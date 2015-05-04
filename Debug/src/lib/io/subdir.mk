################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/lib/io/hypergraph_io_test.cc 

OBJS += \
./src/lib/io/hypergraph_io_test.o 

CC_DEPS += \
./src/lib/io/hypergraph_io_test.d 


# Each subdirectory must supply rules for building sources it contributes
src/lib/io/%.o: ../src/lib/io/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


