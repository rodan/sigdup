################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
driverlib/MSP430FR5xx_6xx/%.o: ../driverlib/MSP430FR5xx_6xx/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: GNU Compiler'
	"C:/ti/ccs1000/ccs/tools/compiler/msp430-gcc-9.2.0.50_win64/bin/msp430-elf-gcc-9.2.0.exe" -c -mmcu=msp430fr5994 -mhwmult=f5series -I"C:/ti/ccs1000/ccs/ccs_base/msp430/include_gcc" -I"W:/ti/msp430fr5994/sigdup/firmware" -I"W:/ti/msp430fr5994/sigdup/firmware/driverlib/MSP430FR5xx_6xx" -I"W:/ti/msp430fr5994/sigdup/firmware/glue" -I"C:/ti/ccs1000/ccs/tools/compiler/msp430-gcc-9.2.0.50_win64/msp430-elf/include" -Og -g -gdwarf-3 -gstrict-dwarf -Wall -mlarge -mcode-region=none -mdata-region=lower -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


