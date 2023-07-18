/*

Copyright (c) 2015-2018 Alex Forencich

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

// Language: Verilog 2001

`timescale 1ns / 1ps

/*
 * 1G Ethernet MAC
 */
module eth_mac_1g #
(
    parameter ENABLE_PADDING = 1,
    parameter MIN_FRAME_LENGTH = 64
)
(
    input  wire        rx_clk,
    input  wire        rx_rst,
    input  wire        tx_clk,
    input  wire        tx_rst,

    /*
    AXI协议是被优化用于通过使用Xilinx进行的相应的开发来做FPGA实现，它被用作FPGA 设计的IP 核之间的一种通信方式。
    关键特性
　　1、地址/控制阶段和数据阶段是分开的，即master(主机)和slave(从机)之间有专门的地址/控制通道，还有专门的数据通道。
　　2、有字节闸来实现对非对齐数据的传输。
　　3、只需发布起始地址就能做批量数据传输
　　4、数据的读写通道是分离的，可以用来实现低成本的DMA(直接存储访问，Direct Memory Access)。除了地址和数据通道是分离的之外，读写数据的通道还是分开的，由此可以看出AXI总线的高速性。
　　5、可以指定多个需要处理的地址。
　　6、通信会话可以乱序完成，主要是指的数据的乱序，乱序发送需要有主机的ID进行支撑。
　　7、为了实现时序收敛，可以方便的加入寄存器，即在用户logic和user interface处加入想要观察和处理的用户逻辑与端口。
     * AXI input
     */
    input  wire [7:0]  tx_axis_tdata,
    input  wire        tx_axis_tvalid,
    output wire        tx_axis_tready,
    input  wire        tx_axis_tlast,
    input  wire        tx_axis_tuser,

    /*
     * AXI output
     */
    output wire [7:0]  rx_axis_tdata,
    output wire        rx_axis_tvalid,
    output wire        rx_axis_tlast,
    output wire        rx_axis_tuser,

    /*
     * GMII interface  GMII接口
     */
    input  wire [7:0]  gmii_rxd,
    input  wire        gmii_rx_dv,
    input  wire        gmii_rx_er,
    output wire [7:0]  gmii_txd,
    output wire        gmii_tx_en,
    output wire        gmii_tx_er,

    /*
     * Control
     */
    input  wire        rx_clk_enable,
    input  wire        tx_clk_enable,
    input  wire        rx_mii_select,
    input  wire        tx_mii_select,

    /*
     * Status
     */
    output wire        tx_start_packet,
    output wire        tx_error_underflow,
    output wire        rx_start_packet,
    output wire        rx_error_bad_frame,
    output wire        rx_error_bad_fcs,

    /*
     * Configuration
     */
    input  wire [7:0]  ifg_delay//以太网中相邻两帧之间的时间间隔保证有效传输数据、冲突避免、调整传输速率、兼容性、接收设备正确处理帧
);

axis_gmii_rx
axis_gmii_rx_inst (
    .clk(rx_clk),
    .rst(rx_rst),
    .gmii_rxd(gmii_rxd),
    .gmii_rx_dv(gmii_rx_dv),
    .gmii_rx_er(gmii_rx_er),
    .m_axis_tdata(rx_axis_tdata),
    .m_axis_tvalid(rx_axis_tvalid),
    .m_axis_tlast(rx_axis_tlast),
    .m_axis_tuser(rx_axis_tuser),
    .clk_enable(rx_clk_enable),
    .mii_select(rx_mii_select),
    .start_packet(rx_start_packet),
    .error_bad_frame(rx_error_bad_frame),
    .error_bad_fcs(rx_error_bad_fcs)
);

axis_gmii_tx #(
    .ENABLE_PADDING(ENABLE_PADDING),
    .MIN_FRAME_LENGTH(MIN_FRAME_LENGTH)
)
axis_gmii_tx_inst (
    .clk(tx_clk),
    .rst(tx_rst),
    .s_axis_tdata(tx_axis_tdata),
    .s_axis_tvalid(tx_axis_tvalid),
    .s_axis_tready(tx_axis_tready),
    .s_axis_tlast(tx_axis_tlast),
    .s_axis_tuser(tx_axis_tuser),
    .gmii_txd(gmii_txd),
    .gmii_tx_en(gmii_tx_en),
    .gmii_tx_er(gmii_tx_er),
    .clk_enable(tx_clk_enable),
    .mii_select(tx_mii_select),
    .ifg_delay(ifg_delay),
    .start_packet(tx_start_packet),
    .error_underflow(tx_error_underflow)
);

endmodule
