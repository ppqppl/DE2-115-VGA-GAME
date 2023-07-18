//-------------------------------------------------------------------------
//    Ball.sv                                                            --
//    Viral Mehta                                                        --
//    Spring 2005                                                        --
//                                                                       --
//    Modified by Stephen Kempf 03-01-2006                               --
//                              03-12-2007                               --
//    Translated by Joe Meng    07-07-2013                               --
//    Modified by Po-Han Huang  12-08-2017                               --
//    Spring 2018 Distribution                                           --
//                                                                       --
//    For use with ECE 385 Lab 8                                         --
//    UIUC ECE Department                                                --
//-------------------------------------------------------------------------

//子弹 (bullet) 实体，顶层文件使用 generate 循环来生成。
//子弹实体用于显示游戏中的子弹
//子弹实体包括一个圆形的位置、半径和颜色等信息，
//以及一个 VGA 控制器的输出信号 (VGA_isObject 和 VGA_Pixel)，用于在 VGA 显示器上显示子弹。
module bullet (input logic [10:0]  BulletX, BulletY, BulletRadius,//子弹坐标及半径
			   input logic [15:0]  BulletColor,//子弹颜色
               input logic [9:0]   DrawX, DrawY,//绘制的坐标
			   output logic VGA_isObject,//用于指示子弹是否为图像对象
               output logic [15:0] VGA_Pixel//图像像素
              );
			  
	assign VGA_Pixel = BulletColor;
    
    int DistX, DistY, Size;
    assign DistX = DrawX - BulletX;
    assign DistY = DrawY - BulletY;
	assign Size = BulletRadius;
	
	always_comb begin//判断子弹与绘图坐标之间的距离是否小于等于子弹半径
		if((DistX*DistX + DistY*DistY) <= (Size*Size)) begin
			VGA_isObject = 1'b1;
		end else begin
			VGA_isObject = 1'b0;
		end
	end

endmodule
