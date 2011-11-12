################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../simplegui/DesktopTest.cc \
../simplegui/X11viewport.cc \

OBJS += \
./simplegui/DesktopTest.o \
./simplegui/X11viewport.o \

CC_DEPS += 


# Each subdirectory must supply rules for building sources it contributes
# Each subdirectory must supply rules for building sources it contributes

simplegui/%.o: ./simplegui/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -fopenmp -march=native -msse4 -mfpmath=sse -ffast-math -Wall -c -fmessage-length=0 -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
