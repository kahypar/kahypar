################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../gmock/test/gmock-actions_test.cc \
../gmock/test/gmock-cardinalities_test.cc \
../gmock/test/gmock-generated-actions_test.cc \
../gmock/test/gmock-generated-function-mockers_test.cc \
../gmock/test/gmock-generated-internal-utils_test.cc \
../gmock/test/gmock-generated-matchers_test.cc \
../gmock/test/gmock-internal-utils_test.cc \
../gmock/test/gmock-matchers_test.cc \
../gmock/test/gmock-more-actions_test.cc \
../gmock/test/gmock-nice-strict_test.cc \
../gmock/test/gmock-port_test.cc \
../gmock/test/gmock-spec-builders_test.cc \
../gmock/test/gmock_all_test.cc \
../gmock/test/gmock_ex_test.cc \
../gmock/test/gmock_leak_test_.cc \
../gmock/test/gmock_link2_test.cc \
../gmock/test/gmock_link_test.cc \
../gmock/test/gmock_output_test_.cc \
../gmock/test/gmock_stress_test.cc \
../gmock/test/gmock_test.cc 

OBJS += \
./gmock/test/gmock-actions_test.o \
./gmock/test/gmock-cardinalities_test.o \
./gmock/test/gmock-generated-actions_test.o \
./gmock/test/gmock-generated-function-mockers_test.o \
./gmock/test/gmock-generated-internal-utils_test.o \
./gmock/test/gmock-generated-matchers_test.o \
./gmock/test/gmock-internal-utils_test.o \
./gmock/test/gmock-matchers_test.o \
./gmock/test/gmock-more-actions_test.o \
./gmock/test/gmock-nice-strict_test.o \
./gmock/test/gmock-port_test.o \
./gmock/test/gmock-spec-builders_test.o \
./gmock/test/gmock_all_test.o \
./gmock/test/gmock_ex_test.o \
./gmock/test/gmock_leak_test_.o \
./gmock/test/gmock_link2_test.o \
./gmock/test/gmock_link_test.o \
./gmock/test/gmock_output_test_.o \
./gmock/test/gmock_stress_test.o \
./gmock/test/gmock_test.o 

CC_DEPS += \
./gmock/test/gmock-actions_test.d \
./gmock/test/gmock-cardinalities_test.d \
./gmock/test/gmock-generated-actions_test.d \
./gmock/test/gmock-generated-function-mockers_test.d \
./gmock/test/gmock-generated-internal-utils_test.d \
./gmock/test/gmock-generated-matchers_test.d \
./gmock/test/gmock-internal-utils_test.d \
./gmock/test/gmock-matchers_test.d \
./gmock/test/gmock-more-actions_test.d \
./gmock/test/gmock-nice-strict_test.d \
./gmock/test/gmock-port_test.d \
./gmock/test/gmock-spec-builders_test.d \
./gmock/test/gmock_all_test.d \
./gmock/test/gmock_ex_test.d \
./gmock/test/gmock_leak_test_.d \
./gmock/test/gmock_link2_test.d \
./gmock/test/gmock_link_test.d \
./gmock/test/gmock_output_test_.d \
./gmock/test/gmock_stress_test.d \
./gmock/test/gmock_test.d 


# Each subdirectory must supply rules for building sources it contributes
gmock/test/%.o: ../gmock/test/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


