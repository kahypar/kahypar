################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/playground/hmetis_lib_test.cc \
../src/playground/pq_test.cc 

OBJS += \
./src/playground/hmetis_lib_test.o \
./src/playground/pq_test.o 

CC_DEPS += \
./src/playground/hmetis_lib_test.d \
./src/playground/pq_test.d 


# Each subdirectory must supply rules for building sources it contributes
src/playground/%.o: ../src/playground/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


