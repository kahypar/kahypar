################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/partition/refinement/hyperedge_fm_refiner_test.cc \
../src/partition/refinement/max_gain_node_k_way_fm_refiner_test.cc \
../src/partition/refinement/two_way_fm_refiner_test.cc 

OBJS += \
./src/partition/refinement/hyperedge_fm_refiner_test.o \
./src/partition/refinement/max_gain_node_k_way_fm_refiner_test.o \
./src/partition/refinement/two_way_fm_refiner_test.o 

CC_DEPS += \
./src/partition/refinement/hyperedge_fm_refiner_test.d \
./src/partition/refinement/max_gain_node_k_way_fm_refiner_test.d \
./src/partition/refinement/two_way_fm_refiner_test.d 


# Each subdirectory must supply rules for building sources it contributes
src/partition/refinement/%.o: ../src/partition/refinement/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


