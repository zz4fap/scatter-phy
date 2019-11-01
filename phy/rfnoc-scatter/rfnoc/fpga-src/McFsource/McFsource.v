
module McFsource #(
  parameter SR_ENABLE         = 0,
  parameter SR_SAMPLE_LEN_1MS = 1,
  parameter SR_SPP            = 2,
  parameter SR_CLK_DIV        = 3)
(
  input clk, input reset, input [15:0] src_sid, input [15:0] dst_sid,
  input set_stb, input [7:0] set_addr, input [31:0] set_data,
  output [31:0] o_tdata, output o_tlast, output o_tvalid, input o_tready, output [127:0] o_tuser
);

  wire enable;
  wire [15:0] sample_len_1ms;
  wire [15:0] spp;
  wire [15:0] clk_div;

  setting_reg #(.my_addr(SR_ENABLE), .awidth(8), .width(1), .at_reset(0)) sr_enable (
    .clk(clk), .rst(reset),
    .strobe(set_stb), .addr(set_addr), .in(set_data), .out(enable), .changed());

  setting_reg #(.my_addr(SR_SAMPLE_LEN_1MS), .awidth(8), .width(16), .at_reset(11520)) sr_sample_len_1ms (
    .clk(clk), .rst(reset),
    .strobe(set_stb), .addr(set_addr), .in(set_data), .out(sample_len_1ms), .changed());

  setting_reg #(.my_addr(SR_SPP), .awidth(8), .width(16), .at_reset(768)) sr_spp (
    .clk(clk), .rst(reset),
    .strobe(set_stb), .addr(set_addr), .in(set_data), .out(spp), .changed());

  setting_reg #(.my_addr(SR_CLK_DIV), .awidth(8), .width(16), .at_reset(16)) sr_clk_div (
    .clk(clk), .rst(reset),
    .strobe(set_stb), .addr(set_addr), .in(set_data), .out(clk_div), .changed());
    
  // McFsource Finite State Machine
  reg [1:0]  McFsource_state;
  reg [7:0]  clk_div_timer;
  reg [15:0] bram_addr;
  reg [15:0] tlast_timer;
  reg        bram_ena;
  reg        tvalid_int;
  reg        tlast_int;

  localparam WAIT       = 0;
  localparam READ_DATA1 = 1;
  localparam READ_DATA2 = 2;
  localparam DATA_READY = 3;
  
  always @(posedge clk)
  if(reset) begin
    McFsource_state <= WAIT;
    clk_div_timer <= 0;
    bram_ena <= 0;
    bram_addr <= -1;
    tlast_timer <= -1;
    tvalid_int <= 0;
    tlast_int <= 0;
  end else begin
    if(enable) begin
      case(McFsource_state)

        // Wait until clk_div timer expires 
        WAIT: begin
          // Clock divide timer
          if (clk_div_timer == clk_div - 1) begin
            McFsource_state <= READ_DATA1;
            clk_div_timer <= 0;
            bram_ena <= 1;

            // Block RAM address counter
            if (bram_addr == sample_len_1ms - 1) begin
              bram_addr <= 0;
            end else begin
              bram_addr <= bram_addr + 1;
            end

            // tlast timer
            if (tlast_timer == spp - 1) begin
              tlast_timer <= 0;
            end else begin
              tlast_timer <= tlast_timer + 1;
            end
          end else begin
            clk_div_timer <= clk_div_timer + 1;
          end
        end
      
        // State to read data. After two clock cycles, data becomes available 
        READ_DATA1: begin
          McFsource_state <= READ_DATA2;
        end

        READ_DATA2: begin
          McFsource_state <= DATA_READY;
          tvalid_int <= 1;
          if(tlast_timer == spp - 1) begin
            tlast_int <= 1;
          end
        end
      
        // Data is ready
        DATA_READY: begin
          if(o_tready) begin
            McFsource_state <= WAIT;
            bram_ena <= 0;
            tvalid_int <= 0;
            tlast_int <= 0;
          end
        end

      endcase
    end
  end
  
  // McFsource block RAM memory
  McF_block_RAM mem (
    .clka(clk),                 // input wire clka
    .ena(bram_ena),             // input wire ena
    .wea(),                     // input wire [0 : 0] wea
    .addra(bram_addr),          // input wire [14 : 0] addra
    .dina(),                    // input wire [31 : 0] dina
    .douta(o_tdata)             // output wire [31 : 0] douta
  );
  
  assign o_tvalid = tvalid_int;
  assign o_tlast  = tlast_int;

  wire [15:0] payload_length = spp << 2;	// Payload length = 4 * IQ Samples per packet
  cvita_hdr_encoder cvita_hdr_encoder (
    .pkt_type(2'd0), .eob(1'b0), .has_time(1'b0),
    .seqnum(12'd0), .payload_length(payload_length), .src_sid(src_sid), .dst_sid(dst_sid),
    .vita_time(64'd0),
    .header(o_tuser));

endmodule // McFsource
