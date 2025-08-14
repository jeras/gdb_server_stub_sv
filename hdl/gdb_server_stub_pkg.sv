///////////////////////////////////////////////////////////////////////////////
// GDB server stub package (contains packet parser, responder)
//
// Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
//
// Licensed under CERN-OHL-P v2 or later
///////////////////////////////////////////////////////////////////////////////

package gdb_server_stub_pkg;

    import socket_dpi_pkg::*;
    import gdb_shadow_pkg::*;

    // byte dynamic array type for casting to/from string
    typedef byte byte_array_t [];

    // dynamic array of strings
    typedef string string_array_t [];

    // queue of strings
    typedef string string_queue_t [$];

    virtual class gdb_server_stub #(
        // list of CPUs
        parameter  string_array_t THREADS = '{"CPU0"},
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
        parameter  bit REMOTE_LOG = 1'b1
    );

    ///////////////////////////////////////
    // GDB state
    ///////////////////////////////////////

        typedef struct {
            shortint unsigned socket_port;
            bit remote_log;   // debug log mode
            bit acknowledge;  // acknowledge mode
            bit extended;     // extended remote mode
            bit register;     // read registers from (0-shadow, 1-DUT)
            bit memory;       // read memory from (0-shadow, 1-DUT)
        } stub_state_t;

        localparam stub_state_t STUB_STATE_INIT = '{
            socket_port: 0,
            remote_log: REMOTE_LOG,
            acknowledge: 1'b1,
            extended: 1'b0,
            register: 1'b0,
            memory: 1'b0
        };

        // initialize stub state
        stub_state_t stub_state = STUB_STATE_INIT;

        // supported features
        string features_gdb  [string];
        string features_stub [string] = '{
            "swbreak"            : "+",
            "hwbreak"            : "+",
            "error-message"      : "+",  // GDB (LLDB asks with QEnableErrorStrings)
            "binary-upload"      : "-",  // TODO: for now it is broken
            "multiprocess"       : "-",
            "ReverseStep"        : "+",
            "ReverseContinue"    : "+",
            "QStartNoAckMode"    : "+"
        };

        // TODO: this are LLDB features, but GDB are similar
//      else if (x == "qXfer:features:read+")
//      else if (x == "qXfer:memory-map:read+")
//      else if (x == "qXfer:siginfo:read+")

        typedef gdb_shadow #(
            // 32/64 bit CPU selection
            .XLEN   (XLEN),
            // choice between F/D/Q floating point support
//          .FLEN   (FLEN),
            // number of all registers
            .GPRN   (GPRN),
            .CSRN   (CSRN),
            // memory map (shadow memory map)
            .SIZE_T (SIZE_T),
            .MEMN   (MEMN  ),
            .MMAP_T (MMAP_T),
            .MMAP   (MMAP  )
        ) gdb_shadow_t;

        typedef gdb_shadow_t::retired_t retired_t;

        // DUT shadow state
        gdb_shadow_t shd;

    ////////////////////////////////////////
    // constructor
    ////////////////////////////////////////

        // constructor
        function new (
            input string socket = ""
        );
            int status;

