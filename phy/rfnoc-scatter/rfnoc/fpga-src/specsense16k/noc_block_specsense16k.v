
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
module noc_block_specsense16k #(
  parameter NOC_ID = 64'hFF77000000000000,
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
  localparam NUM_AXI_CONFIG_BUS = 2; // 1 config bus for FFT block and the other for the ave
  // Register addresses used for AXI Config are 129,130, 131,132

  wire [31:0] m_axis_data_tdata;
  wire        m_axis_data_tlast;
  wire        m_axis_data_tvalid;
  wire        m_axis_data_tready;

  wire [31:0] s_axis_data_tdata;
  wire        s_axis_data_tlast;
  wire        s_axis_data_tvalid;
  wire        s_axis_data_tready;
  wire [127:0] s_axis_data_tuser;

  wire [63:0] m_axis_config_tdata;
  wire [1:0]  m_axis_config_tvalid;
  wire [1:0]  m_axis_config_tready;


  axi_wrapper #(
    .SIMPLE_MODE(0),
    .NUM_AXI_CONFIG_BUS(NUM_AXI_CONFIG_BUS),
    .RESIZE_OUTPUT_PACKET(1))
  inst_axi_wrapper (
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
    .m_axis_data_tuser(m_axis_data_tuser),
    .s_axis_data_tdata(s_axis_data_tdata),
    .s_axis_data_tlast(s_axis_data_tlast),
    .s_axis_data_tvalid(s_axis_data_tvalid),
    .s_axis_data_tready(s_axis_data_tready),
    .s_axis_data_tuser(s_axis_data_tuser),
    .m_axis_config_tdata(m_axis_config_tdata),
    .m_axis_config_tlast(),
    .m_axis_config_tvalid(m_axis_config_tvalid),
    .m_axis_config_tready(m_axis_config_tready),
    .m_axis_pkt_len_tdata(),
    .m_axis_pkt_len_tvalid(),
    .m_axis_pkt_len_tready());


  ////////////////////////////////////////////////////////////
  //
  // User code
  //
  ////////////////////////////////////////////////////////////
  wire [31:0] m_fft_config_tdata;
  wire        m_fft_config_tvalid;
  wire        m_fft_config_tready;

  wire [31:0] m_avg_config_tdata;
  wire        m_avg_config_tvalid;
  wire        m_avg_config_tready;

  assign m_fft_config_tdata = m_axis_config_tdata[31:0];
  assign m_avg_config_tdata = m_axis_config_tdata[63:32];

  assign m_fft_config_tvalid = m_axis_config_tvalid[0];
  assign m_avg_config_tvalid = m_axis_config_tvalid[1];

  assign m_axis_config_tready = {m_avg_config_tready, m_fft_config_tready};

  // Control Source Unused
  assign cmdout_tdata  = 64'd0;
  assign cmdout_tlast  = 1'b0;
  assign cmdout_tvalid = 1'b0;
  assign ackin_tready  = 1'b1;

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

  // NoC Shell registers 0 - 127,
  // User register address space starts at 128

  localparam SR_USER_REG_BASE    = 128;
  localparam SR_AXI_CONFIG_BASE  = SR_USER_REG_BASE + 1;

  localparam [7:0] SR_FFT_RESET     = 133; // 129 - 132 used for AXI config
  localparam [7:0] SR_AVG_SIZE      = 134;
  localparam MAX_FFT_SIZE_LOG2      = 14; // 16k FFT
  localparam MAX_AVG_SIZE_LOG2      = 10;

  wire fft_reset;
  setting_reg #(
    .my_addr(SR_FFT_RESET), .awidth(8), .width(1))
  sr_fft_reset (
    .clk(ce_clk), .rst(ce_rst),
    .strobe(set_stb), .addr(set_addr), .in(set_data), .out(fft_reset), .changed());

  wire [MAX_AVG_SIZE_LOG2-1:0] avg_size;
  wire [64 - MAX_AVG_SIZE_LOG2 -1 :0] avg_size_padding;
  setting_reg #(
    .my_addr(SR_AVG_SIZE), .awidth(8), .width(MAX_AVG_SIZE_LOG2))
  sr_avg_size (
    .clk(ce_clk), .rst(ce_rst),
    .strobe(set_stb), .addr(set_addr), .in(set_data), .out(avg_size), .changed());


  // specsense16k block averages spectral energy over time. Since input output ratio is N:1 where N is the averaging size, axi_wrapper is not used in simple mode. 
  // Header is provided by user code. Time stamp for an output packet is same as the time stamp of the first packet of samples used for obtaining that average output packet.
  // Modify packet header data, same as input except SRC / DST SID fields.

  wire [127:0] modified_hdr;

  cvita_hdr_modify cvita_hdr_modify (
    .header_in(m_axis_data_tuser),
    .header_out(modified_hdr),
    .use_pkt_type(1'b0),       .pkt_type(),
    .use_has_time(1'b0),       .has_time(),
    .use_eob(1'b0),            .eob(),
    .use_seqnum(1'b0),         .seqnum(),
    .use_length(1'b0),         .length(),
    .use_payload_length(1'b1), .payload_length(1024), // packets get fragmented for fft_size > 256 (> 1024 bytes of data)
    .use_src_sid(1'b1),        .src_sid(src_sid),
    .use_dst_sid(1'b1),        .dst_sid(next_dst_sid),
    .use_vita_time(1'b0),      .vita_time());

   reg m_axis_data_tlast_reg;
   always @(posedge ce_clk) begin
       if (ce_rst | fft_reset) begin
         m_axis_data_tlast_reg <= 1'b0;
       end else begin
         m_axis_data_tlast_reg <= m_axis_data_tlast;
       end
   end

   wire hdr_pulse;
   assign hdr_pulse = m_axis_data_tlast &(~ m_axis_data_tlast_reg);

   reg [MAX_AVG_SIZE_LOG2-1 :0] hdr_pulse_cnt;
   always @(posedge ce_clk) begin
       if (ce_rst | fft_reset) begin
         hdr_pulse_cnt <= 0;
       end else begin
         if(hdr_pulse) begin
            if(hdr_pulse_cnt == avg_size - 1) begin
              hdr_pulse_cnt <= 0;
            end else begin
              hdr_pulse_cnt <= hdr_pulse_cnt + 1; // count incoming packets
            end
         end
       end
   end

   wire first_hdr, store_modified_hdr;
   assign first_hdr = (hdr_pulse_cnt == 0);
   assign store_modified_hdr = first_hdr & hdr_pulse; // store header of 1st, (N+1)th, (2N+1)th ... incoming packets (N is the averaging size)

   // FIFO for storing modified header
   axi_fifo #( .WIDTH(128), .SIZE(2))
   inst_hdr_fifo(
    .clk(ce_clk), .reset(ce_rst | fft_reset), .clear(ce_rst | fft_reset),
    .i_tdata(modified_hdr),
    .i_tvalid(store_modified_hdr),
    .i_tready(),
    .o_tdata(s_axis_data_tuser),
    .o_tvalid(),
    .o_tready(s_axis_data_tlast & s_axis_data_tvalid & s_axis_data_tready), // output the next header when the current packet goes out
    .space(),
    .occupied());

  
  // FFT instance
  wire [31:0] fft_data_o_tdata;
  wire        fft_data_o_tlast;
  wire        fft_data_o_tvalid;
  wire        fft_data_o_tready;
  wire [23:0] fft_data_o_tuser; // overflow bit (1) + xk (14). xk is padded to 16 bits. 7 more bits of padding to make tuser 8-bit aligned.


   axi_fft_16k inst_axi_fft (
    .aclk(ce_clk), .aresetn(~(ce_rst | fft_reset)),
    .s_axis_data_tvalid(m_axis_data_tvalid),
    .s_axis_data_tready(m_axis_data_tready),
    .s_axis_data_tlast(m_axis_data_tlast),
    .s_axis_data_tdata({m_axis_data_tdata[15:0], m_axis_data_tdata[31:16]}), // m_axis_data_tdata
    .m_axis_data_tvalid(fft_data_o_tvalid),
    .m_axis_data_tready(fft_data_o_tready),
    .m_axis_data_tlast(fft_data_o_tlast),
    .m_axis_data_tdata(fft_data_o_tdata),
    .m_axis_data_tuser(fft_data_o_tuser), // FFT index
    .m_axis_status_tready(1'b1),
    .s_axis_config_tdata(m_fft_config_tdata[23:0]),// scaling_sch (14) + fwd/inv (1) + log(fft_size) (8) = 23 + 1(padding to end on a byte boundary) = 24
    .s_axis_config_tvalid(m_fft_config_tvalid),
    .s_axis_config_tready(m_fft_config_tready));

   // FFT Magnitude computation
   wire [31:0] fft_mag_tdata;
   wire fft_mag_tlast;
   wire fft_mag_tvalid;
   wire fft_mag_tready;

   complex_to_magsq inst_complex_to_magsq(
    .clk(ce_clk), .reset(ce_rst | fft_reset), .clear(ce_rst | fft_reset),
    .i_tdata(fft_data_o_tdata), .i_tlast(fft_data_o_tlast), .i_tvalid(fft_data_o_tvalid), .i_tready(fft_data_o_tready),
    .o_tdata(fft_mag_tdata), .o_tlast(fft_mag_tlast), .o_tvalid(fft_mag_tvalid), .o_tready(fft_mag_tready));

   // FFT Averaging 
   pkt_avg #(.WIDTH(32), .MAX_PKT_SIZE_LOG2(MAX_FFT_SIZE_LOG2), .MAX_AVG_SIZE_LOG2(MAX_AVG_SIZE_LOG2))
   inst_fft_mag_avg(
    .clk(ce_clk), .reset(ce_rst | fft_reset),
    .i_tdata(fft_mag_tdata), .i_tlast(fft_mag_tlast), .i_tvalid(fft_mag_tvalid), .i_tready(fft_mag_tready),
    .o_tdata(s_axis_data_tdata), .o_tlast(s_axis_data_tlast), .o_tvalid(s_axis_data_tvalid), .o_tready(s_axis_data_tready),
    .i_config_tdata(m_avg_config_tdata[23:0]), .i_config_tvalid(m_avg_config_tvalid), .i_config_tready(m_avg_config_tready));


  // Readback registers
  assign avg_size_padding = 0;
  always @*
    case(rb_addr)
      2'd0    : rb_data <= {63'd0, fft_reset};
      2'd1    : rb_data <= {avg_size_padding, avg_size};
      default : rb_data <= 64'h0BADC0DE0BADC0DE;
  endcase

endmodule
