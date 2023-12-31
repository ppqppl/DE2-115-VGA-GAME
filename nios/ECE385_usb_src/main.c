/*---------------------------------------------------------------------------
  --      main.c                                                    	   --
  --      Christine Chen                                                   --
  --      Ref. DE2-115 Demonstrations by Terasic Technologies Inc.         --
  --      Fall 2014                                                        --
  --                                                                       --
  --      For use with ECE 298 Experiment 7                                --
  --      UIUC ECE Department                                              --
  ---------------------------------------------------------------------------*/

//#include <stdio.h>
//#include <string.h>
//#include <stdlib.h>
#include <sys/alt_stdio.h>
#include <io.h>
#include <fcntl.h>

#include "system.h"
#include "alt_types.h"
#include <unistd.h>  // usleep 
#include "sys/alt_irq.h"
#include "io_handler.h"

#include "cy7c67200.h"
#include "usb.h"
#include "lcp_cmd.h"
#include "lcp_data.h"
#include "comm.h"

volatile keycode_comm_t* keycode_comm = (keycode_comm_t*) USB_KEYCODE_BASE;

//----------------------------------------------------------------------------------------//
//
//                                Main function
//
//----------------------------------------------------------------------------------------//
int main(void)
{
	//IO初始化
	IO_init();
	

	alt_u16 intStat;
	alt_u16 usb_ctl_val;
	static alt_u16 ctl_reg = 0;
	static alt_u16 no_device = 0;
	alt_u16 fs_device = 0;
	int keycode = 0;
	alt_u8 toggle = 0;
	alt_u8 data_size;
	alt_u8 hot_plug_count;
	alt_u16 code;
	int wait_cycle = 0;
	
	alt_printf("USB keyboard setup...\n\n");

	//----------------------------------------SIE1 initial---------------------------------------------------//
//USB热插拔标志位
USB_HOT_PLUG:
	//键盘标志位设置为0
	keycode_comm->keyboard_present = 0;
	//执行USB软重置
	UsbSoftReset();

	// STEP 1a:
	//设置消息地址为0
	UsbWrite (HPI_SIE1_MSG_ADR, 0);
	//写入寄存器的值
	UsbWrite (HOST1_STAT_REG, 0xFFFF);

	/* Set HUSB_pEOT time */
	UsbWrite(HUSB_pEOT, 600); // 根据USB的规范，设置EOT时间为600

	//设置USB控制寄存器的值
	usb_ctl_val = SOFEOP1_TO_CPU_EN | RESUME1_TO_HPI_EN;// | SOFEOP1_TO_HPI_EN;
	UsbWrite(HPI_IRQ_ROUTING_REG, usb_ctl_val);

	//设置中断状态
	intStat = A_CHG_IRQ_EN | SOF_EOP_IRQ_EN ;
	UsbWrite(HOST1_IRQ_EN_REG, intStat);
	// STEP 1a end

	// STEP 1b begin
	//重置时间
	UsbWrite(COMM_R0,0x0000);//reset time
	//端口号设置
	UsbWrite(COMM_R1,0x0000);  //port number
	//后续寄存器设置为0
	UsbWrite(COMM_R2,0x0000);  //r1
	UsbWrite(COMM_R3,0x0000);  //r1
	UsbWrite(COMM_R4,0x0000);  //r1
	UsbWrite(COMM_R5,0x0000);  //r1
	UsbWrite(COMM_R6,0x0000);  //r1
	UsbWrite(COMM_R7,0x0000);  //r1
	UsbWrite(COMM_R8,0x0000);  //r1
	UsbWrite(COMM_R9,0x0000);  //r1
	UsbWrite(COMM_R10,0x0000);  //r1
	UsbWrite(COMM_R11,0x0000);  //r1
	UsbWrite(COMM_R12,0x0000);  //r1
	UsbWrite(COMM_R13,0x0000);  //r1
	//设置通信中断号为HUSB_SIE1_INIT_INT
	UsbWrite(COMM_INT_NUM,HUSB_SIE1_INIT_INT); //HUSB_SIE1_INIT_INT
	//通过HPI_MAILBOX发送执行中断
	IO_write(HPI_MAILBOX,COMM_EXEC_INT);

	wait_cycle = 0;
	//循环等待SIE1消息寄存器不为零
	while (!(IO_read(HPI_STATUS) & 0xFFFF) )  //read sie1 msg register
	{
		//如果等待时间超过256次，跳转到USB_HOT_PLUG重新开始
		if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
	}
	//循环等待直到收到COMM_ACK确认
	while (IO_read(HPI_MAILBOX) != COMM_ACK)
	{
		alt_printf("[ERROR]:routine mailbox data is %x\n",IO_read(HPI_MAILBOX));
		goto USB_HOT_PLUG;
	}
	// STEP 1b end

	alt_printf("STEP 1 Complete");
	// STEP 2 begin
	
	//执行USB复位，发送复位命令
	UsbWrite(COMM_INT_NUM,HUSB_RESET_INT); //husb reset
	//设置复位时间
	UsbWrite(COMM_R0,0x003c);//reset time
	//设置端口号
	UsbWrite(COMM_R1,0x0000);  //port number
	UsbWrite(COMM_R2,0x0000);  //r1
	UsbWrite(COMM_R3,0x0000);  //r1
	UsbWrite(COMM_R4,0x0000);  //r1
	UsbWrite(COMM_R5,0x0000);  //r1
	UsbWrite(COMM_R6,0x0000);  //r1
	UsbWrite(COMM_R7,0x0000);  //r1
	UsbWrite(COMM_R8,0x0000);  //r1
	UsbWrite(COMM_R9,0x0000);  //r1
	UsbWrite(COMM_R10,0x0000);  //r1
	UsbWrite(COMM_R11,0x0000);  //r1
	UsbWrite(COMM_R12,0x0000);  //r1
	UsbWrite(COMM_R13,0x0000);  //r1

	//通过HPI_MAILBOX发送执行中断
	IO_write(HPI_MAILBOX,COMM_EXEC_INT);

	//等待中断响应
	while (IO_read(HPI_MAILBOX) != COMM_ACK)
	{
		alt_printf("[ERROR]:routine mailbox data is %x\n",IO_read(HPI_MAILBOX));
		goto USB_HOT_PLUG;
		//如果未收到响应，则跳转到USB_HOT_PLUG重新开始
	}
	// STEP 2 end

	ctl_reg = USB1_CTL_REG;					//USB控制寄存器地址
	no_device = (A_DP_STAT | A_DM_STAT);	//判断设备是否存在的标志位
	fs_device = A_DP_STAT;					//判断设备速度的标志位
	usb_ctl_val = UsbRead(ctl_reg);			//读取USB控制寄存器的值

	
	if (!(usb_ctl_val & no_device))			//如果设备不存在
	{
		//最多重试五次
		for(hot_plug_count = 0 ; hot_plug_count < 5 ; hot_plug_count++)
		{
			//等待5ms
			usleep(5*1000);
			usb_ctl_val = UsbRead(ctl_reg);		//重新读取USB控制寄存器的值
			if(usb_ctl_val & no_device) break;	//如果设备存在，则跳出循环
		}
		if(!(usb_ctl_val & no_device))			//如果设备还是不存在
		{
			//打印提示信息，STEI1中没有设备
			alt_printf("\n[INFO]: no device is present in SIE1!\n");
			//打印提示信息，提示用户插入USB键盘
			alt_printf("[INFO]: please insert a USB keyboard in SIE1!\n");
			wait_cycle = 0;
			//循环等待，直到检测到设备存在
			while (!(usb_ctl_val & no_device))
			{
				//如果等待时间超过256次，跳转到USB_HOT_PLUG重新开始
				if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
				//重新读取USB控制寄存器的值
				usb_ctl_val = UsbRead(ctl_reg);
				//如果设备存在，则跳出循环
				if(usb_ctl_val & no_device)
					goto USB_HOT_PLUG;

				usleep(2000);
			}
		}
	}
	else
	{
		/* check for low speed or full speed by reading D+ and D- lines */
		//电平，判断是否全速设备
		if (usb_ctl_val & fs_device)
		{
			alt_printf("[INFO]: full speed device\n");
		}
		else
		{
			alt_printf("[INFO]: low speed device\n");
		}
	}



	// STEP 3 begin
	//------------------------------------------------------set address -----------------------------------------------------------------
	UsbSetAddress();
	wait_cycle = 0;
	while (!(IO_read(HPI_STATUS) & HPI_STATUS_SIE1msg_FLAG) )  //read sie1 msg register
	{
		if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
		UsbSetAddress();
		usleep(10*1000);
	}

	if(0xffff == UsbWaitTDListDone()) goto USB_HOT_PLUG;

	IO_write(HPI_ADDR,0x0506); 
	alt_printf("[ENUM PROCESS]:step 3 TD Status Byte is %x\n",IO_read(HPI_DATA));

	IO_write(HPI_ADDR,0x0508); 
	usb_ctl_val = IO_read(HPI_DATA);
	alt_printf("[ENUM PROCESS]:step 3 TD Control Byte is %x\n",usb_ctl_val);
	while (usb_ctl_val != 0x03) // retries occurred
	{
		usb_ctl_val = UsbGetRetryCnt();

		goto USB_HOT_PLUG;
	}

	alt_printf("------------[ENUM PROCESS]:set address done!---------------\n");

	// STEP 4 begin
	//-------------------------------get device descriptor-1 -----------------------------------//
	// TASK: Call the appropriate function for this step.
	UsbGetDeviceDesc1(); 	// Get Device Descriptor -1

	//usleep(10*1000);
	wait_cycle = 0;
	//持续等待，直到收到SIE1消息寄存器的值
	while (!(IO_read(HPI_STATUS) & HPI_STATUS_SIE1msg_FLAG) )  //read sie1 msg register
	{
		//如果等待时间超过256次，跳转到USB_HOT_PLUG重新开始	
		if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
		// TASK: Call the appropriate function again if it wasn't processed successfully.
		
		UsbGetDeviceDesc1();
		usleep(10*1000);
	}

	if(0xffff == UsbWaitTDListDone()) goto USB_HOT_PLUG;

	
	IO_write(HPI_ADDR,0x0506);
	//打印状态字节的值
	alt_printf("[ENUM PROCESS]:step 4 TD Status Byte is %x\n",IO_read(HPI_DATA));

	IO_write(HPI_ADDR,0x0508);
	usb_ctl_val = IO_read(HPI_DATA);
	
	alt_printf("[ENUM PROCESS]:step 4 TD Control Byte is %x\n",usb_ctl_val);
	wait_cycle = 0;
	//如果重试次数不为3
	while (usb_ctl_val != 0x03)
	{
		if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
		usb_ctl_val = UsbGetRetryCnt();
	}

	alt_printf("---------------[ENUM PROCESS]:get device descriptor-1 done!-----------------\n");


	//--------------------------------get device descriptor-2---------------------------------------------//
	//get device descriptor
	// TASK: Call the appropriate function for this step.
	//获取设备描述符
	UsbGetDeviceDesc2(); 	// Get Device Descriptor -2

	//if no message
	wait_cycle = 0;//等待周期计数器归零
	//持续等待，直到收到SIE1消息寄存器的值
	while (!(IO_read(HPI_STATUS) & HPI_STATUS_SIE1msg_FLAG) )  //read sie1 msg register
	{
		//如果等待时间超过256次，跳转到USB_HOT_PLUG重新开始
		if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
		//resend the get device descriptor
		//get device descriptor
		// TASK: Call the appropriate function again if it wasn't processed successfully.
		UsbGetDeviceDesc2();
		usleep(10*1000);
	}

	//等待TD列表完成，返回0xffff表示失败
	if(0xffff == UsbWaitTDListDone()) goto USB_HOT_PLUG;

	//设置HPI地址为0x0506
	IO_write(HPI_ADDR,0x0506);
	alt_printf("[ENUM PROCESS]:step 4 TD Status Byte is %x\n",IO_read(HPI_DATA));

	IO_write(HPI_ADDR,0x0508);
	usb_ctl_val = IO_read(HPI_DATA);
	alt_printf("[ENUM PROCESS]:step 4 TD Control Byte is %x\n",usb_ctl_val);
	wait_cycle = 0;
	//如果重试次数不为3
	while (usb_ctl_val != 0x03)
	{
		//如果等待时间超过256次，跳转到USB_HOT_PLUG重新开始
		if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
		//获取重试次数
		usb_ctl_val = UsbGetRetryCnt();
	}

	alt_printf("------------[ENUM PROCESS]:get device descriptor-2 done!--------------\n");


	// STEP 5 begin
	//-----------------------------------get configuration descriptor -1 ----------------------------------//
	// TASK: Call the appropriate function for this step.
	UsbGetConfigDesc1(); 	// Get Configuration Descriptor -1

	//if no message
	wait_cycle = 0;
	while (!(IO_read(HPI_STATUS) & HPI_STATUS_SIE1msg_FLAG) )  //read sie1 msg register
	{
		if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
		//resend the get device descriptor
		//get device descriptor

		// TASK: Call the appropriate function again if it wasn't processed successfully.
		UsbGetConfigDesc1();
		usleep(10*1000);
	}

	if(0xffff == UsbWaitTDListDone()) goto USB_HOT_PLUG;

	IO_write(HPI_ADDR,0x0506);
	alt_printf("[ENUM PROCESS]:step 5 TD Status Byte is %x\n",IO_read(HPI_DATA));

	IO_write(HPI_ADDR,0x0508);
	usb_ctl_val = IO_read(HPI_DATA);
	alt_printf("[ENUM PROCESS]:step 5 TD Control Byte is %x\n",usb_ctl_val);
	wait_cycle = 0;
	while (usb_ctl_val != 0x03)
	{
		if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
		usb_ctl_val = UsbGetRetryCnt();
	}
	alt_printf("------------[ENUM PROCESS]:get configuration descriptor-1 pass------------\n");

	// STEP 6 begin
	//-----------------------------------get configuration descriptor-2------------------------------------//
	//get device descriptor
	// TASK: Call the appropriate function for this step.
	UsbGetConfigDesc2(); 	// Get Configuration Descriptor -2

	usleep(100*1000);
	//if no message
	//等待周期计数器归零
	wait_cycle = 0;
	//持续等待，直到收到SIE1消息寄存器的值
	while (!(IO_read(HPI_STATUS) & HPI_STATUS_SIE1msg_FLAG) )  //read sie1 msg register
	{
		//如果等待时间超过256次，跳转到USB_HOT_PLUG重新开始
		if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
		// TASK: Call the appropriate function again if it wasn't processed successfully.
		UsbGetConfigDesc2();
		usleep(10*1000);
	}

	if(0xffff == UsbWaitTDListDone()) goto USB_HOT_PLUG;

	IO_write(HPI_ADDR,0x0506);
	alt_printf("[ENUM PROCESS]:step 6 TD Status Byte is %x\n",IO_read(HPI_DATA));

	IO_write(HPI_ADDR,0x0508);
	usb_ctl_val = IO_read(HPI_DATA);
	alt_printf("[ENUM PROCESS]:step 6 TD Control Byte is %x\n",usb_ctl_val);
	wait_cycle = 0;
	while (usb_ctl_val != 0x03)
	{
		if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
		usb_ctl_val = UsbGetRetryCnt();
	}


	alt_printf("-----------[ENUM PROCESS]:get configuration descriptor-2 done!------------\n");


	// ---------------------------------get device info---------------------------------------------//

	// TASK: Write the address to read from the memory for byte 7 of the interface descriptor to HPI_ADDR.
	//从内存中读取接口描述符的第七个字节的地址，并写入HPI_ADDR
	IO_write(HPI_ADDR,0x056c);
	code = IO_read(HPI_DATA);
	code = code & 0x003;
	alt_printf("\ncode = %x\n", code);		

	if (code == 0x01)
	{
		alt_printf("\n[INFO]:check TD rec data7 \n[INFO]:Keyboard Detected!!!\n\n");
	}
	else
	{
		alt_printf("\n[INFO]:Keyboard Not Detected!!! \n\n");
	}

	// TASK: Write the address to read from the memory for the endpoint descriptor to HPI_ADDR.
	//写入内存中用于读取端口描述符的地址到HPI_ADDR
	IO_write(HPI_ADDR,0x0576);
	IO_write(HPI_DATA,0x073F);
	IO_write(HPI_DATA,0x8105);
	IO_write(HPI_DATA,0x0003);
	IO_write(HPI_DATA,0x0008);
	IO_write(HPI_DATA,0xAC0A);
	UsbWrite(HUSB_SIE1_pCurrentTDPtr,0x0576); 	//HUSB_SIE1_pCurrentTDPtr

	//data_size = (IO_read(HPI_DATA)>>8)&0x0ff;
	//data_size = 0x08;//(IO_read(HPI_DATA))&0x0ff;
	//UsbPrintMem();
	IO_write(HPI_ADDR,0x057c);
	data_size = (IO_read(HPI_DATA))&0x0ff;		//读取端口描述符的数据长度
	alt_printf("[ENUM PROCESS]:data packet size is %d\n",data_size);	//打印数据包长度

	
	// STEP 7 begin
	//------------------------------------set configuration -----------------------------------------//
	// TASK: Call the appropriate function for this step.
	UsbSetConfig();		// Set Configuration
	wait_cycle = 0;
	while (!(IO_read(HPI_STATUS) & HPI_STATUS_SIE1msg_FLAG) )  //read sie1 msg register
	{
		if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
		// TASK: Call the appropriate function again if it wasn't processed successfully.
		//如果没有收到消息，则重新发送设置配置命令
		UsbSetConfig();		// Set Configuration
		usleep(10*1000);
	}

	if(0xffff == UsbWaitTDListDone()) goto USB_HOT_PLUG;

	IO_write(HPI_ADDR,0x0506);
	alt_printf("[ENUM PROCESS]:step 7 TD Status Byte is %x\n",IO_read(HPI_DATA));

	IO_write(HPI_ADDR,0x0508);
	usb_ctl_val = IO_read(HPI_DATA);
	alt_printf("[ENUM PROCESS]:step 7 TD Control Byte is %x\n",usb_ctl_val);
	wait_cycle = 0;
	while (usb_ctl_val != 0x03)
	{
		if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
		usb_ctl_val = UsbGetRetryCnt();
	}

	alt_printf("------------[ENUM PROCESS]:set configuration done!-------------------\n");

	//----------------------------------------------class request out ------------------------------------------//
	// TASK: Call the appropriate function for this step.
	UsbClassRequest();
	wait_cycle = 0;
	while (!(IO_read(HPI_STATUS) & HPI_STATUS_SIE1msg_FLAG) )  //read sie1 msg register
	{
		if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
		// TASK: Call the appropriate function again if it wasn't processed successfully.
		UsbClassRequest();
		usleep(10*1000);
	}

	if(0xffff == UsbWaitTDListDone()) goto USB_HOT_PLUG;

	IO_write(HPI_ADDR,0x0506);
	alt_printf("[ENUM PROCESS]:step 8 TD Status Byte is %x\n",IO_read(HPI_DATA));

	IO_write(HPI_ADDR,0x0508);
	usb_ctl_val = IO_read(HPI_DATA);
	alt_printf("[ENUM PROCESS]:step 8 TD Control Byte is %x\n",usb_ctl_val);
	wait_cycle = 0;
	while (usb_ctl_val != 0x03)
	{
		if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
		usb_ctl_val = UsbGetRetryCnt();
	}


	alt_printf("------------[ENUM PROCESS]:class request out done!-------------------\n");

	// STEP 8 begin
	//----------------------------------get descriptor(class 0x21 = HID) request out --------------------------------//
	// TASK: Call the appropriate function for this step.
	UsbGetHidDesc();
	wait_cycle = 0;
	while (!(IO_read(HPI_STATUS) & HPI_STATUS_SIE1msg_FLAG) )  //read sie1 msg register
	{
		if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
		// TASK: Call the appropriate function again if it wasn't processed successfully.
		UsbGetHidDesc();
		usleep(10*1000);
	}

	if(0xffff == UsbWaitTDListDone()) goto USB_HOT_PLUG;

	IO_write(HPI_ADDR,0x0506);
	alt_printf("[ENUM PROCESS]:step 8 TD Status Byte is %x\n",IO_read(HPI_DATA));

	IO_write(HPI_ADDR,0x0508);
	usb_ctl_val = IO_read(HPI_DATA);
	alt_printf("[ENUM PROCESS]:step 8 TD Control Byte is %x\n",usb_ctl_val);
	wait_cycle = 0;
	while (usb_ctl_val != 0x03)
	{
		if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
		usb_ctl_val = UsbGetRetryCnt();
	}

	alt_printf("------------[ENUM PROCESS]:get descriptor (class 0x21) done!-------------------\n");

	// STEP 9 begin
	//-------------------------------get descriptor (class 0x22 = report)-------------------------------------------//
	// TASK: Call the appropriate function for this step.
	UsbGetReportDesc();
	//if no message
	wait_cycle = 0;
	while (!(IO_read(HPI_STATUS) & HPI_STATUS_SIE1msg_FLAG) )  //read sie1 msg register
	{
		if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
		// TASK: Call the appropriate function again if it wasn't processed successfully.
		UsbGetReportDesc();
		usleep(10*1000);
	}

	if(0xffff == UsbWaitTDListDone()) goto USB_HOT_PLUG;

	IO_write(HPI_ADDR,0x0506);
	alt_printf("[ENUM PROCESS]: step 9 TD Status Byte is %x\n",IO_read(HPI_DATA));

	IO_write(HPI_ADDR,0x0508);
	usb_ctl_val = IO_read(HPI_DATA);
	alt_printf("[ENUM PROCESS]: step 9 TD Control Byte is %x\n",usb_ctl_val);
	wait_cycle = 0;
	while (usb_ctl_val != 0x03)
	{
		if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
		usb_ctl_val = UsbGetRetryCnt();
	}

	alt_printf("---------------[ENUM PROCESS]:get descriptor (class 0x22) done!----------------\n");



	//-----------------------------------get keycode value------------------------------------------------//
	//设置键盘存在标志位为1
	keycode_comm->keyboard_present = 1;
	usleep(10000);
	while(1)
	{
		toggle++;

		//设置读取数据的起始地址
		IO_write(HPI_ADDR,0x0500); //the start address
		//data phase IN-1
		IO_write(HPI_DATA,0x051c); //500

		IO_write(HPI_DATA,0x000f & data_size);//2 data length

		IO_write(HPI_DATA,0x0291);//4 //endpoint 1
		if(toggle%2)
		{
			IO_write(HPI_DATA,0x0001);//6 //data 1
		}
		else
		{
			IO_write(HPI_DATA,0x0041);//6 //data 1
		}
		IO_write(HPI_DATA,0x0013);//8
		IO_write(HPI_DATA,0x0000);//a
		//设置当前传输描述符的指针
		UsbWrite(HUSB_SIE1_pCurrentTDPtr,0x0500); //HUSB_SIE1_pCurrentTDPtr
		
		
		while (!(IO_read(HPI_STATUS) & HPI_STATUS_SIE1msg_FLAG) )  //read sie1 msg register
		{
			IO_write(HPI_ADDR,0x0500); //the start address
			//data phase IN-1
			IO_write(HPI_DATA,0x051c); //500

			IO_write(HPI_DATA,0x000f & data_size);//2 data length 数据长度

			IO_write(HPI_DATA,0x0291);//4 //endpoint 1 端点1
			if(toggle%2)
			{
				IO_write(HPI_DATA,0x0001);//6 //data 1
			}
			else
			{
				IO_write(HPI_DATA,0x0041);//6 //data 1
			}
			IO_write(HPI_DATA,0x0013);//8
			IO_write(HPI_DATA,0x0000);//
			UsbWrite(HUSB_SIE1_pCurrentTDPtr,0x0500); //HUSB_SIE1_pCurrentTDPtr
			usleep(10*1000);
		}//end while

		usb_ctl_val = UsbWaitTDListDone();
		if(0xffff == usb_ctl_val) goto USB_HOT_PLUG;

		// The first two keycodes are stored in 0x051E. Other keycodes are in 
		//第一个和第二个键码存储在0x051E中，其他键码存储在后续地址中
		// subsequent addresses.
		keycode = UsbRead(0x051e);
//		alt_printf("\nfirst two keycode values are %x\n",keycode);
		// We only need the first keycode, which is at the lower byte of keycode.
		// Send the keycode to hardware via PIO.
		//我们只需要第一个键码，它在键码的低字节中
		//通过PIO将键码发送到硬件
		keycode_comm->keycode[0] = keycode & 0xff;
		keycode_comm->keycode[1] = (keycode >> 8) & 0xff;

		usleep(200);//usleep(5000);
		usb_ctl_val = UsbRead(ctl_reg);

		if(!(usb_ctl_val & no_device))
		{
			//USB hot plug routine
			//USB热插拔例程
			for(hot_plug_count = 0 ; hot_plug_count < 7 ; hot_plug_count++)
			{
				usleep(5*1000);
				usb_ctl_val = UsbRead(ctl_reg);
				if(usb_ctl_val & no_device) break;
			}
			//如果设备不存在
			if(!(usb_ctl_val & no_device))
			{
				alt_printf("\n[INFO]: the keyboard has been removed!!! \n");
				alt_printf("[INFO]: please insert again!!! \n");
			}
		}

		//如果设备不存在
		while (!(usb_ctl_val & no_device))
		{

			usb_ctl_val = UsbRead(ctl_reg);
			usleep(5*1000);
			usb_ctl_val = UsbRead(ctl_reg);
			usleep(5*1000);
			usb_ctl_val = UsbRead(ctl_reg);
			usleep(5*1000);

			//如果设备存在，则跳出循环
			if(usb_ctl_val & no_device)
				goto USB_HOT_PLUG;

			usleep(200);
		}
	}//end while

	return 0;
}

/*
设置键盘存在标志为1。
等待一段时间。
进入无限循环。
增加toggle计数器的值。
设置读取数据的起始地址和数据阶段。
写入数据长度、端点和数据。
设置当前传输描述符的指针。
检查是否接收到SIE1消息。
如果未接收到消息，则重复步骤5-8，等待一段时间。
检查传输描述符列表是否完成。
如果传输描述符列表未完成，则跳转到USB_HOT_PLUG标签。
读取键码值，第一个和第二个键码存储在0x051E地址处。
将键码发送到硬件。
等待一段时间。
读取ctl_reg寄存器的值。
如果没有检测到设备，则执行USB热插拔例程。
如果检测到设备，则进入循环。
在循环中，检查设备是否被移除，等待一段时间。
如果设备被移除，则跳转到USB_HOT_PLUG标签。
等待一段时间。
返回0。
*/