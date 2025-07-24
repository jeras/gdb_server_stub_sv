///////////////////////////////////////////////////////////////////////////////
// GDB to CPU/SoC adapter for NERV CPU
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

// SoC hierarchical paths
`define soc    $root.nerv_tb.soc
`define cpu    $root.nerv_tb.soc.cpu
// next PC TODO
`define pc     $root.nerv_tb.soc.cpu.pc
`define gpr(i) $root.nerv_tb.soc.cpu.regfile[i]
// indexing a byte within a 32-bit memory array
`define mem(i) $root.nerv_tb.soc.mem[((i)-`cpu.RESET_ADDR)/4][8*((i)%4)+:8]

module nerv_gdb #(
  // 8/16/32/64 bit CPU selection
  localparam int unsigned XLEN = 32,
  localparam type         SIZE_T = int unsigned,  // could be longint (RV64), but it results in warnings
  // number of GPR registers
  parameter  int unsigned GNUM = 32,  // GPR number can be 16 for RISC-V E extension (embedded)
  // Unix/TCP socket
  parameter  string       SOCKET = "gdb_server_stub_socket",
  // XML target/registers/memory description
  parameter  string       XML_TARGET    = "",
  parameter  string       XML_REGISTERS = "",
  parameter  string       XML_MEMORY    = "",
  // DEBUG parameters
  parameter  bit          DEBUG_LOG = 1'b1
)(
  // system signals
  input  logic clk,  // clock
  output logic rst   // reset
);

  import socket_dpi_pkg::*;
  import gdb_server_stub_pkg::*;

///////////////////////////////////////////////////////////////////////////////
// adapter class (extends the gdb_server_stub_socket class)
///////////////////////////////////////////////////////////////////////////////

  class gdb_server_stub_adapter #(
    // 8/16/32/64 bit CPU selection
    parameter  int unsigned XLEN = 32,  // register/address/data width
    parameter  int unsigned ILEN = 32,  // maximum instruction width
    parameter  type         SIZE_T = int unsigned,  // could be longint (RV64), but it results in warnings
    // number of all registers
    parameter  int unsigned GPRN =   32,       // GPR number
    parameter  int unsigned CSRN = 4096,       // CSR number  TODO: should be a list of indexes
    parameter  int unsigned REGN = GPRN+CSRN,  // combined register number
    // memory map (shadow memory map)
    parameter  int unsigned MEMN = 1,          // memory regions number
    parameter  type         MMAP_T = struct {SIZE_T base; SIZE_T size;},
    parameter  MMAP_T       MMAP [0:MEMN-1] = '{default: '{base: 0, size: 256}},
    // DEBUG parameters
    parameter  bit DEBUG_LOG = 1'b1
  ) extends gdb_server_stub #(
    .XLEN      (XLEN  ),
    .ILEN      (ILEN  ),
    .SIZE_T    (SIZE_T),
    .GPRN      (GPRN  ),
    .CSRN      (CSRN  ),
    .REGN      (REGN  ),
    .MEMN      (MEMN  ),
    .MMAP_T    (MMAP_T),
    .MMAP      (MMAP  ),
    .DEBUG_LOG (DEBUG_LOG)
  );

    // constructor
    function new(
      string socket = ""
    );
      super.new(
        .socket (socket)
      );
    endfunction: new

    /////////////////////////////////////////////
    // reset and step
    /////////////////////////////////////////////

    virtual task dut_reset;
      // go through a reset sequence
      rst = 1'b1;
      repeat (4) @(posedge clk);
      rst <= 1'b0;
      repeat (1) @(posedge clk);
    endtask: dut_reset

    virtual task dut_step (
      ref retired_t ret
    );
      // wait for an instruction to retire
      do begin
        @(posedge clk);
      end while (~`cpu.next_rvfi_valid);

      // TODO: first make synchorous samples into temporary signals
      // then populate the structure
      ret.ifu.adr <= `cpu.npc;
      ret.ifu.ins <= `cpu.insn;
      ret.ifu.ill <= 1'b0;
      ret.gpr = new[1];
      ret.gpr[0].idx <= `cpu.next_wr & `cpu.wr_rd;
      ret.gpr[0].val <= `cpu.next_rd;
      ret.lsu.adr <= `cpu.dmem_addr;
      if (`cpu.dmem_valid & `cpu.mem_wr_enable) begin
        int unsigned     siz = 2**`cpu.insn_funct3[1:0];
        logic [XLEN-1:0] val = `cpu.mem_wr_data;
        ret.lsu.val = new[siz];
        ret.lsu.val <= {<<8{val}};
      end
    endtask: dut_step


    // instruction retirement history log entry
    typedef struct {
      // instruction fetch unit
      struct {
        bit [XLEN-1:0] adr;  // instruction address
        bit [ILEN-1:0] ins;  // instruction
        bit            ill;  // illegal instruction
      } ifu;
      // GPRs (NOTE: 4-state values)
      struct {
        bit      [5-1:0] idx;  // GPR index
        logic [XLEN-1:0] old;  // old value
        logic [XLEN-1:0] val;  // new value
      } gpr [];
      // CSRs (NOTE: 2-state values)
      struct {
        bit     [12-1:0] idx;  //
        bit   [XLEN-1:0] rdt;  // old value
        bit   [XLEN-1:0] wdt;  // new value after write
      } csr [];
      // memory (access size is encoded in array size)
      struct {
        bit   [XLEN-1:0] adr;     // data address
        array_t          old [];  // read  data (old data)
        array_t          val [];  // write data (new data)
      } lsu;
    } retired_t;




    virtual task dut_jump (
      input  SIZE_T adr
    );
      $error("step/continue address jump is not supported");
    endtask: dut_jump

    /////////////////////////////////////////////
    // register/memory access function overrides
    /////////////////////////////////////////////

    // TODO: for a multi memory and cache setup, there should be a decoder here

    virtual function bit [XLEN-1:0] dut_reg_read (
      input  int unsigned idx
    );
      if (idx<GNUM) begin
        dut_reg_read = `gpr(idx);
      end else begin
        dut_reg_read = `pc;
      end
    endfunction: dut_reg_read

    virtual function void dut_reg_write (
      input  int unsigned   idx,
      input  bit [XLEN-1:0] dat
    );
      if (idx<GNUM) begin
        `gpr(idx) = dat;
      end else begin
        `pc = dat;
      end
    endfunction: dut_reg_write

    virtual function automatic byte dut_mem_read (
      input  SIZE_T adr
    );
      dut_mem_read = `mem(adr);
    endfunction: dut_mem_read

    virtual function automatic bit dut_mem_write (
      input  SIZE_T adr,
      input  byte   dat
    );
      `mem(adr) = dat;
//      $display("DBG: mem[%08x] = %02x", adr, `mem(adr));
      return(0);
    endfunction: dut_mem_write

  endclass: gdb_server_stub_adapter

///////////////////////////////////////////////////////////////////////////////
// main loop
///////////////////////////////////////////////////////////////////////////////

  // create GDB socket object
  gdb_server_stub_adapter gdb;

  initial
  begin: main_initial
    gdb = new(SOCKET);
    // start state machine
    gdb.gdb_fsm();
  end: main_initial

  final
  begin
    // stop server (close socket)
    void'(socket_close);
  end

endmodule: nerv_gdb
