################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../rtengine/rawtools/amaze_demosaic_RT.cc \
../rtengine/rawtools/dcraw.cc \
../rtengine/rawtools/demosaic_jrp.cc \
../rtengine/rawtools/fast_demo.cc \
../rtengine/rawtools/myfile.cc \
../rtengine/rawtools/rawimage.cc \
../rtengine/rawtools/rawimagesource.cc \
../rtengine/rawtools/ImageRaw.cc \
../rtengine/imageformats/image.cc \
../rtengine/filters/filtermodule.cc \
../rtengine/filters/RGB_denoise.cc \
../rtengine/filters/Lab_denoise.cc \
../rtengine/filters/YCrCb_denoise.cc \
../rtengine/filters/usm.cc \
../rtengine/filters/colorspace.cc \
../rtengine/processing/processfilters.cc \
../rtengine/processing/improps.cc \


OBJS += \
./rtengine/rawtools/amaze_demosaic_RT.o \
./rtengine/rawtools/dcraw.o \
./rtengine/rawtools/demosaic_jrp.o \
./rtengine/rawtools/fast_demo.o \
./rtengine/rawtools/myfile.o \
./rtengine/rawtools/rawimage.o \
./rtengine/rawtools/rawimagesource.o \
./rtengine/rawtools/ImageRaw.o \
./rtengine/imageformats/image.o \
./rtengine/filters/filtermodule.o \
./rtengine/filters/RGB_denoise.o \
./rtengine/filters/Lab_denoise.o \
./rtengine/filters/YCrCb_denoise.o \
./rtengine/filters/usm.o \
./rtengine/filters/colorspace.o \
./rtengine/processing/processfilters.o \
./rtengine/processing/improps.o \
./rtengine/utils/LUT.o \

CC_DEPS += 


# Each subdirectory must supply rules for building sources it contributes

rtengine/%.o: ./rtengine/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ ${CC_OPTIMIZE} -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '