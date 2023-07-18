//vga实体管理器，用于管理游戏对象和子弹等实体的信息。
//该管理器包括一个 Avalon-MM 接口 (AVL_READ、AVL_WRITE、AVL_ADDR、AVL_WRITEDATA 和 AVL_READDATA)，
//用于读取和写入游戏对象和子弹信息的内存地址和数据。该管理器还包括一个导出数据接口 (EXPORT_DATA)，用于导出所有游戏对象和子弹信息的数据。
//该模块的功能是实现多个实体的寄存器，并将寄存器数据通过 EXPORT_DATA 端口输出。
module VGA_entities #(
	parameter REGISTERS = 256
) (
	// Avalon Clock Input
	input logic CLK,
	
	// Avalon Reset Input
	input logic RESET,
	
	// Avalon-MM Slave Signals
	input  logic AVL_READ,					// Avalon-MM 读
	input  logic AVL_WRITE,					// Avalon-MM 写
	input  logic [7:0] AVL_ADDR,			// Avalon-MM 地址信号
	input  logic [31:0] AVL_WRITEDATA,	// Avalon-MM 写数据
	output logic [31:0] AVL_READDATA,	// Avalon-MM 读数据信号
	
	// Exported Conduit
	output logic [REGISTERS-1:0][31:0] EXPORT_DATA		// 输出信号，包含多个实体的寄存器数据
);

genvar i;
generate//生成256个寄存器模块，每个模块对应一个实体的寄存器。
	for(i = 0; i < REGISTERS; i++) begin: generate_vga_entity_registers
		register #(32) entity_register (
			.Clk(CLK), .Reset(RESET),
			.Load((AVL_ADDR == i) && AVL_WRITE),
			.Din(AVL_WRITEDATA),
			.Dout(EXPORT_DATA[i])//对应的寄存器的值存到EXPORT_DATA中
		);
	end
endgenerate

assign AVL_READDATA = EXPORT_DATA[AVL_ADDR];

endmodule
