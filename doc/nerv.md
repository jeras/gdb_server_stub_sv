# NERV CPU review

This is a collection of thoughts on what could be changed in the NERV CPU,
some of the changes might only make sense to me.

My viewpoint when writing this comments is related to three of my projects:

1. My RISC-V CPU ["R5P DEGU"]().
2. My TCB system bus standard.
3. My [GDB stub](https://github.com/jeras/gdb_server_stub_sv)
   for debugging SW running on a CPU within a HDL simulation.

I started using NERV in my GDB stu project,
because the CPU is simple (contains no synthesis optimizations),
and since the RVFI can be used as a standard layer
for porting the project to other CPU/SoC.

## Separation of CPU and formal code

The RVFI formal interface could be implemented in a separate file
and connected to the CPU using hierarchical paths (`$root/soc/nerv/...`).
This would avoid mixing RTL and formal verification code.
It would also remove the need for Verilog `` `ifdef `` macro code in the CPU port definitions.

I understand, using hierarchical paths is not very common,
but so is the use of Verilog  `` `ifdef `` macros.

For me, this would allow placing the RVFI code in the testbench.

I would have two use cases for the RVFI interface:

1. RISC-V verification with RISCOF.

   RISCOF does not provide a practical approach for identifying
   where during the program execution there was a discrepancy
   between the golden signature and the one produced by the DUT.
   You can diff the two signatures, but getting from the index
   of the different signature line to the the address of the instruction
   writing the signature line is time consuming.
   This mapping process can be simplifier by creating a log of executed instructions
   by both the reference simulator and the DUT.

   Now on the DUT side the RVFI interface aligns the PC value, GPR/CSR writes and load/store data
   for each retired instruction, so they can be written into a execution log file.
   Now (at least for simple CPU architectures) the PC of the reference/DUT discrepancy
   can be found by diffing the reference/DUT execution log files.
   With the test disassembly it is now easier to find the affected instruction
   and the position of the discrepancy in the simulation waveform.

2. GDB stub abstraction layer.

   For my GDB stub the RVFI could provide an abstraction layer or glue logic
   between a CPU pipeline and the GDB stub.
   For the GDB stub to be portable to different CPU architectures,
   it should expect access to SoC state changes (PC value, GPR/CSR writes and load/store data)
   as a single packet (in the same clock cycle),
   so it can map the PC (for HW breakpoints) to an instruction,
   its GPR/CSR state changes and load/store operations (memory changes and HW watchpoits).

   To put it simple, all CPUs implementing RVFI could use the same glue logic to interface to the GDB stub.
   Thus greatly improving the stub portability.

   There are still issues with the GDB stub I did not resolve,
   I could put them aside for a separate discussion,
   but since this is a dump of my thoughts, here they are:

   * Right now I implement GDB access to GPR and memories by using `ref` ports.

     This provides GDB read/write access directly to SoC/CPU RTL/model Verilog arrays
     implementing GPR (`logic [XLEN-1:0] gpr [0:32-1]`) and memory (`logic [8-1:0] mem [0:MEM_SIZE-1]`).
     While this approach is very efficient in therms of HDL simulator resources,
     it breaks the debugger abstraction, which states that the instruction
     at which the debugger stopped and any further instruction had no effect on the system state,
     while all previous instructions are fully retired with state changes propagated through the SoC.

     In practice, the GPR could already be modified by the next instruction in the pipeline,
     and memory changes could still be lingering in write buffers, caches, AXI interconnect FIFOs, ...
     I could handle all the exceptions (snoop the cache, ...) but for a complex system,
     this would be very prone to bugs.

   * The other approach would be to map RVFI events to some kind of shadow copies of GPR/memory.

     While this approach provides a more reliable abstraction,
     it fails to model things like DMA writes to memory, ...

   While writing this down, I figured the common used of GDB also breaks some abstractions,
   the most obvious being real time execution (which is the one my project does not break).
   I might ask on GDB forums, but for now  I just plan to write a document section titled
   [_Leaky abstraction_](https://en.wikipedia.org/wiki/Leaky_abstraction).
     

## Adding `imem_valid` and handshake backpressure `*_ready`

The NERV IFU bus interface lacks any handshake signals.
This results in the following effects:

* IFU appears to start from reset with a fetch from RESET_ADDR-4.
* IFU performs a repeated instruction read during every LOAD instruction.

By adding a backpressure READY signal to IFU/LSU,
the interfaces would have a proper AXI handshake
making them more intuitive to developers with experience with the VALID/READY handshake.

I have not yet done a proper analysis on which between IFU/LSU
should be given priority on the bus while accessing the same memory.
The correct priority would prevent lockups.

My CPU also makes repeated fetches of the same instruction,
but only in case of a stall caused by backpressure on the LSU interface.

TCB (tightly coupled bus) is an extension of the SRAM read/write interface with:

* a READY signal for backpressure,
* while SRAM ready has always a single cycle (1) delay,
  TCB allows the delay to be configurable (0, 1, 2, ...).

## Synchronous/asynchronous reset and `reset_q`

Currently `reset` is implemented as a synchronous signals (common in FPGA).

The code could be modified to support an asynchronous reset.
The distinction between synchronous and asynchronous reset
could be limited to the syntax of `always` statements:

* `always @(posedge clock, posedge reset)` for asynchronous reset,
* `always @(posedge clock)` for synchronous reset.

For some logic the reset condition is `reset || reset_q`
where `reset_q` is delayed by a clock period and `stall`.
This means reset is applied to some registers for more than one clock period
which I find unnecessary.

I think the control logic FSM could be made simpler by rewriting the reset logic
and more practical by using interface backpressure instead of stall.

## Single cycle (CPI=1) execution of LOAD instructions

My CPU is similar to NERV, especially the IFU.
To avoid the added clock cycle for LSU loads
I added a write back (WB stage).
All GPR write back is delayed by one clock period,
so LSU read data never happens in the same cycle as other WB operations.
A bypass is added to avoid hazards.

I do not think this would be important for this CPU,
I'm mentioning it, since it can be added to the CPU without too much effort.

## C instructions and misaligned IFU/LSU accesses

I suspect the current implementation does not have C support
since C instructions support also requires support for misaligned
IFU interface access.

In the spirit of this CPU lacking synthesis optimizations,
the best approach would probably be to translate
16-bit instructions into 32-bit instructions.

The C instructions contain many exceptions resulting in reserved opcodes (and hints).
This would significantly increases the complexity
of properly decoding illegal instructions.

The current NERV LSU lacks support for misaligned accesses.

I wrote a simple [SRAM adapter](https://github.com/jeras/TCB/blob/main/hdl/rtl/lib/tcb_lib_misaligned_memory_controller.sv)
which handles misaligned access transparently.

I know LSU misaligned accesses are not very important,
I mention them, since I spent some time studying and implementing them.

## RISCOF and RVFI

I have ported 2 of my CPUs and I helped a newcomer port RISCOF on Reddit.
Still RISCOF lacks simple examples.

For debugging discrepancies between the reference simulator and the DUT
I enabled logging in spike and created a log of retired instructions for the DUT.
With this finding where in the code the simulator and the DUT diverge
requires only performing a diff between the two logs.

A CPU with an existing RVFI makes it simple to log retired instructions.