# MVM (Mov Virtual Machine) 

MVM (Mov Virtual Machine) is a compile/runtime environment for C programs 

It works with ELF/x86 binary files, and requires gcc/ld and a few additional
standard programs offered by Posix systems (e.g. objdump and awk)

It allows instrumenting any memory load/store instruction

Makefile reports how to exploit via a compilation flag a default C-based 
instrumentation function that gets executed right before the instrumented 
memory load/store

Makefile also indicates how to configure the instrumentation process in terms of 
- directory where the application resides 
- C modules where instrumentation needs to be applied
- specific functions in those C modules that need to have load/store 
  instructions instrumented

The user can exploit the user_defined(...) function located in patches/patches.c
for instrumenting any load/store by inserting whatever binary instructions which 
get executed right after the instrumented load/store instruction

The github repo of this projects currently delivers an example of user-defined 
instrumentation of store instructions that simply reports the stored value in an 
additional memory location in the address space, at distance 2^{21} from the 
original (heap located) memory cell where the store takes place. 
This type of instrumentation has been explited in the "PARSIR ubiquitous" 
configuration of the PARSIR package for NUMA machines
To run it just type "make ; ./application/prog" without any change to the Makefile
delivered with MVM

As unique note, Makefile just reports the following message: 
PLEASE for any compiling error run 'make restore-files'



