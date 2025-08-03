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
  localparam int unsigned ILEN = 32,
  localparam type         SIZE_T = int unsigned,  // could be longint (RV64), but it results in warnings
  // number of GPR registers
  parameter  int unsigned GNUM = 32,  // GPR number can be 16 for RISC-V E extension (embedded)
  // Unix/TCP socket
  parameter  string       SOCKET = "gdb_server_stub_socket",
  parameter  shortint unsigned PORT = 0,
  // XML target/registers/memory description
  parameter  string       XML_TARGET    = "",
  parameter  string       XML_REGISTERS = "",
  parameter  string       XML_MEMORY    = "",
  // DEBUG parameters
  parameter  bit          REMOTE_LOG = 1'b1
)(
  // system signals
  input  logic clk,  // clock
  output logic rst   // reset
);

  import socket_dpi_pkg::*;
  import gdb_server_stub_pkg::*;

  event sample_step;

///////////////////////////////////////////////////////////////////////////////
// DUT interface
///////////////////////////////////////////////////////////////////////////////

  logic            ret_ena;  // retire enable

  logic [XLEN-1:0] ifu_adr;  // IFU instruction address
  logic [XLEN-1:0] ifu_pcn;  // IFU PC next
  logic [ILEN-1:0] ifu_rdt;  // IFU instruction
  logic            ifu_ill;  // IFU illegal instruction

  logic            gpr_wen;  // GPR destination write enable
  logic    [5-1:0] gpr_idx;  // GPR destination index
  logic [XLEN-1:0] gpr_wdt;  // GPR destination value

  logic            csr_ena;  // CSR enable
  logic   [12-1:0] csr_idx;  // CSR index
  logic [XLEN-1:0] csr_rdt;  // CSR old value
  logic [XLEN-1:0] csr_wdt;  // CSR new value after write

  logic            lsu_ena;  // LSU enable
  logic    [2-1:0] lsu_siz;  // LSU logarithmic size
  logic [XLEN-1:0] lsu_adr;  // LSU address
  logic [XLEN-1:0] lsu_wdt;  // LSU data
  logic [XLEN-1:0] lsu_rdt;  // LSU data

  assign ret_ena = (`cpu.cycle_insn && !`cpu.mem_rd_enable) || `cpu.cycle_trap || `cpu.cycle_late_wr;

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
//    parameter  int unsigned CSRN = 4096,       // CSR number  TODO: should be a list of indexes
    parameter  int unsigned CSRN = 0,       // CSR number  TODO: should be a list of indexes
    parameter  int unsigned REGN = GPRN+1+CSRN,  // combined register number
    // memory map (shadow memory map)
    parameter  int unsigned MEMN = 1,          // memory regions number
    parameter  type         MMAP_T = struct {SIZE_T base; SIZE_T size;},
    parameter  MMAP_T       MMAP [0:MEMN-1] = '{default: '{base: 0, size: 256}},
    // DEBUG parameters
    parameter  bit REMOTE_LOG = 1'b1
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
    .REMOTE_LOG (REMOTE_LOG)
  );

    // constructor
    function new(
      input string socket = "",
      input shortint unsigned port = 0
    );
      super.new(
        .socket (socket),
        .port   (port)
      );
    endfunction: new

    /////////////////////////////////////////////
    // reset and step
    /////////////////////////////////////////////

    virtual task dut_reset_assert;
      rst = 1'b1;
      repeat (4) @(posedge clk);
    endtask: dut_reset_assert

    virtual task dut_reset_release;
      rst <= 1'b0;
      repeat (1) @(posedge clk);
    endtask: dut_reset_release

    virtual task dut_step (
      ref retired_t ret
    );
      // wait for an instruction to retire
      do begin
        @(posedge clk);
      end while (~ret_ena);

      // synchronous sampling
      ifu_adr <= `cpu.imem_addr_q;
      ifu_pcn <= `cpu.npc;
      ifu_rdt <= `cpu.insn;
      ifu_ill <= 1'b0;
      gpr_wen <= `cpu.next_wr;
      gpr_idx <= `cpu.wr_rd;
      gpr_wdt <= `cpu.next_rd;
      lsu_ena <= `cpu.dmem_valid & `cpu.mem_wr_enable;
      lsu_adr <= `cpu.dmem_addr;
      lsu_siz <= `cpu.insn_funct3[1:0];
      lsu_wdt <= `cpu.mem_wr_data;

      // TODO: rethink the timing in this code
      @(negedge clk);
      -> sample_step;

      // populate structure
      ret.ifu.adr = ifu_adr;
      ret.ifu.pcn = ifu_pcn;
      ret.ifu.rdt = new[2**2]({<<8{ifu_rdt}});  // TODO: handle different instruction sizes
//    ret.ifu.rdt = new[2**lsu_siz]({<<8{ifu_rdt}});
      ret.ifu.ill = ifu_ill;
      if (gpr_wen) begin
        ret.gpr = new[1];
        ret.gpr[0] = '{
          idx: gpr_idx,
          wdt: gpr_wdt,
          default: 'x
        };
      end
      if (lsu_ena) begin
        ret.lsu.adr = lsu_adr;
        ret.lsu.rdt = new[2**lsu_siz]({<<8{lsu_rdt}});
        ret.lsu.wdt = new[2**lsu_siz]({<<8{lsu_wdt}});
      end
      $display("DEBUG: dut_step: ret = %p", ret);
    endtask: dut_step

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
    gdb = new(SOCKET, PORT);
    // start state machine
    gdb.gdb_fsm();
  end: main_initial

  final
  begin
    // stop server (close socket)
    void'(socket_close);
  end

endmodule: nerv_gdb
