################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../rtengine/amaze_demosaic_RT.cc \
../rtengine/dcraw.cc \
../rtengine/demosaic_jrp.cc \
../rtengine/fast_demo.cc \
../rtengine/image.cc \
../rtengine/ImageRaw.cc \
../rtengine/Lab_denoise.cc \
../rtengine/myfile.cc \
../rtengine/rawimage.cc \
../rtengine/rawimagesource.cc \
../rtengine/usm.cc \
../rtengine/improps.cc \


OBJS += \
./rtengine/amaze_demosaic_RT.o \
./rtengine/dcraw.o \
./rtengine/demosaic_jrp.o \
./rtengine/fast_demo.o \
./rtengine/image.o \
./rtengine/ImageRaw.o \
./rtengine/Lab_denoise.o \
./rtengine/myfile.o \
./rtengine/rawimage.o \
./rtengine/rawimagesource.o \
./rtengine/usm.o \
./rtengine/improps.o \
./rtengine/utils/LUT.o \

CC_DEPS += 


# Each subdirectory must supply rules for building sources it contributes

rtengine/%.o: ./rtengine/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I /usr/include/glib-2.0/ -O3 -fopenmp -march=native -msse4 -mfpmath=sse -ffast-math -Wall -c -fmessage-length=0 -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '