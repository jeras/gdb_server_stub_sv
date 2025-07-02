///////////////////////////////////////////////////////////////////////////////
// GDB server stub testbench
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

module gdb_server_stub_tb #(
  // 8/16/32/64 bit CPU selection
  parameter  int unsigned XLEN = 32,
  parameter  type         SIZE_T = int unsigned,  // could be longint (RV64), but it results in warnings
  // Unix/TCP socket
  parameter  string       SOCKET = "gdb_server_stub_socket",
  // XML target description
  parameter  string       XML_TARGET = "",     // TODO
  // registers
  parameter  int unsigned GLEN = 32,           // GPR number can be 16 for RISC-V E extension (embedded)
  parameter  string       XML_REGISTERS = "",  // TODO
  // memory
  parameter  string       XML_MEMORY = "",     // TODO
  parameter  SIZE_T       MLEN = 8,            // memory unit width byte/half/word/double (8-bit byte by default)
  parameter  SIZE_T       MSIZ = 2**16,        // memory size
  parameter  SIZE_T       MBGN = 0,            // memory beginning
  parameter  SIZE_T       MEND = MSIZ-1,       // memory end
  // DEBUG parameters
  parameter  bit DEBUG_LOG = 1'b1
);

  import socket_dpi_pkg::*;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

  localparam logic [XLEN-1:0] PC0 = 32'h8000_0000;

///////////////////////////////////////////////////////////////////////////////
// local signals
///////////////////////////////////////////////////////////////////////////////

  // system signals
  logic clk = 1'b1;  // clock
  logic rst;  // reset

  // IFU interface (instruction fetch unit)
  logic            ifu_trn;  // transfer
  logic [XLEN-1:0] ifu_adr;  // address
  // LSU interface (load/store unit)
  logic            lsu_trn;  // transfer
  logic            lsu_wen;  // write enable
  logic [XLEN-1:0] lsu_adr;  // address
  logic    [2-1:0] lsu_siz;  // address

///////////////////////////////////////////////////////////////////////////////
// main loop
///////////////////////////////////////////////////////////////////////////////

  // clock
  always #(20ns/2) clk = ~clk;

  // reset
  initial
  begin: process_reset
//    $display("io.status.name=%s", io.status().name());
    /* verilator lint_off INITIALDLY */
    repeat (4) @(posedge clk);
    // synchronous reset release
//    rst <= 1'b0;
    repeat (1000) @(posedge clk);
    $display("ERROR: reached simulation timeout!");
    repeat (4) @(posedge clk);
//    $finish();
    /* verilator lint_on INITIALDLY */
  end: process_reset

///////////////////////////////////////////////////////////////////////////////
// CPU DUT
///////////////////////////////////////////////////////////////////////////////

  // GPR
  logic [XLEN-1:0] gpr [0:GLEN-1];
  // PC
  logic [XLEN-1:0] pc;

  // memory
  logic [8-1:0] mem [0:MSIZ-1];

  always @(posedge clk, posedge rst)
  if (rst) begin
    pc <= PC0;
  end else begin
    pc <= pc+4;
//    $display("DBG: mem[%08h] = %p", pc, mem[pc[$clog2(MEM_SIZ):0]+:4]);
  end

  // IFU
  assign ifu_trn = ~rst;
  assign ifu_adr = pc;

  // LSU
  assign lsu_trn = 1'b0;

///////////////////////////////////////////////////////////////////////////////
// debugger stub
///////////////////////////////////////////////////////////////////////////////

  gdb_server_stub #(
    // 8/16/32/64 bit CPU selection
    .XLEN          (XLEN  ),
    .SIZE_T        (SIZE_T),
    // Unix/TCP socket
    .SOCKET        (SOCKET),
    // XML target description
    .XML_TARGET    (XML_TARGET),
    // registers
    .GLEN          (GLEN),
    .XML_REGISTERS (XML_REGISTERS),
    // memory
    .XML_MEMORY    (XML_MEMORY),
    .MLEN          (MLEN),
    .MBGN          (PC0),
    .MEND          (PC0+MSIZ-1),
    // DEBUG parameters
    .DEBUG_LOG     (DEBUG_LOG)
  ) stub (
    // system signals
    .clk     (clk),
    .rst     (rst),
    // registers
    .gpr     (gpr),
    .pc      (pc ),
    // memories
    .mem     (mem),
    // IFU interface (instruction fetch unit)
    .ifu_trn (ifu_trn),
    .ifu_adr (ifu_adr),
    // LSU interface (load/store unit)
    .lsu_trn (lsu_trn),
    .lsu_wen (lsu_wen),
    .lsu_adr (lsu_adr),
    .lsu_siz (lsu_siz)
  );

endmodule: gdb_server_stub_tb
