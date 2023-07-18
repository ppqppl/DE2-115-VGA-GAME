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
	//IO��ʼ��
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
//USB�Ȳ�α�־λ
USB_HOT_PLUG:
	//���̱�־λ����Ϊ0
	keycode_comm->keyboard_present = 0;
	//ִ��USB������
	UsbSoftReset();

	// STEP 1a:
	//������Ϣ��ַΪ0
	UsbWrite (HPI_SIE1_MSG_ADR, 0);
	//д��Ĵ�����ֵ
	UsbWrite (HOST1_STAT_REG, 0xFFFF);

	/* Set HUSB_pEOT time */
	UsbWrite(HUSB_pEOT, 600); // ����USB�Ĺ淶������EOTʱ��Ϊ600

	//����USB���ƼĴ�����ֵ
	usb_ctl_val = SOFEOP1_TO_CPU_EN | RESUME1_TO_HPI_EN;// | SOFEOP1_TO_HPI_EN;
	UsbWrite(HPI_IRQ_ROUTING_REG, usb_ctl_val);

	//�����ж�״̬
	intStat = A_CHG_IRQ_EN | SOF_EOP_IRQ_EN ;
	UsbWrite(HOST1_IRQ_EN_REG, intStat);
	// STEP 1a end

	// STEP 1b begin
	//����ʱ��
	UsbWrite(COMM_R0,0x0000);//reset time
	//�˿ں�����
	UsbWrite(COMM_R1,0x0000);  //port number
	//�����Ĵ�������Ϊ0
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
	//����ͨ���жϺ�ΪHUSB_SIE1_INIT_INT
	UsbWrite(COMM_INT_NUM,HUSB_SIE1_INIT_INT); //HUSB_SIE1_INIT_INT
	//ͨ��HPI_MAILBOX����ִ���ж�
	IO_write(HPI_MAILBOX,COMM_EXEC_INT);

	wait_cycle = 0;
	//ѭ���ȴ�SIE1��Ϣ�Ĵ�����Ϊ��
	while (!(IO_read(HPI_STATUS) & 0xFFFF) )  //read sie1 msg register
	{
		//����ȴ�ʱ�䳬��256�Σ���ת��USB_HOT_PLUG���¿�ʼ
		if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
	}
	//ѭ���ȴ�ֱ���յ�COMM_ACKȷ��
	while (IO_read(HPI_MAILBOX) != COMM_ACK)
	{
		alt_printf("[ERROR]:routine mailbox data is %x\n",IO_read(HPI_MAILBOX));
		goto USB_HOT_PLUG;
	}
	// STEP 1b end

	alt_printf("STEP 1 Complete");
	// STEP 2 begin
	
	//ִ��USB��λ�����͸�λ����
	UsbWrite(COMM_INT_NUM,HUSB_RESET_INT); //husb reset
	//���ø�λʱ��
	UsbWrite(COMM_R0,0x003c);//reset time
	//���ö˿ں�
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

	//ͨ��HPI_MAILBOX����ִ���ж�
	IO_write(HPI_MAILBOX,COMM_EXEC_INT);

	//�ȴ��ж���Ӧ
	while (IO_read(HPI_MAILBOX) != COMM_ACK)
	{
		alt_printf("[ERROR]:routine mailbox data is %x\n",IO_read(HPI_MAILBOX));
		goto USB_HOT_PLUG;
		//���δ�յ���Ӧ������ת��USB_HOT_PLUG���¿�ʼ
	}
	// STEP 2 end

	ctl_reg = USB1_CTL_REG;					//USB���ƼĴ�����ַ
	no_device = (A_DP_STAT | A_DM_STAT);	//�ж��豸�Ƿ���ڵı�־λ
	fs_device = A_DP_STAT;					//�ж��豸�ٶȵı�־λ
	usb_ctl_val = UsbRead(ctl_reg);			//��ȡUSB���ƼĴ�����ֵ

	
	if (!(usb_ctl_val & no_device))			//����豸������
	{
		//����������
		for(hot_plug_count = 0 ; hot_plug_count < 5 ; hot_plug_count++)
		{
			//�ȴ�5ms
			usleep(5*1000);
			usb_ctl_val = UsbRead(ctl_reg);		//���¶�ȡUSB���ƼĴ�����ֵ
			if(usb_ctl_val & no_device) break;	//����豸���ڣ�������ѭ��
		}
		if(!(usb_ctl_val & no_device))			//����豸���ǲ�����
		{
			//��ӡ��ʾ��Ϣ��STEI1��û���豸
			alt_printf("\n[INFO]: no device is present in SIE1!\n");
			//��ӡ��ʾ��Ϣ����ʾ�û�����USB����
			alt_printf("[INFO]: please insert a USB keyboard in SIE1!\n");
			wait_cycle = 0;
			//ѭ���ȴ���ֱ����⵽�豸����
			while (!(usb_ctl_val & no_device))
			{
				//����ȴ�ʱ�䳬��256�Σ���ת��USB_HOT_PLUG���¿�ʼ
				if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
				//���¶�ȡUSB���ƼĴ�����ֵ
				usb_ctl_val = UsbRead(ctl_reg);
				//����豸���ڣ�������ѭ��
				if(usb_ctl_val & no_device)
					goto USB_HOT_PLUG;

				usleep(2000);
			}
		}
	}
	else
	{
		/* check for low speed or full speed by reading D+ and D- lines */
		//��ƽ���ж��Ƿ�ȫ���豸
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
	//�����ȴ���ֱ���յ�SIE1��Ϣ�Ĵ�����ֵ
	while (!(IO_read(HPI_STATUS) & HPI_STATUS_SIE1msg_FLAG) )  //read sie1 msg register
	{
		//����ȴ�ʱ�䳬��256�Σ���ת��USB_HOT_PLUG���¿�ʼ	
		if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
		// TASK: Call the appropriate function again if it wasn't processed successfully.
		
		UsbGetDeviceDesc1();
		usleep(10*1000);
	}

	if(0xffff == UsbWaitTDListDone()) goto USB_HOT_PLUG;

	
	IO_write(HPI_ADDR,0x0506);
	//��ӡ״̬�ֽڵ�ֵ
	alt_printf("[ENUM PROCESS]:step 4 TD Status Byte is %x\n",IO_read(HPI_DATA));

	IO_write(HPI_ADDR,0x0508);
	usb_ctl_val = IO_read(HPI_DATA);
	
	alt_printf("[ENUM PROCESS]:step 4 TD Control Byte is %x\n",usb_ctl_val);
	wait_cycle = 0;
	//������Դ�����Ϊ3
	while (usb_ctl_val != 0x03)
	{
		if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
		usb_ctl_val = UsbGetRetryCnt();
	}

	alt_printf("---------------[ENUM PROCESS]:get device descriptor-1 done!-----------------\n");


	//--------------------------------get device descriptor-2---------------------------------------------//
	//get device descriptor
	// TASK: Call the appropriate function for this step.
	//��ȡ�豸������
	UsbGetDeviceDesc2(); 	// Get Device Descriptor -2

	//if no message
	wait_cycle = 0;//�ȴ����ڼ���������
	//�����ȴ���ֱ���յ�SIE1��Ϣ�Ĵ�����ֵ
	while (!(IO_read(HPI_STATUS) & HPI_STATUS_SIE1msg_FLAG) )  //read sie1 msg register
	{
		//����ȴ�ʱ�䳬��256�Σ���ת��USB_HOT_PLUG���¿�ʼ
		if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
		//resend the get device descriptor
		//get device descriptor
		// TASK: Call the appropriate function again if it wasn't processed successfully.
		UsbGetDeviceDesc2();
		usleep(10*1000);
	}

	//�ȴ�TD�б���ɣ�����0xffff��ʾʧ��
	if(0xffff == UsbWaitTDListDone()) goto USB_HOT_PLUG;

	//����HPI��ַΪ0x0506
	IO_write(HPI_ADDR,0x0506);
	alt_printf("[ENUM PROCESS]:step 4 TD Status Byte is %x\n",IO_read(HPI_DATA));

	IO_write(HPI_ADDR,0x0508);
	usb_ctl_val = IO_read(HPI_DATA);
	alt_printf("[ENUM PROCESS]:step 4 TD Control Byte is %x\n",usb_ctl_val);
	wait_cycle = 0;
	//������Դ�����Ϊ3
	while (usb_ctl_val != 0x03)
	{
		//����ȴ�ʱ�䳬��256�Σ���ת��USB_HOT_PLUG���¿�ʼ
		if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
		//��ȡ���Դ���
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
	//�ȴ����ڼ���������
	wait_cycle = 0;
	//�����ȴ���ֱ���յ�SIE1��Ϣ�Ĵ�����ֵ
	while (!(IO_read(HPI_STATUS) & HPI_STATUS_SIE1msg_FLAG) )  //read sie1 msg register
	{
		//����ȴ�ʱ�䳬��256�Σ���ת��USB_HOT_PLUG���¿�ʼ
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
	//���ڴ��ж�ȡ�ӿ��������ĵ��߸��ֽڵĵ�ַ����д��HPI_ADDR
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
	//д���ڴ������ڶ�ȡ�˿��������ĵ�ַ��HPI_ADDR
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
	data_size = (IO_read(HPI_DATA))&0x0ff;		//��ȡ�˿������������ݳ���
	alt_printf("[ENUM PROCESS]:data packet size is %d\n",data_size);	//��ӡ���ݰ�����

	
	// STEP 7 begin
	//------------------------------------set configuration -----------------------------------------//
	// TASK: Call the appropriate function for this step.
	UsbSetConfig();		// Set Configuration
	wait_cycle = 0;
	while (!(IO_read(HPI_STATUS) & HPI_STATUS_SIE1msg_FLAG) )  //read sie1 msg register
	{
		if((++wait_cycle) & 0x100) goto USB_HOT_PLUG;
		// TASK: Call the appropriate function again if it wasn't processed successfully.
		//���û���յ���Ϣ�������·���������������
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
	//���ü��̴��ڱ�־λΪ1
	keycode_comm->keyboard_present = 1;
	usleep(10000);
	while(1)
	{
		toggle++;

		//���ö�ȡ���ݵ���ʼ��ַ
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
		//���õ�ǰ������������ָ��
		UsbWrite(HUSB_SIE1_pCurrentTDPtr,0x0500); //HUSB_SIE1_pCurrentTDPtr
		
		
		while (!(IO_read(HPI_STATUS) & HPI_STATUS_SIE1msg_FLAG) )  //read sie1 msg register
		{
			IO_write(HPI_ADDR,0x0500); //the start address
			//data phase IN-1
			IO_write(HPI_DATA,0x051c); //500

			IO_write(HPI_DATA,0x000f & data_size);//2 data length ���ݳ���

			IO_write(HPI_DATA,0x0291);//4 //endpoint 1 �˵�1
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
		//��һ���͵ڶ�������洢��0x051E�У���������洢�ں�����ַ��
		// subsequent addresses.
		keycode = UsbRead(0x051e);
//		alt_printf("\nfirst two keycode values are %x\n",keycode);
		// We only need the first keycode, which is at the lower byte of keycode.
		// Send the keycode to hardware via PIO.
		//����ֻ��Ҫ��һ�����룬���ڼ���ĵ��ֽ���
		//ͨ��PIO�����뷢�͵�Ӳ��
		keycode_comm->keycode[0] = keycode & 0xff;
		keycode_comm->keycode[1] = (keycode >> 8) & 0xff;

		usleep(200);//usleep(5000);
		usb_ctl_val = UsbRead(ctl_reg);

		if(!(usb_ctl_val & no_device))
		{
			//USB hot plug routine
			//USB�Ȳ������
			for(hot_plug_count = 0 ; hot_plug_count < 7 ; hot_plug_count++)
			{
				usleep(5*1000);
				usb_ctl_val = UsbRead(ctl_reg);
				if(usb_ctl_val & no_device) break;
			}
			//����豸������
			if(!(usb_ctl_val & no_device))
			{
				alt_printf("\n[INFO]: the keyboard has been removed!!! \n");
				alt_printf("[INFO]: please insert again!!! \n");
			}
		}

		//����豸������
		while (!(usb_ctl_val & no_device))
		{

			usb_ctl_val = UsbRead(ctl_reg);
			usleep(5*1000);
			usb_ctl_val = UsbRead(ctl_reg);
			usleep(5*1000);
			usb_ctl_val = UsbRead(ctl_reg);
			usleep(5*1000);

			//����豸���ڣ�������ѭ��
			if(usb_ctl_val & no_device)
				goto USB_HOT_PLUG;

			usleep(200);
		}
	}//end while

	return 0;
}

/*
���ü��̴��ڱ�־Ϊ1��
�ȴ�һ��ʱ�䡣
��������ѭ����
����toggle��������ֵ��
���ö�ȡ���ݵ���ʼ��ַ�����ݽ׶Ρ�
д�����ݳ��ȡ��˵�����ݡ�
���õ�ǰ������������ָ�롣
����Ƿ���յ�SIE1��Ϣ��
���δ���յ���Ϣ�����ظ�����5-8���ȴ�һ��ʱ�䡣
��鴫���������б��Ƿ���ɡ�
��������������б�δ��ɣ�����ת��USB_HOT_PLUG��ǩ��
��ȡ����ֵ����һ���͵ڶ�������洢��0x051E��ַ����
�����뷢�͵�Ӳ����
�ȴ�һ��ʱ�䡣
��ȡctl_reg�Ĵ�����ֵ��
���û�м�⵽�豸����ִ��USB�Ȳ�����̡�
�����⵽�豸�������ѭ����
��ѭ���У�����豸�Ƿ��Ƴ����ȴ�һ��ʱ�䡣
����豸���Ƴ�������ת��USB_HOT_PLUG��ǩ��
�ȴ�һ��ʱ�䡣
����0��
*/