module pkt_avg #(
  parameter MAX_PKT_SIZE_LOG2 = 14,
  parameter MAX_AVG_SIZE_LOG2 = 10,
  parameter WIDTH = 32
)(
  input clk, input reset,
  input [WIDTH-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
  output[WIDTH-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready,
  input [MAX_PKT_SIZE_LOG2 + MAX_AVG_SIZE_LOG2 - 1:0] i_config_tdata, input i_config_tvalid, output i_config_tready
);


assign i_tready = 1'b1;
assign i_config_tready = 1'b1;


reg [MAX_PKT_SIZE_LOG2-1:0] pkt_size_reg, pkt_size;
reg [MAX_AVG_SIZE_LOG2-1:0] avg_size_reg, avg_size;
reg data_valid_int;
reg [WIDTH-1:0] data_int;
reg [MAX_PKT_SIZE_LOG2-1:0] wr_ptr, rd_ptr;
reg [MAX_AVG_SIZE_LOG2-1:0] pkt_cnt;
wire [WIDTH+MAX_AVG_SIZE_LOG2-1:0] ram_in_data, ram_out_data, prev_sum;
//reg [WIDTH+MAX_AVG_SIZE_LOG2-1:0] ram_out_data_r1;
reg [WIDTH + MAX_AVG_SIZE_LOG2-1:0] current_sum;
reg [WIDTH-1:0] avg;
reg avg_valid, avg_tlast;

// state machine
reg [2:0] pkt_avg_state;
localparam ST_IDLE = 0;
localparam ST_FIRST_PKT = 1;
localparam ST_NOT_FIRST_PKT = 2;


always @(posedge clk)
  if(reset) begin
    pkt_avg_state <= ST_IDLE;
    pkt_size <= 0;  // set default values ??
    avg_size <= 0;
  end else
    case(pkt_avg_state)
      ST_IDLE:
        begin
        pkt_size <= pkt_size_reg;
        avg_size <= avg_size_reg;
        if(i_tvalid)
          pkt_avg_state <= ST_FIRST_PKT;
        end
      ST_FIRST_PKT:
        if(data_valid_int & (wr_ptr == pkt_size-1))
           pkt_avg_state <= ST_NOT_FIRST_PKT;

      ST_NOT_FIRST_PKT:
        if(data_valid_int & (wr_ptr == pkt_size-1) & (pkt_cnt == avg_size - 1))
          pkt_avg_state <= ST_FIRST_PKT;
    endcase


// other sequential and combinatorial logic

// register incoming parameters


always @(posedge clk)
  if(reset) begin
    pkt_size_reg <= 0;
    avg_size_reg <= 0;
  end else begin 
    if(i_config_tvalid) begin
       avg_size_reg <= i_config_tdata[MAX_PKT_SIZE_LOG2 + MAX_AVG_SIZE_LOG2 - 1: MAX_PKT_SIZE_LOG2];
       pkt_size_reg <= i_config_tdata[MAX_PKT_SIZE_LOG2-1:0];
    end
  end

// register i_tvalid, i_tdata

always @(posedge clk)
  if(reset) begin
    data_valid_int <= 1'b0;
    data_int <= 0;
  end else begin
    data_valid_int <= i_tvalid;
    if(i_tvalid)
       data_int <= i_tdata;
  end
    
// update rd_ptr, wr_ptr

always @(posedge clk)
  if(reset) begin
    wr_ptr <= 0;
    rd_ptr <= 2; // rd_ptr is always 1 ahead of wr_ptr so that we don't read from and write to the same location
  end else begin
    if(data_valid_int) begin
      if(wr_ptr == pkt_size - 1)
         wr_ptr <= 0;
      else
         wr_ptr <= wr_ptr + 1;

      if(rd_ptr == pkt_size - 1)
         rd_ptr <= 0;
      else
         rd_ptr <= rd_ptr + 1;
    end
  end

// update pkt_cnt

always @(posedge clk)
  if(reset) begin
    pkt_cnt <= 0;
  end else begin
    if(data_valid_int & (wr_ptr == pkt_size-1)) begin
      if(pkt_cnt == avg_size - 1)
         pkt_cnt <= 0;
      else
         pkt_cnt <= pkt_cnt + 1;
    end
  end    
     
// update ram_in_data, current_sum    
assign ram_in_data = (pkt_avg_state == ST_FIRST_PKT)? data_int : current_sum;
assign prev_sum = (pkt_avg_state == ST_FIRST_PKT)? 0 : ram_out_data;

always @(posedge clk)
  if(reset) begin
    current_sum <= 0;    
  end else begin
    if(i_tvalid) 
      current_sum <= i_tdata + prev_sum;
  end

// register ram_out_data to be used for current_sum
/*always @(posedge clk)
  if(reset) begin
    ram_out_data_r1 <= 0;
  end else begin
    if(data_valid_int)
      ram_out_data_r1 <= ram_out_data;
  end*/


// divide by avg_size - assuming avg_size is a power of 2
always @(posedge clk)
  if(reset) begin
    avg <= 0;
    avg_valid <= 1'b0; 
    avg_tlast <= 1'b0;
  end else begin
    case (avg_size)
      2: avg <= current_sum >> 1;
      4: avg <= current_sum >> 2;
      8: avg <= current_sum >> 3;
      16: avg <= current_sum >> 4;
      32: avg <= current_sum >> 5;
      64: avg <= current_sum >> 6;
      128: avg <= current_sum >> 7;
      256: avg <= current_sum >> 8;
      512: avg <= current_sum >> 9;
      1024: avg <= current_sum >> 10;
    endcase
    if(data_valid_int & (pkt_cnt == avg_size -1))
      avg_valid <= 1'b1;
    else
      avg_valid <= 1'b0;

    if(data_valid_int & (pkt_cnt == avg_size -1) & (wr_ptr == pkt_size - 1))
      avg_tlast <= 1'b1;
    else
      avg_tlast <= 1'b0;
   
  end


// dual port RAM to store accumulated packet data
ram_2port #(.DWIDTH(WIDTH + MAX_AVG_SIZE_LOG2), .AWIDTH(MAX_PKT_SIZE_LOG2))
inst_ram(
         .clka(clk), .ena(data_valid_int), .wea(1'b1), .addra(wr_ptr), .dia(ram_in_data), .doa(),
         .clkb(clk), .enb(data_valid_int), .web(1'b0), .addrb(rd_ptr), .dib(), .dob(ram_out_data));

// FIFO to hold output packet
wire int_tlast;
axi_fifo #(.WIDTH(WIDTH+1), .SIZE(MAX_PKT_SIZE_LOG2))
inst_fifo(
         .clk(clk), .reset(reset), .clear(reset),
         .i_tdata({avg_tlast, avg}), .i_tvalid(avg_valid), .i_tready(),
         .o_tdata({int_tlast, o_tdata}), .o_tvalid(o_tvalid), .o_tready(o_tready),
         .space(), .occupied());

assign o_tlast = o_tvalid & int_tlast;
  
endmodule
  
