///////////////////////////////////////////////////////////////////////////////
// GDB server stub package (contains packet parser, responder)
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

package gdb_server_stub_pkg;

  import socket_dpi_pkg::*;

  // byte dynamic array type for casting to/from string
  typedef byte array_t [];

  // state
  typedef enum byte {
    // signals
    SIGHUP  = 8'd01,  // Hangup
    SIGINT  = 8'd02,  // Terminal interrupt signal
    SIGQUIT = 8'd03,  // Terminal quit signal
    SIGILL  = 8'd04,  // Illegal instruction
    SIGTRAP = 8'd05,  // Trace/breakpoint trap
    SIGABRT = 8'd06,  // Process abort signal
    SIGEMT  = 8'd07,
    SIGFPE  = 8'd08,  // Erroneous arithmetic operation
    SIGKILL = 8'd09,  // Kill (cannot be caught or ignored)
    SIGBUS  = 8'd10,
    SIGSEGV = 8'd11,  // Invalid memory reference (address decoder error)
    SIGSYS  = 8'd12,
    SIGPIPE = 8'd13,  // Write on a pipe with no one to read it
    SIGALRM = 8'd14,  // Alarm clock
    SIGTERM = 8'd15,  // Termination signal
    // reset
    RESET    = 8'h80,
    // running continuously
    CONTINUE = 8'h81,
    // running step
    STEP     = 8'h82
  } state_t;

  // point type
  typedef enum int unsigned {
    swbreak = 0,  // software breakpoint
    hwbreak = 1,  // hardware breakpoint
    watch   = 2,  // write  watchpoint
    rwatch  = 3,  // read   watchpoint
    awatch  = 4   // access watchpoint
  } ptype_t;

  typedef int unsigned pkind_t;

  typedef struct packed {
    ptype_t ptype;
    pkind_t pkind;
  } point_t;


  virtual class gdb_server_stub_socket #(
    // 8/16/32/64 bit CPU selection
    parameter  int unsigned XLEN = 32,  // register/address/data width
    parameter  int unsigned ILEN = 32,  // maximum instruction width
    parameter  type         SIZE_T = int unsigned,  // could be longint (RV64), but it results in warnings
    // number of all registers
    parameter  int unsigned RNUM = 32+1,
    // DEBUG parameters
    parameter  bit DEBUG_LOG = 1'b1
  );

///////////////////////////////////////////////////////////////////////////////
// constructor
///////////////////////////////////////////////////////////////////////////////

    // acknowledge mode
    bit acknowledge = 1'b1;

    // current state
    state_t state;

    // constructor
    function new (
      string socket = ""
    );
      int status;

      // open character device for R/W
      status = socket_listen(socket);

      // check if device was found
      if (status == 0) begin
        $fatal(0, "Could not open '%s' device node.", socket);
      end else begin
        $info("Connected to '%0s'.", socket);
      end
      void'(socket_accept);
    endfunction: new

///////////////////////////////////////////////////////////////////////////////
// register/memory access function prototypes
///////////////////////////////////////////////////////////////////////////////

    pure virtual function bit [XLEN-1:0] reg_read (
      input  int unsigned   idx
    );

    pure virtual function void reg_write (
      input  int unsigned   idx,
      input  bit [XLEN-1:0] dat
    );

    pure virtual function byte mem_read (
      input  SIZE_T adr
    );

    pure virtual function void mem_write (
      input  SIZE_T adr,
      input  byte   dat
    );

    pure virtual function bit exe_illegal (
      input  SIZE_T adr
    );

    pure virtual function bit exe_ebreak (
      input  SIZE_T adr
    );

    pure virtual function void jump (
      input  SIZE_T adr
    );

