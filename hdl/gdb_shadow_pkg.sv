///////////////////////////////////////////////////////////////////////////////
// GDB shadow (DUT shadow copy) package
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

package gdb_shadow_pkg;

    // byte dynamic array type for casting to/from string
    typedef byte array_t [];

///////////////////////////////////////////////////////////////////////////////
// GDB shadow class
///////////////////////////////////////////////////////////////////////////////

    class gdb_shadow #(
        // 32/64 bit CPU selection
        parameter  int unsigned XLEN = 32,  // register/address/data width
        // choice between F/D/Q floating point support
//      parameter  int unsigned FLEN = 32,  // floating point register width (use 0 to disable FPU registers)
        // number of all registers
        parameter  int unsigned GPRN =   32,       // GPR number (use 16 for E extension)
        parameter  int unsigned CSRN = 4096,       // CSR number  TODO: should be a list of indexes
        // memory map (shadow memory map)
        parameter  type         SIZE_T = int unsigned,  // could be longint (RV64), but it results in warnings
        parameter  int unsigned MEMN = 1,          // memory regions number
        parameter  type         MMAP_T = struct {SIZE_T base; SIZE_T size;},
        parameter  MMAP_T       MMAP [0:MEMN-1] = '{default: '{base: 0, size: 256}}
    );

        // dictionary of array_t
        typedef array_t dictionary_t [SIZE_T];

///////////////////////////////////////////////////////////////////////////////
// retired instruction trace
///////////////////////////////////////////////////////////////////////////////

        // IFU (instruction fetch unit)
        typedef struct {
            bit   [XLEN-1:0] adr;  // instruction address (current PC)
            bit   [XLEN-1:0] pcn;  // next PC
            array_t          rdt;  // read data (current instruction)
            bit              ill;  // illegal instruction (DUT behavior might not be defined)
            // TODO: add interrupts
        } retired_ifu_t;

        // GPRs (NOTE: 4-state values)
        typedef struct {
            bit      [5-1:0] idx;  // GPR index
            logic [XLEN-1:0] rdt;  // read  data (current destination register value)
            logic [XLEN-1:0] wdt;  // write data (next    destination register value)
        } retired_gpr_t;

        // CSRs (NOTE: 2-state values)
        typedef struct {
            bit     [12-1:0] idx;  // CSR index
            bit   [XLEN-1:0] rdt;  // read  data
            bit   [XLEN-1:0] wdt;  // write data
        } retired_csr_t;

        // LSU (load store unit)
        // (memory/IO access size is encoded in array size)
        typedef struct {
            bit   [XLEN-1:0] adr;  // data address
            array_t          rdt;  // read  data
            array_t          wdt;  // write data
        } retired_lsu_t;

        // instruction retirement history log entry
        typedef struct {
            retired_ifu_t ifu;
            retired_gpr_t gpr [];
            retired_csr_t csr [];
            retired_lsu_t lsu;
        } retired_t;

///////////////////////////////////////////////////////////////////////////////
// shadow state
///////////////////////////////////////////////////////////////////////////////

        // registers
        logic [XLEN-1:0] gpr [0:GPRN-1];  // GPR (general purpose register file)
        bit   [XLEN-1:0] pc;              // PC  (program counter)
//      logic [FLEN-1:0] fpr [0:  32-1];  // FPR (floating point register file)
        logic [XLEN-1:0] csr [0:CSRN-1];  // CSR (configuration status registers)

        // memories
        array_t          mem [0:MEMN-1];  // array of address map regions

        // memory mapped I/O registers (covers address space not covered by memories)
        dictionary_t     i_o;

        // trace queue
        retired_t        trc [$];

        // instruction counter
        SIZE_T           cnt;

///////////////////////////////////////////////////////////////////////////////
// constructor
///////////////////////////////////////////////////////////////////////////////

        // constructor
        function new ();
            // RISC-V specific x0 (zero) initialization
            gpr[0] = '0;
            // initialize array of memory regions
            for (int unsigned i=0; i<$size(MMAP); i++) begin
                mem[i] = new[MMAP[i].size];
            end
            // initialize I/O dictionaries
            i_o.delete();
            // initialize trace queue
            trc.delete();
            // initialize counter (the counter points to no instruction)
            cnt = -1;
        endfunction: new

