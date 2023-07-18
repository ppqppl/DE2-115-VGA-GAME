//VGA 精灵(飞机)，顶层文件使用 generate 循环来生成。
//用于显示游戏对象的图像和动画
//每个 VGA 精灵实体包括一个 Avalon-MM 接口 (AVL_Addr 和 AVL_ReadData)，用于读取它们的图像和动画数据的内存地址和数据。
//每个 VGA 精灵实体还包括一个 VGA 控制器的输出信号 (VGA_isObject 和 VGA_Pixel)，用于在 VGA 显示器上显示它们的图像和动画。
module VGA_sprite (
	input logic Clk, Reset,

	input logic [15:0] SpriteX, SpriteY,//飞机坐标
	input logic [15:0] SpriteWidth, SpriteHeight,//宽高
	
	input logic [9:0] VGA_DrawX, VGA_DrawY,//要绘制的像素点在画布上的 X、Y 坐标
	output logic [11:0] AVL_Addr,//显存中像素地址
	input logic [15:0] AVL_ReadData,//显存中读取的像素值
	
	output logic VGA_isObject,//是否在有效区域
	output logic [15:0] VGA_Pixel//显示的像素值
);

logic [11:0] Pixel_Addr;
logic VGA_isInObject, VGA_isInObject_prev;

logic [15:0] SpriteRight, SpriteBottom;
assign SpriteRight = SpriteX + SpriteWidth;
assign SpriteBottom = SpriteY + SpriteHeight;

always_comb begin
	VGA_isInObject = 1'b0;
	AVL_Addr = 11'b0;
	
	if((VGA_DrawX >= SpriteX || SpriteX[15])//判断VGA_DrawX、VGA_DrawY是否在精灵区域内来绘制像素点
		&& (VGA_DrawX < SpriteRight && !SpriteRight[15])
		&& (VGA_DrawY >= SpriteY || SpriteY[15])
		&& (VGA_DrawY < SpriteBottom && !SpriteBottom[15])
	) begin
		VGA_isInObject = 1'b1;
		AVL_Addr = SpriteWidth * (VGA_DrawY - SpriteY) + (VGA_DrawX - SpriteX);//计算像素点在显存中的地址
	end
end

always_ff @ (posedge Clk) begin//重置时VGA_isInObject_prev，其余情况保持不变，标志是否在有效区域
	if(Reset) begin
		VGA_isInObject_prev <= 1'b0;
	end else begin
		VGA_isInObject_prev <= VGA_isInObject;
	end
end

assign VGA_Pixel = AVL_ReadData[15:0];//将显存中读取到的像素值输出
//assign VGA_Pixel = 16'h0000;
assign VGA_isObject = (AVL_ReadData[15:0] != 16'h0000) && VGA_isInObject_prev;//显存中读取到的像素值不为0且在有效区域时VGA_isObject为ture输出
//assign VGA_isObject = VGA_isInObject_prev;

endmodule
