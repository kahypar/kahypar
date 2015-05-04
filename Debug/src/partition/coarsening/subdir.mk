################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/partition/coarsening/full_heavy_edge_coarsener_test.cc \
../src/partition/coarsening/heuristic_heavy_edge_coarsener_test.cc \
../src/partition/coarsening/hyperedge_coarsener_test.cc \
../src/partition/coarsening/lazy_update_heavy_edge_coarsener_test.cc \
../src/partition/coarsening/rater_test.cc 

OBJS += \
./src/partition/coarsening/full_heavy_edge_coarsener_test.o \
./src/partition/coarsening/heuristic_heavy_edge_coarsener_test.o \
./src/partition/coarsening/hyperedge_coarsener_test.o \
./src/partition/coarsening/lazy_update_heavy_edge_coarsener_test.o \
./src/partition/coarsening/rater_test.o 

CC_DEPS += \
./src/partition/coarsening/full_heavy_edge_coarsener_test.d \
./src/partition/coarsening/heuristic_heavy_edge_coarsener_test.d \
./src/partition/coarsening/hyperedge_coarsener_test.d \
./src/partition/coarsening/lazy_update_heavy_edge_coarsener_test.d \
./src/partition/coarsening/rater_test.d 


# Each subdirectory must supply rules for building sources it contributes
src/partition/coarsening/%.o: ../src/partition/coarsening/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


