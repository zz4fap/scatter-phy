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

`timescale 1ns/1ps
`define NS_PER_TICK 1
`define NUM_TEST_CASES 7

`include "sim_exec_report.vh"
`include "sim_clks_rsts.vh"
`include "sim_rfnoc_lib.svh"

typedef logic[31:0] sample_t[$];


module noc_block_specsense2k_tb();

function sample_t get_random_complex_samples(int num_samples);
  sample_t sample;
  logic[15:0] re, im;
  begin
    for(int i = 0; i<num_samples; i++)
    begin
      re = $random;
      im = $random;
      sample[i] = {re,im};
    end
  end
  return sample;
endfunction

function cvita_payload_t get_payload(sample_t sample, int num_samples);
  cvita_payload_t payload;
  begin
    for(int i = 0; i<num_samples/2; i++)
    begin
      payload[i] = {sample[2*i], sample[2*i + 1]};
      if((2*i + 1) == num_samples-2) // when num_samples is odd
      begin
        payload[i+1] = {sample[2*i + 2],32'd0};
      end
    end
  end
  return payload;
endfunction


  `TEST_BENCH_INIT("noc_block_specsense2k",`NUM_TEST_CASES,`NS_PER_TICK);
  localparam BUS_CLK_PERIOD = $ceil(1e9/166.67e6);
  localparam CE_CLK_PERIOD  = $ceil(1e9/200e6);
  localparam NUM_CE         = 1;  // Number of Computation Engines / User RFNoC blocks to simulate
  localparam NUM_STREAMS    = 1;  // Number of test bench streams
  `RFNOC_SIM_INIT(NUM_CE, NUM_STREAMS, BUS_CLK_PERIOD, CE_CLK_PERIOD);
  `RFNOC_ADD_BLOCK(noc_block_specsense2k, 0);

   int MAX_PKT_SIZE = 256;
  // FFT specific settings
  int FFT_SIZE = 64;
  int AVG_SIZE = 8;
  wire [13:0] FFT_SIZE_REG = (FFT_SIZE);
  wire [9:0] AVG_SIZE_REG = (AVG_SIZE);

  int FFT_BIN         = FFT_SIZE/8;               // 1/8 sample rate freq
  wire [7:0] fft_size_log2   = $clog2(FFT_SIZE);        // Set FFT size
  wire fft_direction         = 1;                       // Set FFT direction to forward (i.e. DFT[x(n)] => X(k))
  wire [15:0] fft_scale      = 16'b0110101010101010;        // Conservative scaling of 1/N
  // Padding of the control word depends on the FFT options enabled
  wire [24:0] fft_ctrl_word  = {fft_scale, fft_direction, fft_size_log2};

  int NUM_ITERATIONS  = 17;
  int PKTS_PER_FFT = $ceil(FFT_SIZE / MAX_PKT_SIZE); // 1 if FFT_SIZE <= 256
  int NUM_OUT_PKTS = ((NUM_ITERATIONS+1)/(AVG_SIZE+1)) * PKTS_PER_FFT;

  sample_t sample;
  cvita_payload_t payload;

  localparam SPP = 16; // Samples per packet

  /********************************************************
  ** Verification
  ********************************************************/
  initial begin : tb_main
    logic [63:0] readback;
    logic [31:0] avg_mag;
    logic last;

    /********************************************************
    ** Test 1 -- Reset
    ********************************************************/
    `TEST_CASE_START("Wait for Reset");
    while (bus_rst) @(posedge bus_clk);
    while (ce_rst) @(posedge ce_clk);
    `TEST_CASE_DONE(~bus_rst & ~ce_rst);

    /********************************************************
    ** Test 2 -- Check for correct NoC IDs
    ********************************************************/
    `TEST_CASE_START("Check NoC ID");
    // Read NOC IDs
    tb_streamer.read_reg(sid_noc_block_specsense2k, RB_NOC_ID, readback);
    $display("Read specsense2k NOC ID: %16x", readback);
    `ASSERT_ERROR(readback == noc_block_specsense2k.NOC_ID, "Incorrect NOC ID");
    `TEST_CASE_DONE(1);

    /********************************************************
    ** Test 3 -- Connect RFNoC blocks
    ********************************************************/
    `TEST_CASE_START("Connect RFNoC blocks");
    `RFNOC_CONNECT(noc_block_tb,noc_block_specsense2k,SC16,FFT_SIZE);
    `RFNOC_CONNECT(noc_block_specsense2k,noc_block_tb,SC16,FFT_SIZE);
    `TEST_CASE_DONE(1);
    
    /********************************************************
    ** Test 4 -- Setup FFT
    ********************************************************/
    `TEST_CASE_START("Setup FFT registers");
    repeat (200) @(posedge bus_clk);
    tb_streamer.write_reg(sid_noc_block_specsense2k, noc_block_specsense2k.SR_FFT_RESET, {1'b1 });  // Reset
    repeat (100) @(posedge bus_clk);
    tb_streamer.write_reg(sid_noc_block_specsense2k, noc_block_specsense2k.SR_FFT_RESET, {1'b0 });  // Reset
    repeat (1000) @(posedge bus_clk);
    // Setup FFT
    tb_streamer.write_reg(sid_noc_block_specsense2k, noc_block_specsense2k.SR_AXI_CONFIG_BASE, {7'd0, fft_ctrl_word});  // Configure FFT core
    tb_streamer.write_reg(sid_noc_block_specsense2k, noc_block_specsense2k.SR_AXI_CONFIG_BASE + 2, {8'd0, {AVG_SIZE_REG, FFT_SIZE_REG}});             // Configure Avg block
    tb_streamer.write_reg(sid_noc_block_specsense2k, noc_block_specsense2k.SR_AVG_SIZE, {AVG_SIZE_REG});  // Avg size register

    `TEST_CASE_DONE(1);

    /*******************************************************
    *** Test 5 - Test random data
    *********************************************************/
    `TEST_CASE_START("Send random data");
     begin
       sample = get_random_complex_samples(FFT_SIZE);
       payload = get_payload(sample, FFT_SIZE); // 64 bit word i.e., one payload word = 2 samples
       for (int i = 0; i<AVG_SIZE; i++) begin
         tb_streamer.send(payload);
       end
     end
     $display("Random test done");
     repeat (1000) @(posedge bus_clk);
    `TEST_CASE_DONE(1);

    /********************************************************
    ** Test 5 -- Test sine wave
    ********************************************************/
    `TEST_CASE_START("Send sine wave & check FFT data");
    // Send 1/8th sample rate sine wave
    fork
    begin
      for (int n = 0; n < NUM_ITERATIONS; n++) begin
        for (int i = 0; i < (FFT_SIZE/8); i++) begin
          tb_streamer.push_word({ 16'd32767,     16'd0},0);
          tb_streamer.push_word({ 16'd23170, 16'd23170},0);
          tb_streamer.push_word({     16'd0, 16'd32767},0);
          tb_streamer.push_word({-16'd23170, 16'd23170},0);
          tb_streamer.push_word({-16'd32767,     16'd0},0);
          tb_streamer.push_word({-16'd23170,-16'd23170},0);
          tb_streamer.push_word({     16'd0,-16'd32767},0);
          tb_streamer.push_word({ 16'd23170,-16'd23170},(i == (FFT_SIZE/8)-1)); // Assert tlast on final word
        end
      end
    end
    begin
      for (int n = 0; n < NUM_OUT_PKTS; n++) begin
        for (int k = 0; k < (FFT_SIZE / PKTS_PER_FFT); k++) begin
          tb_streamer.pull_word(avg_mag,last);
          if (k == FFT_BIN && (n%PKTS_PER_FFT == 0)) begin
            // Assert that for the special case of a 1/8th sample rate sine wave input,
            // the real part of the corresponding 1/8th sample rate FFT bin should always be greater than 0 and
            // the complex part equal to 0.
            `ASSERT_ERROR(avg_mag > 32'd0, "FFT avg mag is not greater than 0!");
          end else begin
            // Assert all other FFT bins should be 0 for both complex and real parts
            `ASSERT_ERROR(avg_mag == 32'd0, "FFT avg mag is not 0!");
          end
          // Check packet size via tlast assertion
          if (k == (FFT_SIZE / PKTS_PER_FFT)-1) begin
            `ASSERT_ERROR(last == 1'b1, "Detected late tlast!");
          end else begin
            `ASSERT_ERROR(last == 1'b0, "Detected early tlast!");
          end
        end
      end
    end
    join
    $display("First test done ");
    `TEST_CASE_DONE(1);

    /*******************************************************************************
    ** Test 6 -- Reconfigure FFT and Avg blocks, send sine wave and check FFT data
    ********************************************************************************/
    `TEST_CASE_START("Send sine wave & check FFT data");
    // Setup FFT
    FFT_SIZE = 64;
    AVG_SIZE = 16;
    FFT_BIN = FFT_SIZE/8;
    NUM_ITERATIONS = 33;
    PKTS_PER_FFT = $ceil(FFT_SIZE / MAX_PKT_SIZE);
    NUM_OUT_PKTS = ((NUM_ITERATIONS+1)/(AVG_SIZE+1)) * PKTS_PER_FFT;

    repeat (2000) @(posedge bus_clk);
    tb_streamer.write_reg(sid_noc_block_specsense2k, noc_block_specsense2k.SR_FFT_RESET, {1'b1 });  // Reset
    repeat (100) @(posedge bus_clk);
    tb_streamer.write_reg(sid_noc_block_specsense2k, noc_block_specsense2k.SR_FFT_RESET, {1'b0 });  // Reset
    repeat (1000) @(posedge bus_clk);
    tb_streamer.write_reg(sid_noc_block_specsense2k, noc_block_specsense2k.SR_AXI_CONFIG_BASE, {7'd0, fft_ctrl_word});  // Configure FFT core
    tb_streamer.write_reg(sid_noc_block_specsense2k, noc_block_specsense2k.SR_AXI_CONFIG_BASE + 2, {8'd0, {AVG_SIZE_REG, FFT_SIZE_REG}});             // Configure Avg block
    tb_streamer.write_reg(sid_noc_block_specsense2k, noc_block_specsense2k.SR_AVG_SIZE, {AVG_SIZE_REG});  // Avg size register

    // Send 1/8th sample rate sine wave
    fork
    begin
      for (int n = 0; n < NUM_ITERATIONS; n++) begin
        for (int i = 0; i < (FFT_SIZE/8); i++) begin
          tb_streamer.push_word({ 16'd32767,     16'd0},0);
          tb_streamer.push_word({ 16'd23170, 16'd23170},0);
          tb_streamer.push_word({     16'd0, 16'd32767},0);
          tb_streamer.push_word({-16'd23170, 16'd23170},0);
          tb_streamer.push_word({-16'd32767,     16'd0},0);
          tb_streamer.push_word({-16'd23170,-16'd23170},0);
          tb_streamer.push_word({     16'd0,-16'd32767},0);
          tb_streamer.push_word({ 16'd23170,-16'd23170},(i == (FFT_SIZE/8)-1)); // Assert tlast on final word
        end
      end
    end
    begin
      for (int n = 0; n < NUM_OUT_PKTS; n++) begin
        for (int k = 0; k < FFT_SIZE/PKTS_PER_FFT; k++) begin
          tb_streamer.pull_word(avg_mag, last);
          if (k == FFT_BIN && (n%PKTS_PER_FFT==0)) begin
            // Assert that for the special case of a 1/8th sample rate sine wave input,
            // the real part of the corresponding 1/8th sample rate FFT bin should always be greater than 0 and
            // the complex part equal to 0.
            `ASSERT_ERROR(avg_mag > 32'd0, "FFT avg mag is not greater than 0!");
          end else begin
            // Assert all other FFT bins should be 0 for both complex and real parts
            `ASSERT_ERROR(avg_mag  == 32'd0, "FFT avg mag is not 0!");
          end
          // Check packet size via tlast assertion
          if (k == (FFT_SIZE / PKTS_PER_FFT)-1) begin
            `ASSERT_ERROR(last == 1'b1, "Detected late tlast!");
          end else begin
            `ASSERT_ERROR(last == 1'b0, "Detected early tlast!");
          end
        end
      end
    end
    join
    `TEST_CASE_DONE(1);

    /*******************************************************************************
    ** Test 7 -- Reconfigure FFT and Avg blocks, send sine wave and check FFT data
    ********************************************************************************/
    `TEST_CASE_START("Send sine wave & check FFT data");
    // Setup FFT
    FFT_SIZE = 128;
    AVG_SIZE = 8;
    FFT_BIN = FFT_SIZE/8;
    PKTS_PER_FFT = $ceil(FFT_SIZE / MAX_PKT_SIZE);
    NUM_OUT_PKTS = ((NUM_ITERATIONS+1)/(AVG_SIZE+1)) * PKTS_PER_FFT;

    repeat (2000) @(posedge bus_clk);
    tb_streamer.write_reg(sid_noc_block_specsense2k, noc_block_specsense2k.SR_FFT_RESET, {1'b1 });  // Reset
    repeat (100) @(posedge bus_clk);
    tb_streamer.write_reg(sid_noc_block_specsense2k, noc_block_specsense2k.SR_FFT_RESET, {1'b0 });  // Reset
    repeat (1000) @(posedge bus_clk);
    tb_streamer.write_reg(sid_noc_block_specsense2k, noc_block_specsense2k.SR_AXI_CONFIG_BASE, {7'd0, fft_ctrl_word});  // Configure FFT core
    tb_streamer.write_reg(sid_noc_block_specsense2k, noc_block_specsense2k.SR_AXI_CONFIG_BASE + 2, {8'd0, {AVG_SIZE_REG, FFT_SIZE_REG}});             // Configure Avg block
    tb_streamer.write_reg(sid_noc_block_specsense2k, noc_block_specsense2k.SR_AVG_SIZE, {AVG_SIZE_REG});  // Avg size register

    // Send 1/8th sample rate sine wave
    fork
    begin
      for (int n = 0; n < NUM_ITERATIONS; n++) begin
        for (int i = 0; i < (FFT_SIZE/8); i++) begin
          tb_streamer.push_word({ 16'd32767,     16'd0},0);
          tb_streamer.push_word({ 16'd23170, 16'd23170},0);
          tb_streamer.push_word({     16'd0, 16'd32767},0);
          tb_streamer.push_word({-16'd23170, 16'd23170},0);
          tb_streamer.push_word({-16'd32767,     16'd0},0);
          tb_streamer.push_word({-16'd23170,-16'd23170},0);
          tb_streamer.push_word({     16'd0,-16'd32767},0);
          tb_streamer.push_word({ 16'd23170,-16'd23170},(i == (FFT_SIZE/8)-1)); // Assert tlast on final word
        end
      end
    end
    begin
      for (int n = 0; n < NUM_OUT_PKTS; n++) begin
        for (int k = 0; k < FFT_SIZE/PKTS_PER_FFT; k++) begin
          tb_streamer.pull_word(avg_mag, last);
          if (k == FFT_BIN && (n%PKTS_PER_FFT == 0)) begin
            // Assert that for the special case of a 1/8th sample rate sine wave input,
            // the real part of the corresponding 1/8th sample rate FFT bin should always be greater than 0 and
            // the complex part equal to 0.
            `ASSERT_ERROR(avg_mag > 32'd0, "FFT avg mag is not greater than 0!");
          end else begin
            // Assert all other FFT bins should be 0 for both complex and real parts
            `ASSERT_ERROR(avg_mag  == 32'd0, "FFT avg mag is not 0!");
          end
          // Check packet size via tlast assertion
          if (k == (FFT_SIZE / PKTS_PER_FFT)-1) begin
            `ASSERT_ERROR(last == 1'b1, "Detected late tlast!");
          end else begin
            `ASSERT_ERROR(last == 1'b0, "Detected early tlast!");
          end
        end
      end
    end
    join

    `TEST_CASE_DONE(1);

    `TEST_BENCH_DONE;

  end
endmodule