///////////////////////////////////////////////////////////////////////////////
// GDB character get/put
///////////////////////////////////////////////////////////////////////////////

  function automatic void gdb_write (string str);
    int status;
    byte buffer [] = new[str.len()](array_t'(str));
    status = socket_send(buffer, 0);
  endfunction: gdb_write

///////////////////////////////////////////////////////////////////////////////
// GDB packet get/send
///////////////////////////////////////////////////////////////////////////////

  function automatic int gdb_get_packet(
    output string pkt,
    input bit    ack = acknowledge
  );
    int status;
    int unsigned len;
    byte   buffer [] = new[512];
    byte   cmd [];
    string str = "";
    byte   checksum = 0;
    string checksum_ref;
    string checksum_str;

    // wait for the start character, ignore the rest
    // TODO: error handling?
    do begin
      status = socket_recv(buffer, 0);
//      $display("DEBUG: gdb_get_packet: buffer = %p", buffer);
      str = {str, string'(buffer)};
      len = str.len();
//      $display("DEBUG: gdb_get_packet: str = %s", str);
    end while (str[len-3] != "#");

    // extract packet data from received string
    pkt = str.substr(1,len-4);
    if (DEBUG_LOG) begin
  //    $display("DEBUG: <= %s", str);
      $display("DEBUG: <- %s", pkt);
    end

    // calculate packet data checksum
    cmd = new[len-4](array_t'(pkt));
    checksum = cmd.sum();

    // Get checksum now
    checksum_ref = str.substr(len-2,len-1);

    // Verify checksum
    checksum_str = $sformatf("%02h", checksum);
    if (checksum_ref != checksum_str) begin
      $error("Bad checksum. Got 0x%s but was expecting: 0x%s for packet '%s'", checksum_ref, checksum_str, pkt);
      if (ack) begin
        // NACK packet
        gdb_write("-");
      end
      return (-1);
    end else begin
      if (ack) begin
        // ACK packet
        gdb_write("+");
      end
      return(0);
    end
  endfunction: gdb_get_packet

  function automatic int gdb_send_packet(
    input string pkt,
    input bit    ack = acknowledge
  );
    int status;
    byte   ch [] = new[1];
    byte   checksum = 0;
    string checksum_str;

    if (DEBUG_LOG) begin
      $display("DEBUG: -> %p", pkt);
    end

    // Send packet start
    gdb_write("$");

    // Send packet data and calculate checksum
    foreach (pkt[i]) begin
      checksum += pkt[i];
      gdb_write(string'(pkt[i]));
    end

    // Send packet end
    gdb_write("#");

    // Send the checksum
    gdb_write($sformatf("%02h", checksum));

    // Check acknowledge
    if (ack) begin
      status = socket_recv(ch, 0);
      if (ch[0] == "+")  return(0);
      else               return(-1);
    end
  endfunction: gdb_send_packet

///////////////////////////////////////////////////////////////////////////////
// GDB state
///////////////////////////////////////////////////////////////////////////////

  // Send a exception packet "T <value>"
  function automatic int gdb_state();
    string pkt;
    int status;

    // read packet
    status = gdb_get_packet(pkt);

    // reply with current state
    status = gdb_stop_reply();
    return(status);
  endfunction: gdb_state

  // Send a exception packet "T <value>"
  function automatic int gdb_stop_reply(
    input byte signal = state
  );
    // reply with signal (current state by default)
    return(gdb_send_packet($sformatf("S%02h", signal)));
  endfunction: gdb_stop_reply

///////////////////////////////////////////////////////////////////////////////
// GDB query
///////////////////////////////////////////////////////////////////////////////

  function automatic bit gdb_qsupported (
    input string pkt
  );
    int status;
    if (pkt.substr(0,10) == "qSupported") begin
      status = gdb_send_packet("");
      return(1'b1);
    end else begin
      return(1'b0);
    end
  endfunction: gdb_qsupported

  function automatic void gdb_query_packet ();
    string pkt;
    int status;

    // read packet
    status = gdb_get_packet(pkt);

    if (gdb_qsupported(pkt)) begin
      return;
    end else begin
      // not supported, send empty response packet
      status = gdb_send_packet("");
    end
  endfunction: gdb_query_packet

///////////////////////////////////////////////////////////////////////////////
// GDB verbose
///////////////////////////////////////////////////////////////////////////////

  function automatic void gdb_verbose_packet ();
    string pkt;
    int status;

    // read packet
    status = gdb_get_packet(pkt);

    // not supported, send empty response packet
    status = gdb_send_packet("");
  endfunction: gdb_verbose_packet

///////////////////////////////////////////////////////////////////////////////
// GDB memory access (hexadecimal)
///////////////////////////////////////////////////////////////////////////////

  function automatic int gdb_mem_read ();
    int code;
    string pkt;
    int status;
    SIZE_T adr;
    SIZE_T len;

    // read packet
    status = gdb_get_packet(pkt);

//    $display("DBG: gdb_mem_read: pkt = %s", pkt);

    // memory address and length
`ifdef VERILATOR
    code = $sscanf(pkt, "m%h,%h", adr, len);
`else
    case (XLEN)
      32: code = $sscanf(pkt, "m%8h,%8h", adr, len);
      64: code = $sscanf(pkt, "m%16h,%16h", adr, len);
    endcase
`endif

//    $display("DBG: gdb_mem_read: adr = %08x, len=%08x", adr, len);

    // read memory
    pkt = {len{"XX"}};
    for (SIZE_T i=0; i<len; i++) begin
      string tmp = "XX";
      tmp = $sformatf("%02h", mem_read(adr+i));
      pkt[i*2+0] = tmp[0];
      pkt[i*2+1] = tmp[1];
    end

//    $display("DBG: gdb_mem_read: pkt = %s", pkt);

    // send response
    status = gdb_send_packet(pkt);

    return(len);
  endfunction: gdb_mem_read

  function automatic int gdb_mem_write ();
    int code;
    string pkt;
    string str;
    int status;
    SIZE_T adr;
    SIZE_T len;
    byte   dat;

    // read packet
    status = gdb_get_packet(pkt);
//    $display("DBG: gdb_mem_write: pkt = %s", pkt);

    // memory address and length
`ifdef VERILATOR
    code = $sscanf(pkt, "M%h,%h:", adr, len);
`else
    case (XLEN)
      32:     code = $sscanf(pkt, "M%8h,%8h:", adr, len);
      64:     code = $sscanf(pkt, "M%16h,%16h:", adr, len);
    endcase
`endif
//    $display("DBG: gdb_mem_write: adr = 'h%08h, len = 'd%0d", adr, len);

    // remove the header from the packet, only data remains
    str = pkt.substr(pkt.len() - 2*len, pkt.len() - 1);
//    $display("DBG: gdb_mem_write: str = %s", str);

    // write memory
    for (SIZE_T i=0; i<len; i++) begin
//      $display("DBG: gdb_mem_write: adr+i = 'h%08h, mem[adr+i] = 'h%02h", adr+i, mem_read(adr+i));
`ifdef VERILATOR
      status = $sscanf(str.substr(i*2, i*2+1), "%2h", dat);
`else
      status = $sscanf(str.substr(i*2, i*2+1), "%h", dat);
`endif
//      $display("DBG: gdb_mem_write: adr+i = 'h%08h, mem[adr+i] = 'h%02h", adr+i, mem_read(adr+i));
      mem_write(adr+i, dat);
    end

    // send response
    status = gdb_send_packet("OK");

    return(len);
  endfunction: gdb_mem_write

///////////////////////////////////////////////////////////////////////////////
// GDB memory access (binary)
///////////////////////////////////////////////////////////////////////////////

  function automatic int gdb_mem_bin_read ();
    int code;
    string pkt;
    int status;
    SIZE_T adr;
    SIZE_T len;

    // read packet
    status = gdb_get_packet(pkt);

    // memory address and length
`ifdef VERILATOR
    code = $sscanf(pkt, "x%h,%h", adr, len);
`else
    case (XLEN)
      32: code = $sscanf(pkt, "x%8h,%8h", adr, len);
      64: code = $sscanf(pkt, "x%16h,%16h", adr, len);
    endcase
`endif

    // read memory
    pkt = {len{8'h00}};
    for (SIZE_T i=0; i<len; i++) begin
      pkt[i] = mem_read(adr+i);
    end

    // send response
    status = gdb_send_packet(pkt);

    return(len);
  endfunction: gdb_mem_bin_read

  function automatic int gdb_mem_bin_write ();
    int code;
    string pkt;
    int status;
    SIZE_T adr;
    SIZE_T len;

    // read packet
    status = gdb_get_packet(pkt);

    // memory address and length
`ifdef VERILATOR
    code = $sscanf(pkt, "X%h,%h:", adr, len);
`else
    case (XLEN)
      32:     code = $sscanf(pkt, "X%8h,%8h:", adr, len);
      64:     code = $sscanf(pkt, "X%16h,%16h:", adr, len);
    endcase
`endif

    // write memory
    for (SIZE_T i=0; i<len; i++) begin
      mem_write(adr+i, pkt[code+i]);
    end

    // send response
    status = gdb_send_packet("OK");

    return(len);
  endfunction: gdb_mem_bin_write

///////////////////////////////////////////////////////////////////////////////
// GDB multiple register access
///////////////////////////////////////////////////////////////////////////////

  // "g" packet
  function automatic int gdb_reg_readall ();
    int status;
    string pkt;
    bit [XLEN-1:0] val;  // 2-state so GDB does not misinterpret 'x

    // read packet
    status = gdb_get_packet(pkt);

    pkt = "";
    for (int unsigned i=0; i<RNUM; i++) begin
      // swap byte order since they are sent LSB first
      val = {<<8{reg_read(i)}};
      case (XLEN)
        32: pkt = {pkt, $sformatf("%08h", val)};
        64: pkt = {pkt, $sformatf("%016h", val)};
      endcase
    end

    // send response
    status = gdb_send_packet(pkt);

    return(0);
  endfunction: gdb_reg_readall

  function automatic int gdb_reg_writeall ();
    string pkt;
    int status;
    int unsigned len = XLEN/8*2;
    bit [XLEN-1:0] val;

    // read packet
    status = gdb_get_packet(pkt);
    // remove command
    pkt = pkt.substr(1, pkt.len()-1);

    // GPR
    for (int unsigned i=0; i<RNUM; i++) begin
      `ifdef VERILATOR
      status = $sscanf(pkt.substr(i*len, i*len+len-1), "%h", val);
`else
      case (XLEN)
        32: status = $sscanf(pkt.substr(i*len, i*len+len-1), "%8h", val);
        64: status = $sscanf(pkt.substr(i*len, i*len+len-1), "%16h", val);
      endcase
`endif
      // swap byte order since they are sent LSB first
      reg_write(i, {<<8{val}});
    end

    // send response
    status = gdb_send_packet("OK");

    return(0);
  endfunction: gdb_reg_writeall

///////////////////////////////////////////////////////////////////////////////
// GDB single register access
///////////////////////////////////////////////////////////////////////////////

  function automatic int gdb_reg_readone ();
    int status;
    string pkt;
    int unsigned idx;
    bit [XLEN-1:0] val;  // 2-state so GDB does not misinterpret 'x

    // read packet
    status = gdb_get_packet(pkt);

    // register index
    status = $sscanf(pkt, "p%h", idx);

    // swap byte order since they are sent LSB first
    val = {<<8{reg_read(idx)}};
    case (XLEN)
      32: pkt = {pkt, $sformatf("%08h", val)};
      64: pkt = {pkt, $sformatf("%016h", val)};
    endcase

    // send response
    status = gdb_send_packet(pkt);

    return(1);
  endfunction: gdb_reg_readone

  function automatic int gdb_reg_writeone ();
    int status;
    string pkt;
    int unsigned idx;
    bit [XLEN-1:0] val;

    // read packet
    status = gdb_get_packet(pkt);

    // register index and value
`ifdef VERILATOR
    status = $sscanf(pkt, "P%h=%h", idx, val);
`else
    case (XLEN)
      32: status = $sscanf(pkt, "P%h=%8h", idx, val);
      64: status = $sscanf(pkt, "P%h=%16h", idx, val);
    endcase
`endif

    // swap byte order since they are sent LSB first
    reg_write(idx, {<<8{val}});
//    case (XLEN)
//      32: $display("DEBUG: GPR[%0d] <= 32'h%08h", idx, val);
//      64: $display("DEBUG: GPR[%0d] <= 64'h%016h", idx, val);
//    endcase

    // send response
    status = gdb_send_packet("OK");

    return(1);
  endfunction: gdb_reg_writeone

///////////////////////////////////////////////////////////////////////////////
// GDB breakpoints/watchpoints
///////////////////////////////////////////////////////////////////////////////

  // associative array for hardware breakpoints/watchpoint
  point_t points [SIZE_T];

  function automatic int gdb_point_remove ();
    int status;
    string pkt;
    ptype_t ptype;
    SIZE_T addr;
    pkind_t pkind;

    // read packet
    status = gdb_get_packet(pkt);

    // breakpoint/watchpoint
`ifdef VERILATOR
    status = $sscanf(pkt, "z%h,%h,%h", ptype, addr, pkind);
`else
    case (XLEN)
      32: status = $sscanf(pkt, "z%h,%8h,%h", ptype, addr, pkind);
      64: status = $sscanf(pkt, "z%h,%16h,%h", ptype, addr, pkind);
    endcase
`endif

    case (ptype)
      swbreak: begin
        // software breakpoints are not supported
        status = gdb_send_packet("");
      end
      default: begin
        // software breakpoints are not supported
        points.delete(addr);
        status = gdb_send_packet("OK");
      end
    endcase

    return(1);
  endfunction: gdb_point_remove

  function automatic int gdb_point_insert ();
    int status;
    string pkt;
    ptype_t ptype;
    SIZE_T addr;
    pkind_t pkind;

    // read packet
    status = gdb_get_packet(pkt);

    // breakpoint/watchpoint
`ifdef VERILATOR
    status = $sscanf(pkt, "Z%h,%h,%h", ptype, addr, pkind);
`else
    case (XLEN)
      32: status = $sscanf(pkt, "Z%h,%8h,%h", ptype, addr, pkind);
      64: status = $sscanf(pkt, "Z%h,%16h,%h", ptype, addr, pkind);
    endcase
`endif

    case (ptype)
      swbreak: begin
        // software breakpoints are not supported
        status = gdb_send_packet("");
      end
      default: begin
        // software breakpoints are not supported
        points[addr] = '{ptype, pkind};
        status = gdb_send_packet("OK");
      end
    endcase

    return(1);
  endfunction: gdb_point_insert

///////////////////////////////////////////////////////////////////////////////
// GDB breakpoints/watchpoints matching (return value is the new state)
///////////////////////////////////////////////////////////////////////////////

  // this function is called from the SoC adapter and not from the packet parser
  function automatic state_t gdb_breakpoint_match (
    input bit [XLEN-1:0] addr
  );
    int status;
    state_t tmp = state;

    // match illegal instruction
    if (exe_illegal(addr)) begin
      tmp = SIGILL;
      $display("DEBUG: Triggered illegal instruction at address %h.", addr);
      status = gdb_stop_reply(tmp);
    end else
    // match EBREAK/C.EBREAK instruction (software breakpoint)
    if (exe_ebreak(addr)) begin
      tmp = SIGTRAP;
      $display("DEBUG: Triggered SW breakpoint at address %h.", addr);
      status = gdb_stop_reply(tmp);
    end else
    // match hardware breakpoint
    if (points.exists(addr)) begin
      // TODO: there are also explicit SW breakpoints that depend on ILEN
      if (points[addr].ptype == hwbreak) begin
        tmp = SIGTRAP;
        $display("DEBUG: Triggered HW breakpoint at address %h.", addr);
        status = gdb_stop_reply(tmp);
      end
    end
    return(tmp);
  endfunction: gdb_breakpoint_match

  // this function is called from the SoC adapter and not from the packet parser
  function automatic state_t gdb_watchpoint_match (
    input bit [XLEN-1:0] addr,
    input bit            wena,  // write enable
    input bit    [2-1:0] size
  );
    int status;
    state_t tmp = state;

    // match hardware breakpoint
    if (points.exists(addr)) begin
      if (((points[addr].ptype == watch ) && wena == 1'b1) ||
          ((points[addr].ptype == rwatch) && wena == 1'b0) ||
          ((points[addr].ptype == awatch) )) begin
        // TODO: check is transfer size matches
        tmp = SIGTRAP;
        $display("DEBUG: Triggered HW watchpoint at address %h.", addr);
        status = gdb_stop_reply(tmp);
      end
    end
    return(tmp);
  endfunction: gdb_watchpoint_match

///////////////////////////////////////////////////////////////////////////////
// GDB step/continue
///////////////////////////////////////////////////////////////////////////////

  // TODO: jump to address might not be supported

  function automatic int gdb_step;
    int status;
    string pkt;
    SIZE_T addr;
    int    sig;

    // read packet
    status = gdb_get_packet(pkt);

    // signal/address
    case (pkt[0])
      "s": begin
        status = $sscanf(pkt, "s%h", addr);
        state = STEP;
        if (status == 1) begin
          jump(addr);
        end
      end
      "S": begin
        status = $sscanf(pkt, "S%h;%h", sig, addr);
        state = state_t'(sig);
        if (status == 2) begin
          jump(addr);
        end
      end
    endcase

    // do not send packet response here
    return(0);
  endfunction: gdb_step

  function automatic int gdb_continue ();
    int status;
    string pkt;
    SIZE_T addr;
    int    sig;

    // read packet
    status = gdb_get_packet(pkt);

    // signal/address
    case (pkt[0])
      "c": begin
        status = $sscanf(pkt, "c%h", addr);
        state = CONTINUE;
        if (status == 1) begin
          jump(addr);
        end
      end
      "C": begin
        status = $sscanf(pkt, "C%h;%h", sig, addr);
        state = state_t'(sig);
        if (status == 2) begin
          jump(addr);
        end
      end
    endcase

    $display("DBG: points: %p", points);

    // do not send packet response here
    return(0);
  endfunction: gdb_continue

///////////////////////////////////////////////////////////////////////////////
// GDB kill/detach
///////////////////////////////////////////////////////////////////////////////

  function automatic int gdb_kill ();
    int status;
    string pkt;

    // read packet
    status = gdb_get_packet(pkt);

    // enter RESET state
    state = RESET;

    // do not send packet response here
    return(0);
  endfunction: gdb_kill

  function automatic int gdb_detach ();
    int status;
    string pkt;

    // read packet
    status = gdb_get_packet(pkt);

    // send response (GDB cliend will close the socket connection)
    status = gdb_send_packet("OK");

    // stop HDL simulation, so the HDL simulator can render waveforms
    $info("GDB: detached, stopping simulation from within state %s.", state.name);
    $stop();
    // after user continues HDL simulation blocking wait for GDB client to reconnect
    $info("GDB: continuing stopped simulation, waiting for GDB to reconnect to.");
    void'(socket_accept);

    // do not send packet response here
    return(0);
  endfunction: gdb_detach

///////////////////////////////////////////////////////////////////////////////
// GDB packet
///////////////////////////////////////////////////////////////////////////////

  function automatic int gdb_packet (
    input byte ch []
  );
    static byte bf [] = new[2];
    int status;

    if (ch[0] == "+") begin
      $display("DEBUG: unexpected \"+\".");
      // remove the acknowledge from the socket
      status = socket_recv(ch, 0);
    end else
    if (ch[0] == "$") begin
      status = socket_recv(bf, MSG_PEEK);
      // parse command
      case (bf[1])
//        "x": status = gdb_mem_bin_read();
//        "X": status = gdb_mem_bin_write();
        "m": status = gdb_mem_read();
        "M": status = gdb_mem_write();
        "g": status = gdb_reg_readall();
        "G": status = gdb_reg_writeall();
        "p": status = gdb_reg_readone();
        "P": status = gdb_reg_writeone();
        "s",
        "S": status = gdb_step();
        "c",
        "C": status = gdb_continue();
        "?": status = gdb_state();
        "Q",
        "q":          gdb_query_packet();
        "v":          gdb_verbose_packet();
        "z": status = gdb_point_remove();
        "Z": status = gdb_point_insert();
        "k": status = gdb_kill();
        "D": status = gdb_detach();
        default: begin
          string pkt;
          // read packet
          status = gdb_get_packet(pkt);
          // for unsupported commands respond with empty packet
          status = gdb_send_packet("");
        end
      endcase
    end else begin
      $error("Unexpected sequence from degugger %p = \"%s\".", ch, ch);
    end
    return status;
  endfunction: gdb_packet


  endclass:gdb_server_stub_socket

endpackage: gdb_server_stub_pkg
