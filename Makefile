################################################################################
# Automatically-generated file. Do not edit!
################################################################################

#-include ../makefile.init

CC_OPTIMIZE := -O3 -fopenmp -march=native -msse4 -mfpmath=sse -ffast-math -Wall -c -fmessage-length=0 
#CC_OPTIMIZE := -g -O1 -fopenmp -ffast-math -c -fmessage-length=0 
RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include rtengine/subdir.mk
-include simplegui/subdir.mk
-include objects.mk


ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
endif

#-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: rtviewer

# Tool invocations
rtviewer: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++ -g -rdynamic -o "rtviewer" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

plugins:
	@echo 'Building plugins'
	$(MAKE) -f rtengine/filters/Makefile 
	

# Other Targets
clean:
	-$(RM) $(OBJS) rtviewer
	-@echo ' '
	cd rtengine/plugins/ && make clean

post-build:
	-@echo 'install library'
	cd rtengine/plugins/ && make 
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: post-build

#-include ../makefile.targets
