#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "elf_parse.h"


void the_patch (unsigned long, unsigned long) __attribute__((used));

//the_patch(...) is the default function offered by MVM for instrumenting whatever 
//memory load/store instruction
//it can be activated by activating the ASM_PREAMBLE macro in src/_elf_parse.c 
//this function is general purpose and is executed right before the memory load/store
//it passes through C programming hence it has the intrinsic cost of CPU 
//snapshot save/restore to/from the stack when taking control or passing it back
//this function takes the pointers to the instruction metadata and CPU snapshot

void the_patch(unsigned long mem, unsigned long regs){
	instruction_record *instruction = (instruction_record*) mem;
	target_address *target;
	unsigned long A = 0, B = 0;
	unsigned long address;

	AUDIT
	printf("memory access done by the application at instrumented instruction indexed by %d\n",instruction->record_index);	

	if(instruction->effective_operand_address != 0x0){
		printf("__mvm: accessed address is %p - data size is %d access type is %c\n",(void*)instruction->effective_operand_address,instruction->data_size,instruction->type);	
	}
	else{
		target = &(instruction->target);
		//AUDIT
		printf("__mvm: accessing memory according to %lu - %lu - %lu - %lu\n",target->displacement,target->base_index,target->scale_index,target->scale);
		if (target->base_index) memcpy((char*)&A,(char*)(regs + 8*(target->base_index-1)),8);
		if (target->scale_index) memcpy((char*)&B,(char*)(regs + 8*(target->scale_index-1)),8);
		address = (unsigned long)((long)target->displacement + (long)A + (long)((long)B * (long)target->scale));
		printf("__mvm: accessed address is %p - data size is %d - access type is %c\n",(void*)address,instruction->data_size,instruction->type);
	}
	fflush(stdout);

	return;
}



//used_defined(...) is the real body of the user-defined instrumentation process, all the stuff you put here represents the actual execution path of an instrumented instruction
//given that you have the exact representation of the instruction to be instrumented, you can produce the
//block of ASM level instructions to be really used for istrumentation
//clearly any memory/register side effect is under the responsibility of the patch programme
//the instrumentation instructions whill be executed right after the original instruction to be instrumented

#define buffer user_defined_buffer//this avoids compile-time replication of the buffer symbol 
char buffer[1024];
//in this function you get the pointer to the metedata representaion of the instruction to be instrumented
//and the pointer to the buffer where the patch (namely the instrumenting instructions) can be placed
//simply eturning form this function with no management of the pointed areas menas that you are skipping
//the instrumentatn of this instruction

void user_defined(instruction_record * actual_instruction, patch * actual_patch){

	int fd;
	int ret;
	int i;

	//here is stuff used for instrumenting applications in "PARSIR ubiquitous" 
	//it replicates memory updates that are executed on malloc-ed/mmap-ed 
	//memory areas at a given distance which is here set to 2^{21}
	int offset = 0x200000;
        char * offset_string = "0x200000";
        char * aux;


	//check if the instructon is a non RIP-relative store 
	//if it is, it can be skipped, otherwise it needs ot be instrumented
	if(actual_instruction->rip_relative == 'n' && actual_instruction->type == 's'){ 

                        fd = open(user_defined_temp_file, O_CREAT|O_TRUNC|O_RDWR,0666);
                        if (fd == -1){
                                printf("%s: error opening temp file %s\n",VM_NAME,user_defined_temp_file);
                                fflush(stdout);
				exit(EXIT_FAILURE);
                        }

			//check if the instruction already has an offset 
			//in any case add the offset required by PARSIR ubiquitous for replicating memory updates
                        if(actual_instruction->dest[0] == '('){
                                sprintf(buffer,"%s %s,%s%s\n",actual_instruction->op,actual_instruction->source,offset_string,actual_instruction->dest);
                        }
                        else{
                                
                                sprintf(buffer,"%s",actual_instruction->dest);
                                aux = strtok(buffer,"(");
                                sprintf(buffer,"%s %s,%p%s\n",actual_instruction->op,actual_instruction->source,(void*)(strtol(aux, NULL, 16)+strtol(offset_string, NULL, 16)), (actual_instruction->dest + strlen(aux)));
                        }

                        ret = write(fd,buffer,strlen(buffer));
                        close(fd);

                        actual_instruction->instrumentation_instructions += 1;//we used one more instruction in the instrumentation path

			//generate the binary of the instrumentaton instruction
                        sprintf(buffer," cd %s; gcc %s -c",user_defined_dir,user_defined_temp_file);                        
                        ret = system(buffer);

			//put the binary on a file
                        sprintf(buffer,"cd %s; ./provide_binary %s > final-binary", user_defined_dir, user_defined_temp_obj_file);
                        ret = system(buffer);

			sprintf(buffer,"%s/final-binary",user_defined_dir);

                        fd = open(buffer, O_RDONLY);
                        if (fd == -1){
                                printf("%s: error opening file %s\n",VM_NAME,buffer);
                                fflush(stdout);
				exit(EXIT_FAILURE);
                        }

			//get the binary
                        ret = read(fd,buffer,LINE_SIZE);
                        if (ret == -1){
                                printf("%s: error reading from final-binary file\n",VM_NAME);
                                fflush(stdout);
                                exit(EXIT_FAILURE);
                        }
			if((actual_patch->functional_instr_size + ret) > CODE_BLOCK){
				printf("%s: error instrumentation code too long\n",VM_NAME);
				fflush(stdout);
				exit(EXIT_FAILURE);
			};

			//post the binary in the instrumentation buffer
                        memcpy(actual_patch->functional_instr,buffer,ret);
			//tell that a few more bytes are in the instrumentation buffer
                        actual_patch->functional_instr_size += ret;

                        close(fd);
	}
}