///////////////////////////////////////////////////////////////////////////////
// register access
///////////////////////////////////////////////////////////////////////////////

        // write register to shadow copy
        function void reg_write (
          input int unsigned   idx,
          input bit [XLEN-1:0] val
        );
            if (idx < GPRN) begin
                gpr[idx] = val;
            end else
            if (idx == GPRN) begin
                pc = val;
            end else
            if (idx > GPRN) begin
                csr[idx-1-GPRN] = val;
            end
        endfunction: reg_write

        // read register from shadow copy
        function logic [XLEN-1:0] reg_read (
          input int unsigned   idx
        );
            if (idx < GPRN) begin
                return(gpr[idx]);
            end else
            if (idx == GPRN) begin
                return(pc);
            end else
            if (idx > GPRN) begin
                return(csr[idx-1-GPRN]);
            end else
            begin
                return('x);
            end
        endfunction: reg_read

///////////////////////////////////////////////////////////////////////////////
// memory access
///////////////////////////////////////////////////////////////////////////////

        // read from shadow memory
        function automatic array_t mem_read (
            SIZE_T adr,
            SIZE_T siz
        );
            array_t tmp = new[siz];
            // reading from an address map block
            for (int unsigned blk=0; blk<$size(MMAP); blk++) begin: map
                if ((adr >= MMAP[blk].base) &&
                    (adr <  MMAP[blk].base + MMAP[blk].size)) begin: slice
                    tmp = {>>{mem[blk] with [adr - MMAP[blk].base +: siz]}};
                    return tmp;
                end: slice
            end: map
            // reading from an unmapped IO region (reads have higher priority)
            // TODO: handle access to nonexistent entries with a warning?
            // TODO: handle access with a size mismatch
            tmp = i_o[adr];
            return tmp;
        endfunction: mem_read

        // write to shadow memory
        function automatic void mem_write (
            SIZE_T  adr,
            array_t dat
        );
            // writing to an address map block
            for (int unsigned blk=0; blk<$size(MMAP); blk++) begin: map
                if ((adr >= MMAP[blk].base) &&
                    (adr <  MMAP[blk].base + MMAP[blk].size)) begin: slice
                    {>>{mem[blk] with [adr - MMAP[blk].base +: dat.size()]}} = dat;
//                    for (int unsigned i=0; i<dat.size(); i++) begin: byt
//                      mem[blk][adr - MMAP[blk].base] = dat[i];
//                    end: byt
                    return;
                end: slice
            end: map
            // writing to an unmapped IO region (reads have higher priority)
            i_o[adr] = dat;
        endfunction: mem_write

///////////////////////////////////////////////////////////////////////////////
// update/record/replay/revert retired instruction to/from DUT shadow state
///////////////////////////////////////////////////////////////////////////////

        // update the shadow with state experienced by retired instruction (GPR/PC are no affected)
        // for example CSR state changes and memory writes by a DMA
        function automatic void update (
            ref retired_t ret
        );
            // IFU/PC
            mem_write(ret.ifu.adr, ret.ifu.rdt);
            pc = ret.ifu.adr;
            // CSR
            for (int unsigned i=0; i<ret.csr.size(); i++) begin: csr_idx
                csr[ret.csr[i].idx] = ret.csr[i].rdt;
            end: csr_idx
            // LSU
            mem_write(ret.lsu.adr, ret.lsu.rdt);
        endfunction: update

        // remember the current shadow state for all shadow state changes to be able to revert later
        function automatic void record (
            ref retired_t ret
        );
            // IFU/PC
            ret.ifu.rdt = mem_read(ret.ifu.adr, $size(ret.ifu.rdt));
            ret.ifu.adr = pc;
            // GPR
            for (int unsigned i=0; i<ret.gpr.size(); i++) begin: gpr_idx
                ret.gpr[i].rdt = gpr[ret.gpr[i].idx];
            end: gpr_idx
            // CSR
            for (int unsigned i=0; i<ret.csr.size(); i++) begin: csr_idx
                ret.csr[i].rdt = csr[ret.csr[i].idx];
            end: csr_idx
            // LSU
            ret.lsu.rdt = mem_read(ret.lsu.adr, $size(ret.lsu.wdt));
        endfunction: record

        function automatic void replay (
            ref retired_t ret
        );
            // PC
            pc = ret.ifu.pcn;
            // GPR remember the old value and apply the new one
            for (int unsigned i=0; i<ret.gpr.size(); i++) begin: gpr_idx
                gpr[ret.gpr[i].idx] = ret.gpr[i].wdt;
            end: gpr_idx
            // CSR remember the read value and apply written value
            for (int unsigned i=0; i<ret.csr.size(); i++) begin: csr_idx
                csr[ret.csr[i].idx] = ret.csr[i].wdt;
            end: csr_idx
            // memory
            mem_write(ret.lsu.adr, ret.lsu.wdt);
        endfunction: replay

        function automatic void revert (
            ref retired_t ret
        );
            // PC
            pc = ret.ifu.adr;
            // GPR remember the old value and apply the new one
            for (int unsigned i=0; i<ret.gpr.size(); i++) begin: gpr_idx
                gpr[ret.gpr[i].idx] = ret.gpr[i].rdt;
            end: gpr_idx
            // CSR remember the read value and apply written value
            for (int unsigned i=0; i<ret.csr.size(); i++) begin: csr_idx
                csr[ret.csr[i].idx] = ret.csr[i].rdt;
            end: csr_idx
            // memory
            mem_write(ret.lsu.adr, ret.lsu.rdt);
        endfunction: revert

///////////////////////////////////////////////////////////////////////////////
// forward/backward steps
///////////////////////////////////////////////////////////////////////////////

        function void forward ();
            $display("DEBUG: FORWARD: trc.size() = %0d, trc[%0d] = %p", trc.size(), cnt, trc[cnt-1]);
            // replay the previous retired instruction to the shadow
            // if there is no previous retired instruction,
            // there is nothing to apply to the shadow
            if (cnt != -1) begin
                replay(trc[cnt]);
            end
            // increment retirement counter
            cnt++;
            // update/record (if not in replay mode)
            if (cnt == trc.size()) begin
                update(trc[cnt]);
                record(trc[cnt]);
            end
        endfunction: forward

        function void backward ();
            $display("DEBUG: BACKWARD-I: trc.size() = %0d, trc[%0d] = %p", trc.size(), cnt, trc[cnt]);
            // revert the previous retired instruction to the shadow
            // if there is no previous retired instruction,
            // there is nothing to apply to the shadow
            if (cnt != 0) begin
                revert(trc[cnt-1]);
            end
            // decrement retirement counter
            cnt--;
            $display("DEBUG: BACKWARD-O: trc.size() = %0d, trc[%0d] = %p", trc.size(), cnt, trc[cnt]);
        endfunction: backward

    endclass: gdb_shadow

endpackage: gdb_shadow_pkg
