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
  } signal_t;

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
    parameter  int unsigned GNUM = 32,  // GPR number
    parameter  type         SIZE_T = int unsigned,  // could be longint (RV64), but it results in warnings
    // number of all registers
    parameter  int unsigned GPRN =   32,       // GPR number
    parameter  int unsigned CSRN = 4096,       // CSR number  TODO: should be a list of indexes
    parameter  int unsigned RNUM = GPRN+CSRN,  // combined register number
    // memory map (shadow memory map)
    parameter  int unsigned MEMN = 1,          // memory regions number
    parameter  type         MMAP_T = struct {SIZE_T base; SIZE_T size;},
    parameter  MMAP_T       MMAP [0: MEMN-1] = '{default: '{base: 0, size: 256}},
    // DEBUG parameters
    parameter  bit DEBUG_LOG = 1'b1
  );

///////////////////////////////////////////////////////////////////////////////
// GDB state
///////////////////////////////////////////////////////////////////////////////

    typedef struct {
      bit debug_log;    // debug log mode
      bit acknowledge;  // acknowledge mode
      bit extended;     // extended remote mode
    } stub_state_t;

    localparam stub_state_t STUB_STATE_INIT = '{
      debug_log: DEBUG_LOG,
      acknowledge: 1'b1,
      extended: 1'b0
    };

    // initialize stub state
    stub_state_t stub_state = STUB_STATE_INIT;

    // supported features
    string features_gdb  [string];
    string features_stub [string] = '{
      "swbreak"        : "+",  // TODO: actually add it to reply
      "hwbreak"        : "+",  // TODO: actually add it to reply
      "error-message"  : "+",
      "binary-upload"  : "-",  // TODO: for now it is broken
      "multiprocess"   : "-",
      "ReverseStep"    : "-",
      "ReverseContinue": "-",
      "QStartNoAckMode": "-"   // TODO: test it
    };

    // DUT shadow state
    typedef struct {
      // signal
      signal_t         sig;             // signal
      // registers
      logic [XLEN-1:0] gpr [0:GPRN-1];  // GPR (general purpose registers)
      logic [XLEN-1:0] pc;              // PC (program counter)
      logic [XLEN-1:0] csr [0:CSRN-1];  // CSR (configuration status registers)
      // memories
      array_t          mem [0:MEMN-1];  // array of memory regions
    } shadow_t;

    // instruction retirement history log entry
    typedef struct {
      // instruction fetch unit
      struct {
        bit [XLEN-1:0] adr;  // instruction address
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

    // DUT simulation state
    typedef struct {
      shadow_t     shd;
      retired_t    run [$];
      int unsigned cnt;      // retired instruction counter
    } dut_state_t;

    dut_state_t dut;

///////////////////////////////////////////////////////////////////////////////
// constructor
///////////////////////////////////////////////////////////////////////////////

    // constructor
    function new (
      string socket = ""
    );
      int status;

      // open character device for R/W
      status = socket_listen(socket);

      // check socket
      if (status == 0) begin
        $fatal(0, "Could not open '%s' device node.", socket);
      end else begin
        $info("Connected to '%0s'.", socket);
      end
      // accept connection from GDB
      void'(socket_accept);

      // DUT shadow state initialization
      // signal
      dut.shd.sig = SIGTRAP;
      // array of memory regions
      shd.mem = new[$size(MMAP)];
      for (int unsigned i; i<$size(MMAP); i++) begin
        shd.mem[i] = new[MMAP[i].size];
      end

      // start state machine
      gdb_fsm();
    endfunction: new

///////////////////////////////////////////////////////////////////////////////
// architecture register/memory access function prototypes
///////////////////////////////////////////////////////////////////////////////

    pure virtual task dut_reset;

    pure virtual task dut_step (
      output retired_t ret
    );

    pure virtual function bit [XLEN-1:0] dut_reg_read (
      input  int unsigned   idx
    );

    pure virtual function void dut_reg_write (
      input  int unsigned   idx,
      input  bit [XLEN-1:0] dat
    );

    pure virtual function byte dut_mem_read (
      input  SIZE_T adr
    );

    // TODO: handle memory write errors
    pure virtual function bit dut_mem_write (
      input  SIZE_T adr,
      input  byte   dat
    );

    // TODO: handle PC write errors
    pure virtual function bit dut_jump (
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
    input bit    ack = stub_state.acknowledge
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
    if (stub_state.debug_log) begin
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
    input bit    ack = stub_state.acknowledge
  );
    int status;
    byte   ch [] = new[1];
    byte   checksum = 0;
    string checksum_str;

    if (stub_state.debug_log) begin
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
// hex encoding of ASCII data
///////////////////////////////////////////////////////////////////////////////

  function automatic string gdb_hex2ascii (
    input string hex
  );
    int code;
    int unsigned len = hex.len()/2;
    string ascii = {len{"-"}};
    // loop through HEX character pairs and convert them to ASCII characters
    for (int unsigned i=0; i<len; i++) begin
      code = $sscanf(hex.substr(2*i, 2*i+1), "%2h", ascii[i]);
    end
    return(ascii);
  endfunction: gdb_hex2ascii

  function automatic string gdb_ascii2hex (
    input string ascii
  );
    int code;
    int unsigned len = ascii.len();
    string hex = {len{"XX"}};
    string tmp;
    // loop through HEX character pairs and convert them to ASCII characters
    for (int unsigned i=0; i<len; i++) begin
      tmp = $sformatf("%02h", ascii[i]);
      hex[i*2+0] = tmp[0];
      hex[i*2+1] = tmp[1];
    end
    return(hex);
  endfunction: gdb_ascii2hex

  // SyetemVerilog uses spaces as separator for strings and GDB packets don't use spaces
  function automatic string char2space (
    input string str,
    input byte   sep  // separator
  );
    for (int unsigned i=0; i<str.len(); i++) begin
      if (str[i] == sep)  str[i] = byte'(" ");
    end
    return(str);
  endfunction: char2space

///////////////////////////////////////////////////////////////////////////////
// GDB signal
///////////////////////////////////////////////////////////////////////////////

  // in response to '?'
  function automatic int gdb_signal();
    string pkt;
    int status;

    // read packet
    status = gdb_get_packet(pkt);

    // reply with current signal
    status = gdb_stop_reply();
    return(status);
  endfunction: gdb_signal

  // TODO: Send a exception packet "T <value>"
  function automatic int gdb_stop_reply (
    input byte signal = dut.shd.sig
  );
    // reply with signal (current signal by default)
    return(gdb_send_packet($sformatf("S%02h", dut.shd.sig)));
  endfunction: gdb_stop_reply

  // send ERROR number reply
  // https://sourceware.org/gdb/current/onlinedocs/gdb.html/Standard-Replies.html#Standard-Replies
  function automatic int gdb_error_number_reply (
    input byte val = 0
  );
    return(gdb_send_packet($sformatf("E%02h", val)));
  endfunction: gdb_error_number_reply

  // send ERROR text reply
  // https://sourceware.org/gdb/current/onlinedocs/gdb.html/Standard-Replies.html#Standard-Replies
  function automatic int gdb_error_text_reply (
    input string str = ""
  );
    return(gdb_send_packet($sformatf("E.%s", gdb_ascii2hex(str))));
  endfunction: gdb_error_text_reply

  // send message to GDB console output
  function automatic int gdb_console_output (
    input string str
  );
    return(gdb_send_packet($sformatf("O%s", gdb_ascii2hex(str))));
  endfunction: gdb_console_output

///////////////////////////////////////////////////////////////////////////////
// GDB query (monitor, )
///////////////////////////////////////////////////////////////////////////////

  // send message to GDB console output
  function automatic int gdb_query_monitor_reply (
    input string str
  );
    return(gdb_send_packet($sformatf("%s", gdb_ascii2hex(str))));
  endfunction: gdb_query_monitor_reply

  // GDB monitor commands
  // https://sourceware.org/gdb/current/onlinedocs/gdb.html/Server.html
  // https://sourceware.org/gdb/current/onlinedocs/gdb.html/General-Query-Packets.html#General-Query-Packets
  function automatic bit gdb_query_monitor (
    input string str
  );
    int status;

    case (str)
      "help": begin
        status = gdb_query_monitor_reply({"HELP: Available monitor commands:\n",
                                          "* set debug on/off.\n"});
        return(1'b1);
      end
      "set debug on": begin
        stub_state.debug_log = 1'b1;
        status = gdb_query_monitor_reply("Enabled debug logging to STDOUT.\n");
        return(1'b1);
      end
      "set debug off": begin
        stub_state.debug_log = 1'b0;
        status = gdb_query_monitor_reply("Disabled debug logging.\n");
        return(1'b1);
      end
      "reset": begin
        state = RESET;
      end
      default begin
        return(1'b0);
      end
    endcase
  endfunction: gdb_query_monitor

  // GDB supported features
  // https://sourceware.org/gdb/current/onlinedocs/gdb.html/Server.html
  // https://sourceware.org/gdb/current/onlinedocs/gdb.html/General-Query-Packets.html#General-Query-Packets
  function automatic bit gdb_query_supported (
    input string str
  );
    int code;
    int status;
    string tmp;
    string feature;
    string value;

    // parse features supported by the GDB client
    str = char2space(str, byte'(";"));
    while ($sscanf(str, "%s", tmp)) begin
      // add feature and its value (+/-/?)
      value = string'(tmp[tmp.len()-1]);
      if (value inside {"+", "-", "?"}) begin
        feature = tmp.substr(0, tmp.len()-2);
      end else begin
        tmp = char2space(tmp, byte'("="));
        code = $sscanf(str, "%s=%s", feature, value);
      end
      features_gdb[feature] = value;
      // remove the processed feature from the string (unless it is already the last one)
      if (str.len() > tmp.len()) begin
        // +1 to skip the space
        str = str.substr(tmp.len()+1, str.len()-1);
      end else begin
        break;
      end
    end
    $display("DEBUG: features_gdb = %p", features_gdb);

    // reply with stub features
    str = "";
    foreach (features_stub[feature]) begin
      if (features_stub[feature] inside {"+", "-", "?"}) begin
        str = {str, $sformatf("%s%s;", feature, features_stub[feature])};
      end else begin
        str = {str, $sformatf("%s=%s;", feature, features_stub[feature])};
      end
    end
    // remove the trailing semicolon
    str = str.substr(0, str.len()-2);
    status = gdb_send_packet(str);

    return(0);
  endfunction: gdb_query_supported

  function automatic int gdb_query_packet ();
    string pkt;
    string str;
    int status;

    // read packet
    status = gdb_get_packet(pkt);

    // parse various query packets
    if ($sscanf(pkt, "qSupported:%s", str) > 0) begin
      $display("DEBUG: qSupported = %p", str);
      status = gdb_query_supported(str);
    end else
    if ($sscanf(pkt, "qRcmd,%s", str) > 0) begin
      status = gdb_query_monitor(gdb_hex2ascii(str));
      status = gdb_send_packet("OK");
    end else begin
      // not supported, send empty response packet
      status = gdb_send_packet("");
    end
    return(0);
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
      tmp = $sformatf("%02h", dut_mem_read(adr+i));
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
//      $display("DBG: gdb_mem_write: adr+i = 'h%08h, mem[adr+i] = 'h%02h", adr+i, dut_mem_read(adr+i));
`ifdef VERILATOR
      status = $sscanf(str.substr(i*2, i*2+1), "%2h", dat);
`else
      status = $sscanf(str.substr(i*2, i*2+1), "%h", dat);
`endif
//      $display("DBG: gdb_mem_write: adr+i = 'h%08h, mem[adr+i] = 'h%02h", adr+i, dut_mem_read(adr+i));
      // TODO handle memory access errors
      void'(dut_mem_write(adr+i, dat));
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
      pkt[i] = dut_mem_read(adr+i);
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
      // TODO handle memory access errors
      void'(dut_mem_write(adr+i, pkt[code+i]));
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
      val = {<<8{dut_reg_read(i)}};
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
      dut_reg_write(i, {<<8{val}});
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
    val = {<<8{dut_reg_read(idx)}};
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
    dut_reg_write(idx, {<<8{val}});
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
  function automatic signal_t gdb_breakpoint_match (
    input bit [XLEN-1:0] addr
  );
    int status;
    signal_t tmp = dut.shd.sig;

    // match illegal instruction
    if (dut_illegal(addr)) begin
      tmp = SIGILL;
      $display("DEBUG: Triggered illegal instruction at address %h.", addr);
      status = gdb_stop_reply(tmp);
    end else
    // match EBREAK/C.EBREAK instruction (software breakpoint)
    if (dut_break(addr)) begin
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
  function automatic signal_t gdb_watchpoint_match (
    input bit [XLEN-1:0] addr,
    input bit            wena,  // write enable
    input bit    [2-1:0] size
  );
    int status;
    signal_t tmp = dut.shd.sig;

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
// applying /reverting retired instruction to/from DUT shadow state
///////////////////////////////////////////////////////////////////////////////

function automatic int shadow_old (
  int unsigned idx
);
  // PC
  dut.shd.pc = dut.run[idx].ifu.adr;
  // GPR remember the old value and apply the new one
  for (int unsigned i=0; i<$size(dut.run[idx].gpr); i++) begin: gpr
    dut.run[idx].gpr[i].old = dut.shd.gpr[dut.run[idx].gpr[i].idx];
  end: gpr
  // memory
  bit [XLEN-1:0] adr = dut.run[idx].lsu.adr;
  for (int unsigned blk=0; blk<$size(MMAP); blk++) begin: blk
    if ((adr >= MMAP[blk].base) &&
        (adr <  MMAP[blk].base + MMAP[blk].size)) begin: mem
      int unsigned size = $size(dut.run[idx].lsu.val);
      dut.run[idx].lsu.old = new[size];
      for (int unsigned i=0; i<size; i++) begin: byt
        dut.run[idx].lsu.old[i] = dut.shd.mem[adr - MMAP[blk].base];
      end: byt
    end: mem
  end: blk
endfunction: shadow_old

function automatic int shadow_apply (
  int unsigned idx
);
  // PC
  dut.shd.pc = dut.run[idx].ifu.adr;
  // GPR remember the old value and apply the new one
  for (int unsigned i=0; i<$size(dut.run[idx].gpr); i++) begin: gpr
    bit [5-1:0] r = dut.run[idx].gpr[i].idx;
    dut.shd.gpr[dut.run[idx].gpr[i].idx] = dut.run[idx].gpr[i].val;
  end: gpr
  // CSR remember the read value and apply wtitten value
  for (int unsigned i=0; i<$size(dut.run[idx].csr); i++) begin: csr
    dut.shd.csr[dut.run[idx].csr[i].idx] = dut.run[idx].csr[i].wdt;
  end: csr
  // memory
  bit [XLEN-1:0] adr = dut.run[idx].lsu.adr;
  for (int unsigned blk=0; blk<$size(MMAP); blk++) begin: blk
    if ((adr >= MMAP[blk].base) &&
        (adr <  MMAP[blk].base + MMAP[blk].size)) begin: mem
      int unsigned size = $size(dut.run[idx].lsu.val);
      for (int unsigned i=0; i<size; i++) begin: byt
        dut.shd.mem[adr - MMAP[blk].base] = dut.run[idx].lsu.val[i];
      end: byt
    end: mem
  end: blk
  // increment position counter
  dut.cnt++;
endfunction: shadow_apply

function automatic int shadow_revert (
  int unsigned idx
);
  // PC
  dut.shd.pc = dut.run[idx-1].ifu.adr;
  // GPR remember the old value and apply the new one
  for (int unsigned i=0; i<$size(dut.run[idx].gpr); i++) begin: gpr
    dut.shd.gpr[dut.run[idx].gpr[i].idx] = dut.run[idx].gpr[i].old;
  end: gpr
  // CSR remember the read value and apply wtitten value
  for (int unsigned i=0; i<$size(dut.run[idx].csr); i++) begin: csr
    dut.shd.csr[dut.run[idx].csr[i].idx] = dut.run[idx].csr[i].rdt;
  end: csr
  // memory
  bit [XLEN-1:0] adr = dut.run[idx].lsu.adr;
  for (int unsigned blk=0; blk<$size(MMAP); blk++) begin: blk
    if ((adr >= MMAP[blk].base) &&
        (adr <  MMAP[blk].base + MMAP[blk].size)) begin: mem
      int unsigned size = $size(dut.run[idx].lsu.val);
      for (int unsigned i=0; i<size; i++) begin: byt
        dut.shd.mem[adr - MMAP[blk].base] = dut.run[idx].lsu.old[i];
      end: byt
    end: mem
  end: blk
  // decrement position counter
  dut.cnt--;
endfunction: shadow_revert

///////////////////////////////////////////////////////////////////////////////
// GDB step/continue
///////////////////////////////////////////////////////////////////////////////

  task automatic gdb_step;
    int       status;
    string    pkt;
    SIZE_T    addr;
    int       sig;
    bit       jmp;
    retired_t ret;

    // read packet
    status = gdb_get_packet(pkt);

    // signal/address
    case (pkt[0])
      "s": begin
        status = $sscanf(pkt, "s%h", addr);
        jmp = (status == 1);
      end
      "S": begin
        status = $sscanf(pkt, "S%h;%h", sig, addr);
        dut.shd.sig = signal_t'(sig);
        jmp = (status == 2);
      end
    endcase

    // TODO handle PC write errors
    //void'(dut_jump(addr));

    // record
    if (dut.cnt == dut.run.size()) begin
      // perform DUT step
      shadow_old  (dut.cnt);
      shadow_apply(dut.cnt);
      dut_step(ret);
      dut.run.push_back(ret);
    end
    // replay
    else begin
      shadow_apply(dut.cnt);
    end

    // breakpoint/watchpoint
    // response packet
    // TODO

  endtask: gdb_step

  task automatic gdb_continue ();
    int status;
    string pkt;
    SIZE_T addr;
    int    sig;
    bit    jmp;

    // read packet
    status = gdb_get_packet(pkt);

    // signal/address
    case (pkt[0])
      "c": begin
        status = $sscanf(pkt, "c%h", addr);
        dut.shd.sig = CONTINUE;
        jmp = (status == 1);
        if (status == 1) begin
          // TODO handle PC write errors
          void'(dut_jump(addr));
        end
      end
      "C": begin
        status = $sscanf(pkt, "C%h;%h", sig, addr);
        dut.shd.sig = signal_t'(sig);
        jmp = (status == 2);
      end
    endcase

    // TODO handle PC write errors
    //void'(dut_jump(addr));

    $display("DBG: points: %p", points);

  endtask: gdb_continue

///////////////////////////////////////////////////////////////////////////////
// GDB extended/reset/detach/kill
///////////////////////////////////////////////////////////////////////////////

  function automatic int gdb_extended ();
    int status;
    string pkt;

    // read packet
    status = gdb_get_packet(pkt);

    // set extended mode
    stub_state.extended = 1'b1;

    // send response
    status = gdb_send_packet("OK");

    return(0);
  endfunction: gdb_extended

  function automatic int gdb_reset ();
    int status;
    string pkt;

    // read packet
    status = gdb_get_packet(pkt);

    // enter RESET state
    state = RESET;

    // do not send packet response here
    return(0);
  endfunction: gdb_reset

  function automatic int gdb_detach ();
    int status;
    string pkt;

    // read packet
    status = gdb_get_packet(pkt);

    // send response (GDB cliend will close the socket connection)
    status = gdb_send_packet("OK");

    // re-initialize stub state
    stub_state = STUB_STATE_INIT;

    // stop HDL simulation, so the HDL simulator can render waveforms
    $info("GDB: detached, stopping simulation from within state %s.", state.name);
    $stop();
    // after user continues HDL simulation blocking wait for GDB client to reconnect
    $info("GDB: continuing stopped simulation, waiting for GDB to reconnect to.");
    void'(socket_accept);

    return(0);
  endfunction: gdb_detach

  function automatic int gdb_kill ();
    int status;
    string pkt;

    // read packet
    status = gdb_get_packet(pkt);

    // finish simulation
    $finish();

    // do not send packet response here
    return(0);
  endfunction: gdb_kill

///////////////////////////////////////////////////////////////////////////////
// GDB packet
///////////////////////////////////////////////////////////////////////////////

  task automatic gdb_packet (
    input byte ch []
  );
    static byte bf [] = new[2];
    int status;

    if (ch[0] == "+") begin
      $display("DEBUG: unexpected \"+\".");
      // remove the initial welcome acknowledge "+" from the socket
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
        "S":          gdb_step();
        "c",
        "C": status = gdb_continue();
        "?": status = gdb_signal();
        "Q",
        "q": status = gdb_query_packet();
        "v":          gdb_verbose_packet();
        "z": status = gdb_point_remove();
        "Z": status = gdb_point_insert();
        "!": status = gdb_extended();
        "R": status = gdb_reset();
        "D": status = gdb_detach();
        "k": status = gdb_kill();
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
  endtask: gdb_packet


///////////////////////////////////////////////////////////////////////////////
// GDB state machine
///////////////////////////////////////////////////////////////////////////////

    task gdb_fsm ();


    endtask: gdb_fsm

  endclass:gdb_server_stub_socket

endpackage: gdb_server_stub_pkg
