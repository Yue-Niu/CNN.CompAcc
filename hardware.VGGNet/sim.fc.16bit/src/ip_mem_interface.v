// inner product memmory interface
// consists: 
// parameter buffer and rd_wr control
// data block Ram
`include "common.v"
module ip_mem_interface
#(
    parameter FW = 32,
    parameter DW = 512,
    parameter CDW = 1024,
    parameter WL = 32
)
(
    input rstn_i,
    input clk_i,

    // param buf
    input param_buf_en_i,
    input         wr_buf_sel_i,
    input [2-1:0] param_buf_full_i,
    input [DW-1:0] param_i,
    input [2-1:0] param_buf_busy_i,
    input [9-1:0] param_buf_addr_i,
    output wr_buf_done_o,
    output [FW-1:0] ip_param_o,

    // conv_buf
    input conv_buf_ena_i,
    input conv_buf_wea_i,
    input [10-1:0] conv_buf_addra_i,
    input [CDW-1:0] conv_buf_dina_i,
    input conv_buf_enb_i,
    input [15-1:0] conv_buf_addrb_i,
    output [FW-1:0] conv_buf_doub_o,

    // ip_buf_0
    input ip_buf0_ena_i,
    input ip_buf0_wea_i,
    input [12-1:0] ip_buf0_addra_i,
    input [FW-1:0] ip_buf0_dina_i,
    output [FW-1:0] ip_buf0_douta_o,

    // ip_buf_1
    input ip_buf1_ena_i,
    input ip_buf1_wea_i,
    input [10-1:0] ip_buf1_addra_i,
    input [FW-1:0] ip_buf1_dina_i,
    output [FW-1:0]ip_buf1_douta_o

);


    //====================================
    //  ip parameter buf
    //==================================== 
    ip_param_buf
    #(
        .FW(FW),
        .DW(DW),
        .WL(WL)
    )
    ip_param_buf_U
    (
        .clk_i (clk_i ),
        .rstn_i(rstn_i),
        .en_i            (param_buf_en_i  ),
        .wr_buf_sel_i    (wr_buf_sel_i    ),
        .param_buf_full_i(param_buf_full_i),
        .param_i         (param_i         ),
        .param_buf_busy_i(param_buf_busy_i),
        .ip_buf_addr_i   (param_buf_addr_i),
        .wr_buf_done_o   (wr_buf_done_o   ),
        .ip_param_o      (ip_param_o      )
    );

    //====================================
    //  conv buf
    //==================================== 
    conv_buf
    conv_buf_U
    (
        .clka (clk_i),
        .ena  (conv_buf_ena_i  ),
        .wea  (conv_buf_wea_i  ),
        .addra(conv_buf_addra_i),
        .dina (conv_buf_dina_i ),
        .clkb (clk_i),
        .enb  (conv_buf_enb_i  ),
        .addrb(conv_buf_addrb_i),
        .doutb(conv_buf_doub_o )
    );


    //====================================
    //  ip pingpang buf
    //==================================== 
    ip_buf_0
    ip_buf_0_U
    (
        .clka (clk_i),
        .ena  (ip_buf0_ena_i  ),
        .wea  (ip_buf0_wea_i  ),
        .addra(ip_buf0_addra_i),
        .dina (ip_buf0_dina_i ),
        .douta(ip_buf0_douta_o)
    );

    ip_buf_1
    ip_buf_1_U
    (
        .clka(clk_i),
        .ena  (ip_buf1_ena_i  ),
        .wea  (ip_buf1_wea_i  ),
        .addra(ip_buf1_addra_i),
        .dina (ip_buf1_dina_i ),
        .douta(ip_buf1_douta_o)
    );
endmodule
