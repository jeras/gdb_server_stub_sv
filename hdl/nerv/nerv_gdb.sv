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
    parameter  int unsigned XLEN = 32,
    parameter  type         SIZE_T = int unsigned,  // could be longint (RV64), but it results in warnings
    // number of all registers
    parameter  int unsigned RNUM = GNUM+1,
    // DEBUG parameters
    parameter  bit DEBUG_LOG = 1'b1
  ) extends gdb_server_stub_socket #(
    .XLEN      (XLEN),
    .SIZE_T    (SIZE_T),
    .RNUM      (RNUM),
    .DEBUG_LOG (DEBUG_LOG)
  );

    // constructor
    function new(
      string socket = ""
    );
      super.new(
        .socket (socket)
      );
      // debugger starts in the reset state
      state = RESET;
    endfunction: new

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

    virtual function automatic bit dut_illegal (
      input  SIZE_T adr
    );
      // TODO: implement proper illegal instruction check
      dut_illegal = $isunknown({`mem(adr+1), `mem(adr+0)}) ? 1'b1 : 1'b0;
//      $display("DBG: dut_illegal[%08x] = %04x", adr, {`mem(adr+1), `mem(adr+0)});
    endfunction: dut_illegal

    virtual function automatic bit dut_break (
      input  SIZE_T adr
    );
      // TODO: implement proper illegal instruction check
//    mem_ebreak = ({                          `mem(adr+1), `mem(adr+0)} == 16'hxxxx) ||
//                 ({`mem(adr+3), `mem(adr+2), `mem(adr+1), `mem(adr+0)} == 32'hxxxxxxxx) ? 1'b1 : 1'b0;
      dut_break = 1'b0;
    endfunction: dut_break

    virtual function automatic bit dut_jump (
      input  SIZE_T adr
    );
      $error("step/continue address jump is not supported");
      return(1);
    endfunction: dut_jump

//    virtual task automatic dut_step (
//  
//    );
//
//    endtask: dut_step

  endclass: gdb_server_stub_adapter

///////////////////////////////////////////////////////////////////////////////
// main loop
///////////////////////////////////////////////////////////////////////////////

  event socket_blocking;
  event socket_nonblocking;

//  task step;
//    do begin
//      @(posedge clk);
//    end while (~(bus_trn & bus_xen));
//  endtask: step

  gdb_server_stub_adapter gdb;

  initial
  begin: main_initial
    static byte ch [] = new[1];
    int status;

    // set RESET
    rst = 1'b1;

    // create GDB socket object
    gdb = new(SOCKET);

    // main loop/FSM
    forever
    begin: main_loop
      case (gdb.state)

        RESET: begin
          // go through a reset sequence
          rst = 1'b1;
          repeat (4) @(posedge clk);
          rst <= 1'b0;
          repeat (1) @(posedge clk);
          // enter trap state
          gdb.state = SIGTRAP;
        end

        CONTINUE: begin
//          $display("DEBUG: %t: continue.", $realtime);
          // non-blocking socket read
          -> socket_nonblocking;
          status = socket_recv(ch, MSG_PEEK | MSG_DONTWAIT);

          // if empty, check for breakpoints/watchpoints and continue
          if (status != 1) begin
            // on clock edge sample system buses
            @(posedge clk);

            // check for illegal instructions and hardware/software breakpoints
            if (~`soc.stall) begin
              gdb.state = gdb.gdb_breakpoint_match(`soc.imem_addr);
            end

            // check for hardware watchpoints
            if (~`soc.stall & `soc.dmem_valid) begin
              gdb.state = gdb.gdb_watchpoint_match(
                `soc.dmem_addr,
                `cpu.mem_wr_enable,
                `cpu.insn_funct3
              );
            end

          // in case of Ctrl+C (character 0x03)
          end else if (ch[0] == SIGQUIT) begin
            // TODO: perhaps step to next instruction
            gdb.state = SIGINT;
            $display("DEBUG: Interrupt SIGQUIT (0x03) (Ctrl+c).");
            // send response
            status = gdb.gdb_stop_reply(gdb.state);

          // parse packet and loop back
          end else begin
            status = gdb.gdb_packet(ch);
          end
        end

        STEP: begin
//          $display("DEBUG: %t: continue.", $realtime);
          // step to the next instruction and trap again
          do begin
            @(posedge clk);
//            $display("DEBUG: %t: continue while loop.", $realtime);
          end while (`soc.stall);
          gdb.state = SIGTRAP;

          // send response
          status = gdb.gdb_stop_reply(gdb.state);
        end

        // SIGILL, SIGTRAP, SIGINT, ...
        default: begin
          // blocking socket read
          -> socket_blocking;
          status = socket_recv(ch, MSG_PEEK);

          // parse packet and loop back
          status = gdb.gdb_packet(ch);
        end
      endcase
    end: main_loop
  end: main_initial

  final
  begin
    // stop server (close socket)
    void'(socket_close);
  end

endmodule: nerv_gdb
