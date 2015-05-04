################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../gmock/src/gmock-all.cc \
../gmock/src/gmock-cardinalities.cc \
../gmock/src/gmock-internal-utils.cc \
../gmock/src/gmock-matchers.cc \
../gmock/src/gmock-spec-builders.cc \
../gmock/src/gmock.cc \
../gmock/src/gmock_main.cc 

OBJS += \
./gmock/src/gmock-all.o \
./gmock/src/gmock-cardinalities.o \
./gmock/src/gmock-internal-utils.o \
./gmock/src/gmock-matchers.o \
./gmock/src/gmock-spec-builders.o \
./gmock/src/gmock.o \
./gmock/src/gmock_main.o 

CC_DEPS += \
./gmock/src/gmock-all.d \
./gmock/src/gmock-cardinalities.d \
./gmock/src/gmock-internal-utils.d \
./gmock/src/gmock-matchers.d \
./gmock/src/gmock-spec-builders.d \
./gmock/src/gmock.d \
./gmock/src/gmock_main.d 


# Each subdirectory must supply rules for building sources it contributes
gmock/src/%.o: ../gmock/src/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


