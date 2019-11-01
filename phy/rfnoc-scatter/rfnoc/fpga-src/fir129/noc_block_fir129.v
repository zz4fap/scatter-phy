
//
/* 
 * Copyright 2018 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

//
module noc_block_fir129 #(
  parameter NOC_ID = 64'hF129000000000000,
  parameter STR_SINK_FIFOSIZE = 11)
(
  input bus_clk, input bus_rst,
  input ce_clk, input ce_rst,
  input  [63:0] i_tdata, input  i_tlast, input  i_tvalid, output i_tready,
  output [63:0] o_tdata, output o_tlast, output o_tvalid, input  o_tready,
  output [63:0] debug
);

  ////////////////////////////////////////////////////////////
  //
  // RFNoC Shell
  //
  ////////////////////////////////////////////////////////////
  wire [31:0] set_data;
  wire [7:0]  set_addr;
  wire        set_stb;
  reg  [63:0] rb_data;
  wire [7:0]  rb_addr;

  wire [63:0] cmdout_tdata, ackin_tdata;
  wire        cmdout_tlast, cmdout_tvalid, cmdout_tready, ackin_tlast, ackin_tvalid, ackin_tready;

  wire [63:0] str_sink_tdata, str_src_tdata;
  wire        str_sink_tlast, str_sink_tvalid, str_sink_tready, str_src_tlast, str_src_tvalid, str_src_tready;

  wire [15:0] src_sid;
  wire [15:0] next_dst_sid, resp_out_dst_sid;
  wire [15:0] resp_in_dst_sid;

  wire        clear_tx_seqnum;

  noc_shell #(
    .NOC_ID(NOC_ID),
    .STR_SINK_FIFOSIZE(STR_SINK_FIFOSIZE))
  noc_shell (
    .bus_clk(bus_clk), .bus_rst(bus_rst),
    .i_tdata(i_tdata), .i_tlast(i_tlast), .i_tvalid(i_tvalid), .i_tready(i_tready),
    .o_tdata(o_tdata), .o_tlast(o_tlast), .o_tvalid(o_tvalid), .o_tready(o_tready),
    // Computer Engine Clock Domain
    .clk(ce_clk), .reset(ce_rst),
    // Control Sink
    .set_data(set_data), .set_addr(set_addr), .set_stb(set_stb), .set_time(), .set_has_time(),
    .rb_stb(1'b1), .rb_data(rb_data), .rb_addr(rb_addr),
    // Control Source
    .cmdout_tdata(cmdout_tdata), .cmdout_tlast(cmdout_tlast), .cmdout_tvalid(cmdout_tvalid), .cmdout_tready(cmdout_tready),
    .ackin_tdata(ackin_tdata), .ackin_tlast(ackin_tlast), .ackin_tvalid(ackin_tvalid), .ackin_tready(ackin_tready),
    // Stream Sink
    .str_sink_tdata(str_sink_tdata), .str_sink_tlast(str_sink_tlast), .str_sink_tvalid(str_sink_tvalid), .str_sink_tready(str_sink_tready),
    // Stream Source
    .str_src_tdata(str_src_tdata), .str_src_tlast(str_src_tlast), .str_src_tvalid(str_src_tvalid), .str_src_tready(str_src_tready),
    // Stream IDs set by host
    .src_sid(src_sid),                   // SID of this block
    .next_dst_sid(next_dst_sid),         // Next destination SID
    .resp_in_dst_sid(resp_in_dst_sid),   // Response destination SID for input stream responses / errors
    .resp_out_dst_sid(resp_out_dst_sid), // Response destination SID for output stream responses / errors
    // Misc
    .vita_time('d0), .clear_tx_seqnum(clear_tx_seqnum),
    .debug(debug));

  ////////////////////////////////////////////////////////////
  //
  // AXI Wrapper
  // Convert RFNoC Shell interface into AXI stream interface
  //
  ////////////////////////////////////////////////////////////
  wire [31:0] m_axis_data_tdata;
  wire        m_axis_data_tlast;
  wire        m_axis_data_tvalid;
  wire        m_axis_data_tready;

  wire [31:0] s_axis_data_tdata;
  wire        s_axis_data_tlast;
  wire        s_axis_data_tvalid;
  wire        s_axis_data_tready;

  axi_wrapper #(
    .SIMPLE_MODE(1))
  axi_wrapper (
    .clk(ce_clk), .reset(ce_rst),
    .bus_clk(bus_clk), .bus_rst(bus_rst),
    .clear_tx_seqnum(clear_tx_seqnum),
    .next_dst(next_dst_sid),
    .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
    .i_tdata(str_sink_tdata), .i_tlast(str_sink_tlast), .i_tvalid(str_sink_tvalid), .i_tready(str_sink_tready),
    .o_tdata(str_src_tdata), .o_tlast(str_src_tlast), .o_tvalid(str_src_tvalid), .o_tready(str_src_tready),
    .m_axis_data_tdata(m_axis_data_tdata),
    .m_axis_data_tlast(m_axis_data_tlast),
    .m_axis_data_tvalid(m_axis_data_tvalid),
    .m_axis_data_tready(m_axis_data_tready),
    .m_axis_data_tuser(),
    .s_axis_data_tdata(s_axis_data_tdata),
    .s_axis_data_tlast(s_axis_data_tlast),
    .s_axis_data_tvalid(s_axis_data_tvalid),
    .s_axis_data_tready(s_axis_data_tready),
    .s_axis_data_tuser(),
    .m_axis_config_tdata(),
    .m_axis_config_tlast(),
    .m_axis_config_tvalid(),
    .m_axis_config_tready(),
    .m_axis_pkt_len_tdata(),
    .m_axis_pkt_len_tvalid(),
    .m_axis_pkt_len_tready());

  ////////////////////////////////////////////////////////////
  //
  // User code
  //
  ////////////////////////////////////////////////////////////
  // NoC Shell registers 0 - 127,
  // User register address space starts at 128
  localparam SR_USER_REG_BASE = 128;

  // Control Source Unused
  assign cmdout_tdata  = 64'd0;
  assign cmdout_tlast  = 1'b0;
  assign cmdout_tvalid = 1'b0;
  assign ackin_tready  = 1'b1;

  wire [79:0] s_axis_fir_tdata;
  wire        s_axis_fir_tlast;
  wire        s_axis_fir_tvalid;
  wire        s_axis_fir_tready;
  wire [15:0] m_axis_fir_reload_tdata;
  wire        m_axis_fir_reload_tvalid, m_axis_fir_reload_tready, m_axis_fir_reload_tlast;
  wire [7:0]  m_axis_fir_config_tdata;
  wire        m_axis_fir_config_tvalid, m_axis_fir_config_tready;

  localparam SR_RELOAD       = 128;
  localparam SR_RELOAD_LAST  = 129;
  localparam SR_CONFIG       = 130;
  localparam RB_NUM_TAPS     = 0;

  localparam NUM_TAPS = 129;

  // Settings registers
  //
  // - The settings register bus is a simple strobed interface.
  // - Transactions include both a write and a readback.
  // - The write occurs when set_stb is asserted.
  //   The settings register with the address matching set_addr will
  //   be loaded with the data on set_data.
  // - Readback occurs when rb_stb is asserted. The read back strobe
  //   must assert at least one clock cycle after set_stb asserts /
  //   rb_stb is ignored if asserted on the same clock cycle of set_stb.
  //   Example valid and invalid timing:
  //              __    __    __    __
  //   clk     __|  |__|  |__|  |__|  |__
  //               _____
  //   set_stb ___|     |________________
  //                     _____
  //   rb_stb  _________|     |__________     (Valid)
  //                           _____
  //   rb_stb  _______________|     |____     (Valid)
  //           __________________________
  //   rb_stb                                 (Valid if readback data is a constant)
  //               _____
  //   rb_stb  ___|     |________________     (Invalid / ignored, same cycle as set_stb)
  //

  
  // FIR filter coefficient reload bus
  // (see Xilinx FIR Filter Compiler documentation)
  axi_setting_reg #(
    .ADDR(SR_RELOAD),
    .USE_ADDR_LAST(1),
    .ADDR_LAST(SR_RELOAD_LAST),
    .WIDTH(16),
    .USE_FIFO(1),
    .FIFO_SIZE(8))
  set_coeff (
    .clk(ce_clk),
    .reset(ce_rst),
    .set_stb(set_stb),
    .set_addr(set_addr),
    .set_data(set_data),
    .o_tdata(m_axis_fir_reload_tdata),
    .o_tlast(m_axis_fir_reload_tlast),
    .o_tvalid(m_axis_fir_reload_tvalid),
    .o_tready(m_axis_fir_reload_tready));

  // FIR filter config bus
  // (see Xilinx FIR Filter Compiler documentation)
  axi_setting_reg #(
    .ADDR(SR_CONFIG),
    .WIDTH(8))
  set_config (
    .clk(ce_clk),
    .reset(ce_rst),
    .set_stb(set_stb),
    .set_addr(set_addr),
    .set_data(set_data),
    .o_tdata(m_axis_fir_config_tdata),
    .o_tlast(),
    .o_tvalid(m_axis_fir_config_tvalid),
    .o_tready(m_axis_fir_config_tready));

  // Xilinx FIR Filter IP Core
  axi_fir129 inst_axi_fir (
    .aresetn(~(ce_rst | clear_tx_seqnum)), .aclk(ce_clk),
    .s_axis_data_tdata(m_axis_data_tdata),
    .s_axis_data_tlast(m_axis_data_tlast),
    .s_axis_data_tvalid(m_axis_data_tvalid),
    .s_axis_data_tready(m_axis_data_tready),
    .m_axis_data_tdata(s_axis_fir_tdata),
    .m_axis_data_tlast(s_axis_fir_tlast),
    .m_axis_data_tvalid(s_axis_fir_tvalid),
    .m_axis_data_tready(s_axis_fir_tready),
    .s_axis_config_tdata(m_axis_fir_config_tdata),
    .s_axis_config_tvalid(m_axis_fir_config_tvalid),
    .s_axis_config_tready(m_axis_fir_config_tready),
    .s_axis_reload_tdata(m_axis_fir_reload_tdata),
    .s_axis_reload_tvalid(m_axis_fir_reload_tvalid),
    .s_axis_reload_tready(m_axis_fir_reload_tready),
    .s_axis_reload_tlast(m_axis_fir_reload_tlast));

  // Clip extra bits for bit growth and round
  axi_round_and_clip_complex #(
    .WIDTH_IN(39),
    .WIDTH_OUT(16),
    .CLIP_BITS(8)) // clog2(129) = 8. Assuming that the filter gain <=1 i.e, sum of filter coefficients <= 2^(COEFF_WIDTH-1) for signed coefficients
  inst_axi_round_and_clip (
    .clk(ce_clk), .reset(ce_rst | clear_tx_seqnum),
    .i_tdata({s_axis_fir_tdata[78:40],s_axis_fir_tdata[38:0]}),
    .i_tlast(s_axis_fir_tlast),
    .i_tvalid(s_axis_fir_tvalid),
    .i_tready(s_axis_fir_tready),
    .o_tdata(s_axis_data_tdata),
    .o_tlast(s_axis_data_tlast),
    .o_tvalid(s_axis_data_tvalid),
    .o_tready(s_axis_data_tready));


  // Readback registers
  // rb_stb set to 1'b1 on NoC Shell
  // Readback register for number of FIR filter taps
  always @*
    case(rb_addr)
      RB_NUM_TAPS : rb_data <= {NUM_TAPS};
      default     : rb_data <= 64'h0BADC0DE0BADC0DE;
  endcase


endmodule