//            stub_state.socket_port = socket.atoi();
            if ($sscanf(socket, ":%d", stub_state.socket_port)) begin
                status = socket_tcp_listen(stub_state.socket_port);
                status = socket_tcp_accept();
            end else begin
                status = socket_unix_listen(socket);
                status = socket_unix_accept();
            end

            // DUT shadow state initialization
            shd = new();
        endfunction: new

    ////////////////////////////////////////
    // DUT reg/mem access function prototypes
    ////////////////////////////////////////

        pure virtual task dut_reset_assert;

        pure virtual task dut_reset_release;

        pure virtual task dut_step (
            ref retired_t ret
        );

        // TODO: handle PC write errors
        pure virtual task dut_jump (
            input  SIZE_T adr
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

    ////////////////////////////////////////
    // RSP character get/put
    ////////////////////////////////////////

        function automatic void rsp_write (string str);
            int status;
            byte buffer [] = new[str.len()](byte_array_t'(str));
            status = socket_send(buffer, 0);
        endfunction: rsp_write

    ////////////////////////////////////////
    // RSP packet get/send
    ////////////////////////////////////////

        function automatic int rsp_get_packet(
            output string pkt,
            input  bit    ack = stub_state.acknowledge
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
      //          $display("DEBUG: rsp_get_packet: buffer = %p", buffer);
                str = {str, string'(buffer)};
                len = str.len();
      //          $display("DEBUG: rsp_get_packet: str = %s", str);
            end while (str[len-3] != "#");

            // extract packet data from received string
            pkt = str.substr(1,len-4);
            if (stub_state.remote_log) begin
                $display("REMOTE: <- %p", pkt);
            end

            // calculate packet data checksum
            cmd = new[len-4](byte_array_t'(pkt));
            checksum = cmd.sum();

            // Get checksum now
            checksum_ref = str.substr(len-2,len-1);

            // Verify checksum
            checksum_str = $sformatf("%02h", checksum);
            if (checksum_ref != checksum_str) begin
                $error("Bad checksum. Got 0x%s but was expecting: 0x%s for packet '%s'", checksum_ref, checksum_str, pkt);
                if (ack) begin
                    // NACK packet
                    rsp_write("-");
                end
                return (-1);
            end else begin
                if (ack) begin
                    // ACK packet
                    rsp_write("+");
                end
                return(0);
            end
        endfunction: rsp_get_packet

        function automatic int rsp_send_packet(
            input string pkt,
            input bit    ack = stub_state.acknowledge
        );
            int status;
            byte   ch [] = new[1];
            byte   checksum = 0;

            if (stub_state.remote_log) begin
                $display("REMOTE: -> %p", pkt);
            end

            // Send packet start
            rsp_write("$");

            // Send packet data and calculate checksum
            foreach (pkt[i]) begin
                checksum += pkt[i];
                rsp_write(string'(pkt[i]));
            end

            // Send packet end
            rsp_write("#");

            // Send the checksum
            rsp_write($sformatf("%02h", checksum));

            // Check acknowledge
            if (ack) begin
                status = socket_recv(ch, 0);
                if (ch[0] == "+")  return(0);
                else               return(-1);
            end
        endfunction: rsp_send_packet

    ////////////////////////////////////////
    // hex encoding of ASCII data
    ////////////////////////////////////////

        function automatic string rsp_hex2ascii (
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
        endfunction: rsp_hex2ascii

        function automatic string rsp_ascii2hex (
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
        endfunction: rsp_ascii2hex

        // SyetemVerilog uses spaces as separator for strings and GDB packets don't use spaces
        // replace separator characters with spaces
        function automatic string char2space (
            input string str,
            input byte   sep  // separator
        );
            for (int unsigned i=0; i<str.len(); i++) begin
                if (str[i] == sep)  str[i] = byte'(" ");
            end
            return(str);
        endfunction: char2space

        function automatic string_queue_t split (
            input string str,
            input byte   sep  // separator
        );
            string tmp;
            // replace separator with space
            str = char2space(str, sep);
            while ($sscanf(str, "%s", tmp)) begin
                // add element to list
                split.push_back(tmp);
                // remove element from string
                if (str.len() > tmp.len()) begin
                    // +1 to skip the space left by the ";" separator
                    str = str.substr(tmp.len()+1, str.len()-1);
                end else begin
                    break;
                end
            end
        endfunction: split

    ///////////////////////////////////////
    // RSP signal
    ///////////////////////////////////////

        // in response to '?'
        function automatic int rsp_signal();
            string pkt;
            int status;

            // read packet
            status = rsp_get_packet(pkt);

            // reply with current signal
            status = rsp_stop_reply();
            return(status);
        endfunction: rsp_signal

        // TODO: Send a exception packet "T <value>"
        function automatic int rsp_stop_reply (
            // register
            input int unsigned idx = -1,
            input [XLEN-1:0]   val = 'x,
            // thread
            input int thr = -1,
            // core
            input int unsigned core = -1
        );
            string str;
            // reply with signal/register/thread/core/reason
            str = $sformatf("T%02h;", shd.sig);
            // register
            if (idx != -1) begin
                case (XLEN)
                    32: str = {str, $sformatf("%0h:%08h;", idx, val)};
                    64: str = {str, $sformatf("%0h:%016h;", idx, val)};
                endcase
            end
            // thread
            if (thr != -1) begin
                str = {str, $sformatf("thread:%s;", sformat_thread(1, thr))};
            end
            // core
            if (core != -1) begin
                str = {str, $sformatf("core:%h;", core)};
            end
            // reason
            case (shd.rsn.ptype)
                watch, rwatch, awatch: begin
                    str = {str, $sformatf("%s:%h;", shd.rsn.ptype.name, shd.ret.lsu.adr)};
                end
                swbreak, hwbreak: begin
                    str = {str, $sformatf("%s:;", shd.rsn.ptype.name)};
                end
                replaylog: begin
                    str = {str, $sformatf("%s:%s;", shd.rsn.ptype.name, shd.cnt == 0 ? "begin" : "end")};
                end
            endcase
            // remove the trailing semicolon
            str = str.substr(0, str.len()-2);
            return(rsp_send_packet(str));
        //    // reply with signal (current signal by default)
        //    return(rsp_send_packet($sformatf("S%02h", sig)));
        endfunction: rsp_stop_reply

        // send ERROR number reply (GDB only)
        // https://sourceware.org/gdb/current/onlinedocs/gdb.html/Standard-Replies.html#Standard-Replies
        function automatic int rsp_error_number_reply (
            input byte val = 0
        );
            return(rsp_send_packet($sformatf("E%02h", val)));
        endfunction: rsp_error_number_reply

        // send ERROR text reply (GDB only)
        // https://sourceware.org/gdb/current/onlinedocs/gdb.html/Standard-Replies.html#Standard-Replies
        function automatic int rsp_error_text_reply (
            input string str = ""
        );
            return(rsp_send_packet($sformatf("E.%s", rsp_ascii2hex(str))));
        endfunction: rsp_error_text_reply

        // send ERROR LLDB reply
        // https://lldb.llvm.org/resources/lldbgdbremote.html#qenableerrorstrings
        function automatic int rsp_error_lldb_reply (
            input byte   val = 0,
            input string str = ""
        );
            return(rsp_send_packet($sformatf("E%02h;%s", val, rsp_ascii2hex(str))));
        endfunction: rsp_error_lldb_reply

        // send message to GDB console output
        function automatic int rsp_console_output (
            input string str
        );
            return(rsp_send_packet($sformatf("O%s", rsp_ascii2hex(str))));
        endfunction: rsp_console_output

    ///////////////////////////////////////
    // RSP query (monitor, )
    ///////////////////////////////////////

        // send message to GDB console output
        function automatic int rsp_query_monitor_reply (
            input string str
        );
            return(rsp_send_packet($sformatf("%s", rsp_ascii2hex(str))));
        endfunction: rsp_query_monitor_reply

        // GDB monitor commands
        // https://sourceware.org/gdb/current/onlinedocs/gdb.html/Server.html
        // https://sourceware.org/gdb/current/onlinedocs/gdb.html/General-Query-Packets.html#General-Query-Packets
        task rsp_query_monitor (
            input string str
        );
            int status;

            case (str)
                "help": begin
                    status = rsp_query_monitor_reply({"HELP: Available monitor commands:\n",
                                                      "* 'set remote log on/off',\n",
                                                      "* 'set waveform dump on/off',\n",
                                                      "* 'set register=dut/shadow' (reading registers from dut/shadow, default is shadow),\n",
                                                      "* 'set memory=dut/shadow' (reading memories from dut/shadow, default is shadow),\n",
                                                      "* 'reset assert' (assert reset for a few clock periods),\n",
                                                      "* 'reset release' (synchronously release reset)."});
                end
                "set remote log on": begin
                    stub_state.remote_log = 1'b1;
                    status = rsp_query_monitor_reply("Enabled remote logging to STDOUT.\n");
                end
                "set remote log off": begin
                    stub_state.remote_log = 1'b0;
                    status = rsp_query_monitor_reply("Disabled remote logging.\n");
                end
                "set waveform dump on": begin
                    $dumpon;
                    status = rsp_query_monitor_reply("Enabled waveform dumping.\n");
                end
                "set waveform dump off": begin
                    $dumpoff;
                    status = rsp_query_monitor_reply("Disabled waveform dumping.\n");
                end
                "set register=dut": begin
                    stub_state.register = 1'b1;
                    status = rsp_query_monitor_reply("Reading registers directly from DUT.\n");
                end
                "set register=shadow": begin
                    stub_state.register = 1'b0;
                    status = rsp_query_monitor_reply("Reading registers from shadow copy.\n");
                end
                "set memory=dut": begin
                    stub_state.memory = 1'b1;
                    status = rsp_query_monitor_reply("Reading memory directly from DUT.\n");
                end
                "set memory=shadow": begin
                    stub_state.memory = 1'b0;
                    status = rsp_query_monitor_reply("Reading memory from shadow copy.\n");
                end
                "reset assert": begin
                    dut_reset_assert;
                    // TODO: rethink whether to reset the shadow or keep it
                    //shd = new();
                    status = rsp_query_monitor_reply("DUT reset asserted.\n");
                end
                "reset release": begin
                    dut_reset_release;
                    status = rsp_query_monitor_reply("DUT reset released.\n");
                end
                default begin
                    status = rsp_query_monitor_reply("'monitor' command was not recognized.\n");
                end
            endcase
        endtask: rsp_query_monitor

        // GDB supported features
        // https://sourceware.org/gdb/current/onlinedocs/gdb.html/Server.html
        // https://sourceware.org/gdb/current/onlinedocs/gdb.html/General-Query-Packets.html#General-Query-Packets
        function automatic bit rsp_query_supported (
            input string str
        );
            int code;
            int status;
            string_queue_t features;
            string feature;
            string value;

            // parse features supported by the GDB client
            features = split(str, byte'(";"));
            foreach (features[i]) begin
                int unsigned len = features[i].len();
                // add feature and its value (+/-/?)
                value = string'(features[i][len-1]);
                if (value inside {"+", "-", "?"}) begin
                    feature = features[i].substr(0, len-2);
                end else begin
                    features[i] = char2space(features[i], byte'("="));
                    code = $sscanf(str, "%s %s", feature, value);
                end
                features_gdb[feature] = value;
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
            status = rsp_send_packet(str);

            return(0);
        endfunction: rsp_query_supported


        function automatic string sformat_thread (
            input int process,
            input int thread
        );
            case (features_stub["multiprocess"])
                "+": return($sformatf("p%h,%h", process, thread));
                "-": return($sformatf("%0h", thread));
            endcase
        endfunction: sformat_thread

        function automatic int sscan_thread (
            input string str
        );
            int code;
            int process;
            int thread;
            case (features_stub["multiprocess"])
                "+": code = $sscanf(str, "p%h,%h;", process, thread);
                "-": code = $sscanf(str, "%h", thread);
            endcase
            return(thread);
        endfunction: sscan_thread


        task rsp_query_packet ();
            string pkt;
            string str;
            int status;

            // read packet
            status = rsp_get_packet(pkt);

            // parse various query packets
            if ($sscanf(pkt, "qSupported:%s", str) > 0) begin
                $display("DEBUG: qSupported = %p", str);
                status = rsp_query_supported(str);
            end else
            // parse various monitor packets
            if ($sscanf(pkt, "qRcmd,%s", str) > 0) begin
                rsp_query_monitor(rsp_hex2ascii(str));
            end else
            // start no acknowledge mode
            if (pkt == "QStartNoAckMode") begin
                status = rsp_send_packet("OK");
                stub_state.acknowledge = 1'b0;
            end else
            // start no acknowledge mode
            if (pkt == "QEnableErrorStrings") begin
                status = rsp_send_packet("OK");
                stub_state.acknowledge = 1'b0;
            end else
            // query first thread info
            if (pkt == "qfThreadInfo") begin
                str = "m";
                foreach (THREADS[thr]) begin
                    str = {str, sformat_thread(1, thr+1), ","};
                end
                // remove the trailing comma
                str = str.substr(0, str.len()-2);
                status = rsp_send_packet(str);
            end else
            // query subsequent thread info
            if (pkt == "qsThreadInfo") begin
                // last thread
                status = rsp_send_packet("l");
            end else
            // query extra info for given thread
            if ($sscanf(pkt, "qThreadExtraInfo,%s", str) > 0) begin
                int thr;
                thr = sscan_thread(str);
                $display("DEBUG: qThreadExtraInfo: str = %p, thread = %0d, THREADS[%0d-1] = %s", str, thr, thr, THREADS[thr-1]);
                status = rsp_send_packet(rsp_ascii2hex(THREADS[thr-1]));
            end else
            // query first thread info
            if (pkt == "qC") begin
                int thr = 1;
                str = {"QC", sformat_thread(1, thr)};
                status = rsp_send_packet(str);
            end else
            // query whether the remote server attached to an existing process or created a new process
            if (pkt == "qAttached") begin
                // respond as "attached"
                status = rsp_send_packet("1");
            end else
            // not supported, send empty response packet
            begin
                status = rsp_send_packet("");
            end
        endtask: rsp_query_packet

    ////////////////////////////////////////
    // RSP verbose
    ////////////////////////////////////////

        function automatic void rsp_verbose_packet ();
            string pkt;
            string str;
            string tmp;
            int status;
            int code;

            // read packet
            status = rsp_get_packet(pkt);

//          // interrupt signal
//          if (pkt == "vCtrlC") begin
//              shd.sig = SIGINT;
//              status = rsp_send_packet("OK");
//          end else
            // list actions supported by the ‘vCont’ packet
            if (pkt == "vCont?") begin
                status = rsp_send_packet("vCont;c:C;s:S");
            end else
            // parse 'vcont' packet
            if ($sscanf(pkt, "vCont;%s", str) > 0) begin
                string_queue_t features;
                string action;
                string thread;
                // parse action list
                features = split(str, byte'(";"));
                foreach (features[i]) begin
                    // parse action/thread pair
                    features[i] = char2space(features[i], byte'(":"));
                    code = $sscanf(features[i], "%s %s", action, thread);
                    // TODO
                end
            end else
            // not supported, send empty response packet
            begin
                status = rsp_send_packet("");
            end
        endfunction: rsp_verbose_packet

    ////////////////////////////////////////
    // RSP memory access (hexadecimal)
    ////////////////////////////////////////

        function automatic int rsp_mem_read ();
            int code;
            string pkt;
            int status;
            SIZE_T adr;
            SIZE_T len;
            byte val;

            // read packet
            status = rsp_get_packet(pkt);

        //    $display("DBG: rsp_mem_read: pkt = %s", pkt);

            // memory address and length
`ifdef VERILATOR
            code = $sscanf(pkt, "m%h,%h", adr, len);
`else
            case (XLEN)
                32: code = $sscanf(pkt, "m%8h,%8h", adr, len);
                64: code = $sscanf(pkt, "m%16h,%16h", adr, len);
            endcase
`endif

        //    $display("DBG: rsp_mem_read: adr = %08x, len=%08x", adr, len);

            // read memory
            pkt = {len{"XX"}};
            for (SIZE_T i=0; i<len; i++) begin
                string tmp = "XX";
                if (stub_state.memory) begin
                    val =     dut_mem_read(adr+i);
                end else begin
                    val = shd.mem_read(adr+i, 1)[0];
                end
                tmp = $sformatf("%02h", val);
                pkt[i*2+0] = tmp[0];
                pkt[i*2+1] = tmp[1];
            end

        //    $display("DBG: rsp_mem_read: pkt = %s", pkt);

            // send response
            status = rsp_send_packet(pkt);

            return(len);
        endfunction: rsp_mem_read

        function automatic int rsp_mem_write ();
            int code;
            string pkt;
            string str;
            int status;
            SIZE_T adr;
            SIZE_T len;
            byte   dat;

            // read packet
            status = rsp_get_packet(pkt);
        //    $display("DBG: rsp_mem_write: pkt = %s", pkt);

            // memory address and length
`ifdef VERILATOR
            code = $sscanf(pkt, "M%h,%h:", adr, len);
`else
            case (XLEN)
                32: code = $sscanf(pkt, "M%8h,%8h:", adr, len);
                64: code = $sscanf(pkt, "M%16h,%16h:", adr, len);
            endcase
`endif
            //    $display("DBG: rsp_mem_write: adr = 'h%08h, len = 'd%0d", adr, len);

                // remove the header from the packet, only data remains
                str = pkt.substr(pkt.len() - 2*len, pkt.len() - 1);
            //    $display("DBG: rsp_mem_write: str = %s", str);

            // write memory
            for (SIZE_T i=0; i<len; i++) begin
            //    $display("DBG: rsp_mem_write: adr+i = 'h%08h, mem[adr+i] = 'h%02h", adr+i, dut_mem_read(adr+i));
`ifdef VERILATOR
                status = $sscanf(str.substr(i*2, i*2+1), "%2h", dat);
`else
                status = $sscanf(str.substr(i*2, i*2+1), "%h", dat);
`endif
            //    $display("DBG: rsp_mem_write: adr+i = 'h%08h, mem[adr+i] = 'h%02h", adr+i, dut_mem_read(adr+i));
                // TODO handle memory access errors
                // NOTE: memory writes are always done to both DUT and shadow
                void'(    dut_mem_write(adr+i,                 dat  ));
                void'(shd.mem_write(adr+i, byte_array_t'('{dat})));
            end

            // send response
            status = rsp_send_packet("OK");

            return(len);
        endfunction: rsp_mem_write

    ////////////////////////////////////////
    // RSP memory access (binary)
    ////////////////////////////////////////

        function automatic int rsp_mem_bin_read ();
            int code;
            string pkt;
            int status;
            SIZE_T adr;
            SIZE_T len;

            // read packet
            status = rsp_get_packet(pkt);

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
                if (stub_state.memory) begin
                    pkt[i] =     dut_mem_read(adr+i);
                end else begin
                    pkt[i] = shd.mem_read(adr+i, 1)[0];
                end
            end

            // send response
            status = rsp_send_packet(pkt);

            return(len);
        endfunction: rsp_mem_bin_read

        function automatic int rsp_mem_bin_write ();
            int code;
            string pkt;
            int status;
            SIZE_T adr;
            SIZE_T len;

            // read packet
            status = rsp_get_packet(pkt);

            // memory address and length
`ifdef VERILATOR
            code = $sscanf(pkt, "X%h,%h:", adr, len);
`else
            case (XLEN)
                32: code = $sscanf(pkt, "X%8h,%8h:", adr, len);
                64: code = $sscanf(pkt, "X%16h,%16h:", adr, len);
            endcase
`endif

            // write memory
            for (SIZE_T i=0; i<len; i++) begin
                // TODO handle memory access errors
                // NOTE: memory writes are always done to both DUT and shadow
                void'(dut_mem_write(adr+i,                 pkt[code+i]  ));
                void'(shd.mem_write(adr+i, byte_array_t'('{pkt[code+i]})));
            end

            // send response
            status = rsp_send_packet("OK");

            return(len);
        endfunction: rsp_mem_bin_write

    ////////////////////////////////////////
    // RSP multiple register access
    ////////////////////////////////////////

        // "g" packet
        function automatic int rsp_reg_readall ();
            int status;
            string pkt;
            logic [XLEN-1:0] val;  // 4-state so GDB can iterpret 'x

            // read packet
            status = rsp_get_packet(pkt);

            pkt = "";
            for (int unsigned i=0; i<REGN; i++) begin
                // swap byte order since they are sent LSB first
                if (stub_state.register) begin
                    val = {<<8{dut_reg_read(i)}};
                end else begin
                    val = {<<8{shd.reg_read(i)}};
                end
                case (XLEN)
                    32: pkt = {pkt, $sformatf("%08h", val)};
                    64: pkt = {pkt, $sformatf("%016h", val)};
                endcase
            end

            // send response
            status = rsp_send_packet(pkt);

            return(0);
        endfunction: rsp_reg_readall

        function automatic int rsp_reg_writeall ();
            string pkt;
            int status;
            int unsigned len = XLEN/8*2;
            bit [XLEN-1:0] val;

            // read packet
            status = rsp_get_packet(pkt);
            // remove command
            pkt = pkt.substr(1, pkt.len()-1);

            // GPR
            for (int unsigned i=0; i<REGN; i++) begin
`ifdef VERILATOR
                status = $sscanf(pkt.substr(i*len, i*len+len-1), "%h", val);
`else
                case (XLEN)
                  32: status = $sscanf(pkt.substr(i*len, i*len+len-1), "%8h", val);
                  64: status = $sscanf(pkt.substr(i*len, i*len+len-1), "%16h", val);
                endcase
`endif
                // swap byte order since they are sent LSB first
                // NOTE: register writes are always done to both DUT and shadow
                dut_reg_write(i, {<<8{val}});
                shd.reg_write(i, {<<8{val}});
            end

            // send response
            status = rsp_send_packet("OK");

            return(0);
        endfunction: rsp_reg_writeall

    ////////////////////////////////////////
    // RSP single register access
    ////////////////////////////////////////

        function automatic int rsp_reg_readone ();
            int status;
            string pkt;
            int unsigned idx;
            logic [XLEN-1:0] val;  // 4-state so GDB can iterpret 'x

            // read packet
            status = rsp_get_packet(pkt);

            // register index
            status = $sscanf(pkt, "p%h", idx);

            // swap byte order since they are sent LSB first
            if (stub_state.register) begin
                val = {<<8{dut_reg_read(idx)}};
            end else begin
                val = {<<8{shd.reg_read(idx)}};
            end
            case (XLEN)
                32: pkt = {pkt, $sformatf("%08h", val)};
                64: pkt = {pkt, $sformatf("%016h", val)};
            endcase

            // send response
            status = rsp_send_packet(pkt);

            return(1);
        endfunction: rsp_reg_readone

        function automatic int rsp_reg_writeone ();
            int status;
            string pkt;
            int unsigned idx;
            bit [XLEN-1:0] val;

            // read packet
            status = rsp_get_packet(pkt);

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
            // NOTE: register writes are always done to both DUT and shadow
            dut_reg_write(idx, {<<8{val}});
            shd.reg_write(idx, {<<8{val}});
        //    case (XLEN)
        //        32: $display("DEBUG: GPR[%0d] <= 32'h%08h", idx, val);
        //        64: $display("DEBUG: GPR[%0d] <= 64'h%016h", idx, val);
        //    endcase

            // send response
            status = rsp_send_packet("OK");

            return(1);
        endfunction: rsp_reg_writeone

    ////////////////////////////////////////
    // RSP breakpoints/watchpoints
    ////////////////////////////////////////

        function automatic int rsp_point_remove ();
            int status;
            string pkt;
            ptype_t ptype;
            SIZE_T  addr;
            pkind_t pkind;

            // read packet
            status = rsp_get_packet(pkt);

            // breakpoint/watchpoint
`ifdef VERILATOR
            status = $sscanf(pkt, "z%h,%h,%h", ptype, addr, pkind);
`else
            case (XLEN)
                32: status = $sscanf(pkt, "z%h,%8h,%h", ptype, addr, pkind);
                64: status = $sscanf(pkt, "z%h,%16h,%h", ptype, addr, pkind);
            endcase
`endif

            void'(shd.point_remove(ptype, addr, pkind));
            return(1);
        endfunction: rsp_point_remove

        function automatic int rsp_point_insert ();
            int status;
            string pkt;
            ptype_t ptype;
            SIZE_T  addr;
            pkind_t pkind;

            // read packet
            status = rsp_get_packet(pkt);

            // breakpoint/watchpoint
`ifdef VERILATOR
            status = $sscanf(pkt, "Z%h,%h,%h", ptype, addr, pkind);
`else
            case (XLEN)
                32: status = $sscanf(pkt, "Z%h,%8h,%h", ptype, addr, pkind);
                64: status = $sscanf(pkt, "Z%h,%16h,%h", ptype, addr, pkind);
            endcase
`endif

            void'(shd.point_insert(ptype, addr, pkind));
            return(1);
        endfunction: rsp_point_insert

    ///////////////////////////////////////
    // RSP step/continue
    ///////////////////////////////////////

        task rsp_forward_step;
            retired_t ret;

            // record (if not in replay mode)
            if (shd.cnt == shd.trc.size()-1) begin
                // perform DUT step
                dut_step(ret);
                shd.trc.push_back(ret);
            end
            // handle shadow and trace
            shd.forward();
        endtask: rsp_forward_step

        task rsp_step;
            int       status;
            string    pkt;
            SIZE_T    addr;
            int       sig;
            bit       jmp;

            // read packet
            status = rsp_get_packet(pkt);

            // signal/address
            case (pkt[0])
                "s": begin
                    status = $sscanf(pkt, "s%h", addr);
            //        shd.sig = SIGNONE;
                    jmp = (status == 1);
                end
                "S": begin
                    status = $sscanf(pkt, "S%h;%h", sig, addr);
                    shd.sig = signal_t'(sig);
                    jmp = (status == 2);
                end
            endcase

            // TODO handle PC write errors
            //dut_jump(addr);

            // forward step
            rsp_forward_step;

            // response packet
            status = rsp_stop_reply(shd.sig);
        endtask: rsp_step

        task rsp_continue ();
            byte ch [] = new[1];
            int status;
            string pkt;
            SIZE_T addr;
            int    sig;
            bit    jmp;

            // read packet
            status = rsp_get_packet(pkt);

            // signal/address
            case (pkt[0])
                "c": begin
                    status = $sscanf(pkt, "c%h", addr);
                    shd.sig = SIGNONE;
                    jmp = (status == 1);
                end
                "C": begin
                    status = $sscanf(pkt, "C%h;%h", sig, addr);
                    shd.sig = signal_t'(sig);
                    jmp = (status == 2);
                end
            endcase

            // TODO handle PC write errors
            //dut_jump(addr);

            // step forward
            do begin
                rsp_forward_step;

                status = socket_recv(ch, MSG_PEEK | MSG_DONTWAIT);

                // if empty
                if (status != 1) begin
                    // do nothing
                end
                // in case of Ctrl+C (character 0x03)
                else if (ch[0] == SIGQUIT) begin
                    shd.sig = SIGINT;
                    $display("DEBUG: Interrupt SIGQUIT (0x03) (Ctrl+c).");
                end
                // parse packet and loop back
                else begin
                    rsp_packet(ch);
                end
            end while (shd.sig == SIGNONE);

            // send response
            status = rsp_stop_reply(shd.sig);
        endtask: rsp_continue

    ////////////////////////////////////////
    // RSP reverse step/continue
    ////////////////////////////////////////

        function void rsp_backward_step;
            // record (if not replay)
            if (shd.cnt == -1) begin
                // DUT is still somewhere in the reset sequence
                // TODO: return some kind of error
                shd.sig = SIGTRAP;
                return;
            end
            else if (shd.cnt == 0) begin
                // already at the beginning of history, can't go further back
                // TODO: maybe there is a better option than a trap, check what QEMU does
                shd.sig = SIGTRAP;
                return;
            end else begin
                // handle shadow and trace
                shd.backward();
            end
        endfunction: rsp_backward_step

        task rsp_backward;
            byte ch [] = new[1];
            int status;
            string pkt;

            // read packet
            status = rsp_get_packet(pkt);

            // signal/address
            case (pkt[0:1])
                "bs": begin
                    // backward step
            //        shd.sig = SIGNONE;
                    rsp_backward_step;
                end
                "bc": begin
                    // backward continue
                    do begin
                        shd.sig = SIGNONE;
                        rsp_backward_step;

                        status = socket_recv(ch, MSG_PEEK | MSG_DONTWAIT);

                        // if empty
                        if (status != 1) begin
                            // do nothing
                        end
                        // in case of Ctrl+C (character 0x03)
                        else if (ch[0] == SIGQUIT) begin
                            shd.sig = SIGINT;
                            $display("DEBUG: Interrupt SIGQUIT (0x03) (Ctrl+c).");
                        end
                        // parse packet and loop back
                        else begin
                            rsp_packet(ch);
                        end
                    end while (shd.sig == SIGNONE);
                end
            endcase

            // send response
            status = rsp_stop_reply(shd.sig);
        endtask: rsp_backward

    ////////////////////////////////////////
    // RSP extended/reset/detach/kill
    ////////////////////////////////////////

        function automatic int rsp_extended ();
            int status;
            string pkt;

            // read packet
            status = rsp_get_packet(pkt);

            // set extended mode
            stub_state.extended = 1'b1;

            // send response
            status = rsp_send_packet("OK");

            return(0);
        endfunction: rsp_extended

        task rsp_reset ();
            int status;
            string pkt;

            // read packet
            status = rsp_get_packet(pkt);

            // perform RESET sequence
            dut_reset_assert();

            // do not send packet response here
        endtask: rsp_reset

        function automatic int rsp_detach ();
            int status;
            string pkt;

            // read packet
            status = rsp_get_packet(pkt);

            // send response (GDB cliend will close the socket connection)
            status = rsp_send_packet("OK");

            // re-initialize stub state
            stub_state = STUB_STATE_INIT;

            // stop HDL simulation, so the HDL simulator can render waveforms
            $info("GDB: detached, stopping simulation from within state %s.", shd.sig.name);
            $stop();
            // after user continues HDL simulation blocking wait for GDB client to reconnect
            $info("GDB: continuing stopped simulation, waiting for GDB to reconnect to.");
            // accept connection from GDB (blocking)
            if (stub_state.socket_port) begin
                status = socket_tcp_accept();
            end else begin
                status = socket_unix_accept();
            end

            return(0);
        endfunction: rsp_detach

        function automatic int rsp_kill ();
            int status;
            string pkt;

            // read packet
            status = rsp_get_packet(pkt);

            // finish simulation
            $finish();

            // do not send packet response here
            return(0);
        endfunction: rsp_kill

    ////////////////////////////////////////
    // RSP packet
    ////////////////////////////////////////

        task automatic rsp_packet (
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
            //        "x": status = rsp_mem_bin_read();
            //        "X": status = rsp_mem_bin_write();
                    "m": status = rsp_mem_read();
                    "M": status = rsp_mem_write();
                    "g": status = rsp_reg_readall();
                    "G": status = rsp_reg_writeall();
                    "p": status = rsp_reg_readone();
                    "P": status = rsp_reg_writeone();
                    "s",
                    "S":          rsp_step();
                    "c",
                    "C":          rsp_continue();
                    "b":          rsp_backward();
                    "?": status = rsp_signal();
                    "Q",
                    "q":          rsp_query_packet();
            //        "q": status = rsp_query_packet();
                    "v":          rsp_verbose_packet();
                    "z": status = rsp_point_remove();
                    "Z": status = rsp_point_insert();
                    "!": status = rsp_extended();
                    "R":          rsp_reset();
                    "D": status = rsp_detach();
                    "k": status = rsp_kill();
                    default: begin
                        string pkt;
                        // read packet
                        status = rsp_get_packet(pkt);
                        // for unsupported commands respond with empty packet
                        status = rsp_send_packet("");
                    end
                endcase
            end else begin
                $error("Unexpected sequence from degugger %p = \"%s\".", ch, ch);
            end
        endtask: rsp_packet

    ////////////////////////////////////////
    // GDB state machine
    ////////////////////////////////////////

        task gdb_fsm ();
            static byte ch [] = new[1];
            int status;

            forever begin
                // blocking socket read
                status = socket_recv(ch, MSG_PEEK);
                // parse packet and loop back
                rsp_packet(ch);
            end
        endtask: gdb_fsm

    endclass: gdb_server_stub

endpackage: gdb_server_stub_pkg
