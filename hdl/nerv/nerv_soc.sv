/*
 *  NERV -- Naive Educational RISC-V Processor
 *
 *  Copyright (C) 2020  Claire Xenia Wolf <claire@yosyshq.com>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

module nerv_soc #(
	parameter [31:0] RESET_ADDR = 32'h 0000_0000,
	parameter integer NUMREGS = 32
)(
	input clock,
	input reset,
	output reg [31:0] leds
);
	reg [31:0] mem [0:1023];

	wire stall = 0;
	wire trap;

	wire [31:0] imem_addr;
	reg  [31:0] imem_data;

	wire        dmem_valid;
	wire [31:0] dmem_addr;
	wire [3:0]  dmem_wstrb;
	wire [31:0] dmem_wdata;
	reg  [31:0] dmem_rdata;

	initial begin
		$readmemh("firmware.hex", mem);
	end

	always @(posedge clock)
		imem_data <= mem[imem_addr[31:2]];

	always @(posedge clock) begin
		if (dmem_valid) begin
			if (dmem_addr == 32'h 0100_0000) begin
				if (dmem_wstrb[0]) leds[ 7: 0] <= dmem_wdata[ 7: 0];
				if (dmem_wstrb[1]) leds[15: 8] <= dmem_wdata[15: 8];
				if (dmem_wstrb[2]) leds[23:16] <= dmem_wdata[23:16];
				if (dmem_wstrb[3]) leds[31:24] <= dmem_wdata[31:24];
			end else begin
				if (dmem_wstrb[0]) mem[dmem_addr[31:2]][ 7: 0] <= dmem_wdata[ 7: 0];
				if (dmem_wstrb[1]) mem[dmem_addr[31:2]][15: 8] <= dmem_wdata[15: 8];
				if (dmem_wstrb[2]) mem[dmem_addr[31:2]][23:16] <= dmem_wdata[23:16];
				if (dmem_wstrb[3]) mem[dmem_addr[31:2]][31:24] <= dmem_wdata[31:24];
			end
			dmem_rdata <= mem[dmem_addr[31:2]];
		end
	end

	nerv cpu (
		// system signals
		.clock     (clock     ),
		.reset     (reset     ),
		// control state machine
		.stall     (stall     ),
		.trap      (trap      ),
		// interrupt
        .irq       ('0        ),
        // IFU interface
		.imem_addr (imem_addr ),
		.imem_data (imem_data ),
        // LSU interface
		.dmem_valid(dmem_valid),
		.dmem_addr (dmem_addr ),
		.dmem_wstrb(dmem_wstrb),
		.dmem_wdata(dmem_wdata),
		.dmem_rdata(dmem_rdata)
	);

endmodule: nerv_soc
