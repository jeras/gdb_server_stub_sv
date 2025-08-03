///////////////////////////////////////////////////////////////////////////////
// GDB server stub with NERV CPU adapter testbench
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

module nerv_tb #(
  // constants used across the design in signal range sizing instead of literals
  localparam int unsigned XLEN = 32,  // register/address/data width
  parameter  int unsigned GNUM = 32,  // GPR number
  // Unix/TCP socket/port
  parameter  string       SOCKET,
  parameter  shortint unsigned PORT = 0
);

  // system signals
  logic clk;  // clock
  logic rst;  // reset

  // LEDS
  wire logic [32-1:0] led;

////////////////////////////////////////////////////////////////////////////////
// RTL SoC instance
////////////////////////////////////////////////////////////////////////////////

  nerv_soc #(
    .RESET_ADDR (32'h 0000_0000),
	  .NUMREGS    (GNUM)
  ) soc (
	  .clock (clk),
	  .reset (rst),
	  .leds  (led)
  );

////////////////////////////////////////////////////////////////////////////////
// GDB stub instance
////////////////////////////////////////////////////////////////////////////////

  nerv_gdb #(
    // number of GPR registers
    .GNUM (GNUM),
    // Unix/TCP socket
    .SOCKET        (SOCKET),
    .PORT          (PORT)
    // XML target/registers/memory description
//  .XML_TARGET    (XML_TARGET),
//  .XML_REGISTERS (XML_REGISTERS),
//  .XML_MEMORY    (XML_MEMORY),
    // DEBUG parameters
//  .REMOTE_LOG     (REMOTE_LOG)
  ) gdb (
    // system signals
    .clk     (clk),
    .rst     (rst)
  );

////////////////////////////////////////////////////////////////////////////////
// test sequence
////////////////////////////////////////////////////////////////////////////////

  // 2*25ns=50ns period is 20MHz frequency
  initial      clk = 1'b1;
  always #25ns clk = ~clk;

endmodule: nerv_tb
