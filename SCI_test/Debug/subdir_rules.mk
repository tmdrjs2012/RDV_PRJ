################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.asm $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs1040/ccs/tools/compiler/ti-cgt-c2000_20.2.5.LTS/bin/cl2000" -v28 -ml -mt --float_support=fpu32 --include_path="C:/Users/hc.ro/Desktop/MY_PROJECT/DSP/CCS_ing/10202021_CCS/newFile_1018/DSP/practice/SCI_test" --include_path="C:/ti/controlSUITE/device_support/f2833x/v140/DSP2833x_common/include" --include_path="C:/ti/controlSUITE/device_support/f2833x/v140/DSP2833x_headers/include" --include_path="C:/ti/ccs1040/ccs/tools/compiler/ti-cgt-c2000_20.2.5.LTS/include" --advice:performance=all -g --printf_support=full --diag_warning=225 --diag_wrap=off --display_error_number --abi=coffabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs1040/ccs/tools/compiler/ti-cgt-c2000_20.2.5.LTS/bin/cl2000" -v28 -ml -mt --float_support=fpu32 --include_path="C:/Users/hc.ro/Desktop/MY_PROJECT/DSP/CCS_ing/10202021_CCS/newFile_1018/DSP/practice/SCI_test" --include_path="C:/ti/controlSUITE/device_support/f2833x/v140/DSP2833x_common/include" --include_path="C:/ti/controlSUITE/device_support/f2833x/v140/DSP2833x_headers/include" --include_path="C:/ti/ccs1040/ccs/tools/compiler/ti-cgt-c2000_20.2.5.LTS/include" --advice:performance=all -g --printf_support=full --diag_warning=225 --diag_wrap=off --display_error_number --abi=coffabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


