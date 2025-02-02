// ---------------------------------------------------
// File       : pe_array3x3.v
//
// Description: 2 dim(3x3) processing element array
//
// Version    : 1.0
// ---------------------------------------------------
module pe_array3x3#(
          parameter EXPONENT = 8,
          parameter MANTISSA = 23
      )(
        input  wire                                 clk,
        input  wire                                 pe3_array0_valid, // data valid, enable pe_array
        input  wire                                 pe3_array1_valid,
        input  wire                                 pe3_array2_valid,
        input  wire [3*(1+MANTISSA+EXPONENT)-1:0]   pe3_array0_data3, // 3 data
        input  wire [3*(1+MANTISSA+EXPONENT)-1:0]   pe3_array1_data3,
        input  wire [3*(1+MANTISSA+EXPONENT)-1:0]   pe3_array2_data3,
        input  wire [3*(1+MANTISSA+EXPONENT)-1:0]   pe3_array0_ker3,  // 3 weight
        input  wire [3*(1+MANTISSA+EXPONENT)-1:0]   pe3_array1_ker3,
        input  wire [3*(1+MANTISSA+EXPONENT)-1:0]   pe3_array2_ker3,

        output wire [MANTISSA+EXPONENT:0]           pe3_o
      );

  wire [MANTISSA+EXPONENT:0] _pe_array0_o;
  wire [MANTISSA+EXPONENT:0] _pe_array1_o;
  wire [MANTISSA+EXPONENT:0] _pe_array2_o;

  assign pe3_o = _pe_array2_o;

  pe_array1x3#(
    .EXPONENT(EXPONENT),
    .MANTISSA(MANTISSA)
    ) pe3_0(
      .clk(clk),
      .pe_ker3_i(pe3_array0_ker3),
      .pe_bias_i({(EXPONENT+MANTISSA+1){1'b0}}),
      .pe_data3_i(pe3_array0_data3),
      .pe_on_3mul_pe_en(pe3_array0_valid),
      .pe_on_align3(1'b1),
      .pe_on_sum3(1'b1),
      .pe_on_bias(1'b1),
      .pe_data_o(_pe_array0_o)
    );

  pe_array1x3#(
    .EXPONENT(EXPONENT),
    .MANTISSA(MANTISSA)
    ) pe3_1(
      .clk(clk),
      .pe_ker3_i(pe3_array1_ker3),
      .pe_bias_i(_pe_array0_o),
      .pe_data3_i(pe3_array1_data3),
      .pe_on_3mul_pe_en(pe3_array1_valid),
      .pe_on_align3(1'b1),
      .pe_on_sum3(1'b1),
      .pe_on_bias(1'b1),
      .pe_data_o(_pe_array1_o)
    );

  pe_array1x3#(
    .EXPONENT(EXPONENT),
    .MANTISSA(MANTISSA)
    ) pe3_2(
      .clk(clk),
      .pe_ker3_i(pe3_array2_ker3),
      .pe_bias_i(_pe_array1_o),
      .pe_data3_i(pe3_array2_data3),
      .pe_on_3mul_pe_en(pe3_array2_valid),
      .pe_on_align3(1'b1),
      .pe_on_sum3(1'b1),
      .pe_on_bias(1'b1),
      .pe_data_o(_pe_array2_o)
    );

endmodule
