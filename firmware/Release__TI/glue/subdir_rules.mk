################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
glue/%.obj: ../glue/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccs1000/ccs/tools/compiler/ti-cgt-msp430_20.2.2.LTS/bin/cl430" -vmspx --code_model=small --data_model=small -O2 --use_hw_mpy=F5 --include_path="C:/ti/ccs1000/ccs/ccs_base/msp430/include" --include_path="W:/ti/msp430fr5994/sigdup/firmware/driverlib/MSP430FR5xx_6xx" --include_path="W:/ti/msp430fr5994/sigdup/firmware" --include_path="W:/ti/msp430fr5994/sigdup/firmware/glue" --include_path="C:/ti/ccs1000/ccs/tools/compiler/ti-cgt-msp430_20.2.2.LTS/include" --advice:power="all" --advice:hw_config="all" --define=__MSP430FR5994__ --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --large_memory_model --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="glue/$(basename $(<F)).d_raw" --obj_directory="glue" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


