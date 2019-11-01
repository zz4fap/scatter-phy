`timescale 1ns/1ps
`define NS_PER_TICK 1
`define NUM_TEST_CASES 5

`include "sim_exec_report.vh"
`include "sim_clks_rsts.vh"
`include "sim_rfnoc_lib.svh"

module noc_block_fir129_tb();
  `TEST_BENCH_INIT("noc_block_fir129",`NUM_TEST_CASES,`NS_PER_TICK);
  localparam BUS_CLK_PERIOD = $ceil(1e9/166.67e6);
  localparam CE_CLK_PERIOD  = $ceil(1e9/200e6);
  localparam NUM_CE         = 1;  // Number of Computation Engines / User RFNoC blocks to simulate
  localparam NUM_STREAMS    = 1;  // Number of test bench streams
  `RFNOC_SIM_INIT(NUM_CE, NUM_STREAMS, BUS_CLK_PERIOD, CE_CLK_PERIOD);
  `RFNOC_ADD_BLOCK(noc_block_fir129, 0);

  localparam SPP = 64; // Samples per packet
  localparam NUM_COEFFS = 65;
  localparam COEFF_WIDTH = 16;
  localparam [COEFF_WIDTH*NUM_COEFFS-1:0] COEFFS_VEC_0 = {16'sd0, 16'sd0, 16'sd0, 16'sd0, 16'sd0, 16'sd0, -16'sd1, 16'sd0, 16'sd3, -16'sd2, -16'sd4, 16'sd7, 16'sd2, -16'sd12, 16'sd4, 16'sd15, -16'sd15, -16'sd13, 16'sd28, 16'sd1, -16'sd38, 16'sd18, 16'sd38, -16'sd45, -16'sd24, 16'sd70, -16'sd7, -16'sd82, 16'sd54, 16'sd72, -16'sd104, -16'sd31, 16'sd141, -16'sd37, -16'sd149, 16'sd123, 16'sd111, -16'sd205, -16'sd23, 16'sd254, -16'sd106, -16'sd243, 16'sd253, 16'sd152, -16'sd378, 16'sd20, 16'sd435, -16'sd254, -16'sd381, 16'sd504, 16'sd187, -16'sd707, 16'sd152, 16'sd784, -16'sd615, -16'sd650, 16'sd1149, 16'sd210, -16'sd1685, 16'sd690, 16'sd2143, -16'sd2559, -16'sd2451, 16'sd10110, 16'sd18944}; // TX FIR coefficients

  int pkt_size;

  /********************************************************
  ** Verification
  ********************************************************/
  initial begin : tb_main
    string s;
    logic [31:0] random_word;
    logic [63:0] readback;
    int num_taps;

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
    tb_streamer.read_reg(sid_noc_block_fir129, RB_NOC_ID, readback);
    $display("Read FIR Filter NOC ID: %16x", readback);
    `ASSERT_ERROR(readback == noc_block_fir129.NOC_ID, "Incorrect NOC ID");
    `TEST_CASE_DONE(1);

    /********************************************************
    ** Test 3 -- Connect RFNoC blocks
    ********************************************************/
    `TEST_CASE_START("Connect RFNoC blocks");
    `RFNOC_CONNECT(noc_block_tb,noc_block_fir129,SC16,SPP);
    `RFNOC_CONNECT(noc_block_fir129,noc_block_tb,SC16,SPP);
    `TEST_CASE_DONE(1);

    /********************************************************
    ** Test 4 -- Impulse Response
    ********************************************************/
    // Sending an impulse will readback the FIR filter coefficients
    `TEST_CASE_START("Test impulse response");

    /* Set filter coefficients via reload bus */
    // Read NUM_TAPS
    tb_streamer.read_user_reg(sid_noc_block_fir129, noc_block_fir129.RB_NUM_TAPS, num_taps);
    $display("Number of taps : %d", num_taps);
    // Write a ramp to FIR filter coefficients
    // Since the filter was customized to be symmetric, load only 65 coefficients
    // Loading coefficients 0,1,2,3,4,5...64
    for (int i = 0; i < (num_taps-1)/2; i++) begin // 0 to 63 - 64 taps
      tb_streamer.write_user_reg(sid_noc_block_fir129, noc_block_fir129.SR_RELOAD, i); 
    end
    tb_streamer.write_user_reg(sid_noc_block_fir129, noc_block_fir129.SR_RELOAD_LAST, (num_taps-1)/2); // 65th tap
    // Load coefficients
    tb_streamer.write_user_reg(sid_noc_block_fir129, noc_block_fir129.SR_CONFIG, 0);

    /* Send and check impulse */
    fork
      begin
        $display("Send impulse");
        tb_streamer.push_word({16'h7FFF,16'h7FFF}, 0); // 32767
        for (int i = 0; i < num_taps-1; i++) begin
          tb_streamer.push_word(0, (i == num_taps-2) ); // Assert tlast on last word 
        end
      end
      begin
        logic [31:0] recv_val;
        logic last;
        logic [15:0] i_samp, q_samp;
        int expected_val;
        $display("Receive FIR filter output");
        for (int i = 0; i < num_taps; i++) begin
          tb_streamer.pull_word({i_samp, q_samp}, last);
          // Check I / Q values 
          // noc_block_fir129 rounds the filter output by discarding 15 LSBs i.e., divides by 32678. So expected values: 0,1,2,3,...64,63,62,...2,1,0
          if(i <= (num_taps-1)/2) begin
            expected_val = i;
          end else begin
            expected_val = num_taps-i-1;
          end

          $sformat(s, "Incorrect I value received! Expected: %0d, Received: %0d", expected_val, i_samp);
          `ASSERT_ERROR(i_samp == expected_val, s);
          $sformat(s, "Incorrect Q value received! Expected: %0d, Received: %0d", expected_val, q_samp);
          `ASSERT_ERROR(q_samp == expected_val, s);
          // Check tlast
          if (i == num_taps-1) begin
            `ASSERT_ERROR(last, "Last not asserted on final word!");
          end else begin
            `ASSERT_ERROR(~last, "Last asserted early!");
          end
        end
      end
    join
    `TEST_CASE_DONE(1);
    /*********************************************************************
    ***  Test 5 - Reload coefficients and test 
    *********************************************************************/
    `TEST_CASE_START("Reload coefficients and test");

    pkt_size = 2000;

    for (int i = 0; i < (num_taps-1)/2; i++) begin // 0 to 63 - 64 taps
      tb_streamer.write_user_reg(sid_noc_block_fir129, noc_block_fir129.SR_RELOAD, COEFFS_VEC_0[COEFF_WIDTH*(NUM_COEFFS-i)-1 -: COEFF_WIDTH]);
    end
    tb_streamer.write_user_reg(sid_noc_block_fir129, noc_block_fir129.SR_RELOAD_LAST, COEFFS_VEC_0[COEFF_WIDTH-1 : 0]); // 65th tap
    // Load coefficients
    tb_streamer.write_user_reg(sid_noc_block_fir129, noc_block_fir129.SR_CONFIG, 0);

    // Send impulse
    fork
      begin
        $display("Send impulse");
        tb_streamer.push_word({16'h7FFF,16'h7FFF}, 0); // 32767
        for (int i = 0; i < pkt_size-1; i++) begin
          tb_streamer.push_word(0, (i == pkt_size-2) /* Assert tlast on last word */);
        end
      end
      begin
        logic last;
        logic signed [15:0] i_samp, q_samp;
        int expected_val;
        $display("Receive FIR filter output");
        for (int i = 0; i < pkt_size; i++) begin
          tb_streamer.pull_word({i_samp, q_samp}, last);
          if(i<num_taps)
            $display("i : %d, samp : %d", i, i_samp);
          // Check tlast
          if (i == pkt_size-1) begin
            `ASSERT_ERROR(last, "Last not asserted on final word!");
          end else begin
            `ASSERT_ERROR(~last, "Last asserted early!");
          end
        end
      end
    join



    `TEST_CASE_DONE(1);

    `TEST_BENCH_DONE;

  end
endmodule
