#include "include.h"
#include "iic.h"
#include "clib.h"
#if PLXC_UP_EN





//GPIOASET: 把设置为1的位置1. 设置为0的位不影响以前的状态.
//GPIOACLR: 把设置为1的位置0. 设置为0的位不影响以前的状态.

//使用方法如:
//GPIOASET = BIT(6);    //PA0输出高, 等效于 GPIOA |= BIT(6);
//GPIOACLR = BIT(6);    //PA0输出低, 等效于GPIOA &= ~BIT(6);


//GPIOASET = BIT(7);    //PA0输出高, 等效于 GPIOA |= BIT(7);
//GPIOACLR = BIT(6);    //PA0输出低, 等效于GPIOA &= ~BIT(7);

//PB2----SDA   PB1----SCL


//#define iic_SCL_H()                     GPIOBSET = BIT(1)
//#define iic_SCL_L()                     GPIOBCLR = BIT(1)
//
//#define iic_SDA_H()                     GPIOBSET = BIT(2)
//#define iic_SDA_L()                     GPIOBCLR = BIT(2)


//#define READ_SDA GPIOB & BIT(2)



//AT(.com_text.spt50)
void spt51_key_Init(void)
{
    GPIOBFEN &= ~(BIT(SCL)|BIT(SDA));   //PB1,PB2作为GPIO使用
    GPIOBDE  |= (BIT(SCL)|BIT(SDA));    //PB1,PB2设置为数字IO
    GPIOBDIR |= (BIT(SCL)|BIT(SDA));   //PB1,PB2方向设置为输入
    GPIOBPU  |= (BIT(SCL)|BIT(SDA));
//    GPIOBPD  &= ~(BIT(1)|BIT(2));

//    GPIOBDE |= BIT(5);
//    GPIOBDIR |= BIT(5);
//    GPIOBPU |= BIT(5);
}

//AT(.com_text.spt50)
void IIC_LOW(void)
{
    GPIOBDIR &= ~(BIT(SCL)|BIT(SDA));   //PB1,PB2方向设置为输出
    GPIOBCLR = (BIT(SCL) | BIT(SDA));    //PB1,PB2输出0
}

//AT(.com_text.spt50)
void iic_Init(void)
{
    GPIOBFEN &= ~(BIT(SCL)|BIT(SDA));   //PB1,PB2作为GPIO使用
    GPIOBDE  |= (BIT(SCL)|BIT(SDA));    //PB1,PB2设置为数字IO
    GPIOBDIR &= ~(BIT(SCL)|BIT(SDA));   //PB1,PB2方向设置为输出

    GPIOBSET = (BIT(SCL)|BIT(SDA));   //上电默认拉高
}
//AT(.com_text.spt50)
void iic_SDA_IN(void)
{
    GPIOBDIR |= BIT(SDA);    //PB2方向设置为输入
    GPIOBPU |= BIT(SDA);
}
//AT(.com_text.spt50)
void iic_SDA_OUT()
{
    GPIOBDIR &= ~BIT(SDA);   //PB2方向设置为输出
}
//产生IIC起始信号
//AT(.com_text.spt50)
void IIC_Start(void)
{
    iic_SDA_OUT(); //SDA设置为输出

	iic_SDA_H(); //IIC_SDA=1;ii
	iic_SCL_H();//IIC_SCL=1;
	delay_us(5);
	iic_SDA_L();//IIC_SDA=0;//START:when CLK is high,DATA change form high to low
	delay_us(5);
	iic_SCL_L();//IIC_SCL=0;//钳制SCL总线，准备发送和接受数据。
}
//产生IIC停止信号
//AT(.com_text.spt50)
void IIC_Stop(void)
{
    iic_SDA_OUT(); //SDA设置为输出

    iic_SDA_L();//STOP:when CLK is high DATA change form low to high
    delay_us(5);
	iic_SCL_H();//IIC_SCL=1;
	delay_us(5);
	iic_SDA_H(); //IIC_SDA=1;//·发送结束信号
	delay_us(5);
}
//等待应答信号到来
//返回值：1，接收应答失败
//        0，接收应答成功
//AT(.com_text.spt50)
uint8_t IIC_Wait_Ack(void)
{
	uint8_t ucErrTime=0;

	iic_SDA_IN();

	iic_SDA_H(); //IIC_SDA=1;
	delay_us(5);
	iic_SCL_H();//IIC_SCL=1;
	delay_us(5);
	while(READ_SDA)
	{
		ucErrTime++;
		if(ucErrTime>250)
		{
			IIC_Stop();
			return 1;
		}
	}
	iic_SCL_L();//IIC_SCL=0;//时钟输出0
	return 0;
}
//产生ACK应答
//AT(.com_text.spt50)
void IIC_Ack(void)
{
	iic_SDA_OUT();

	iic_SDA_L();
	delay_us(5);
	iic_SCL_H();
	delay_us(5);
	iic_SCL_L();
	delay_us(5);
	iic_SDA_H();
}
//不产生ACK应答
//AT(.com_text.spt50)
void IIC_NAck(void)
{
    iic_SDA_OUT();

	iic_SDA_H(); //IIC_SDA=1;
	delay_us(5);
	iic_SCL_H();//IIC_SCL=1;
	delay_us(5);
	iic_SCL_L();//IIC_SCL=0;
	delay_us(5);
}
//IIC发送一个字节
//返回从机有无应答
//1，有应答
//0，无应答
//AT(.com_text.spt50)
void IIC_Send_Byte(uint8_t txd)
{
    u8 t;
    iic_SDA_OUT();

    iic_SCL_L();//IIC_SCL=0;//拉低时钟开始数据传输
    for(t=0;t<8;t++)
    {
		if((txd&0x80)>>7)
			iic_SDA_H(); //IIC_SDA=1;
		else
			iic_SDA_L();//IIC_SDA=0;
		txd<<=1;
		delay_us(5);   //对TEA5767这三个延时都是必须的
		iic_SCL_H();//IIC_SCL=1;
		delay_us(5);
		delay_us(5);
		iic_SCL_L();//IIC_SCL=0;
		delay_us(5);
    }
}
//读1个字节，ack=1时，发送ACK，ack=0，发送nACK
//AT(.com_text.spt50)
uint8_t IIC_Read_Byte(void)
{
	uint8_t i,receive=0;

	iic_SDA_IN();
    for(i=0;i<8;i++ )
	{
        receive<<=1;
		iic_SCL_H();//IIC_SCL=1;
        delay_us(5);
        if(READ_SDA)
        {
			receive++;
        }
        iic_SCL_L();
		delay_us(5);
    }
    return receive;
}

#define Write 0     		//写标志
#define Read 1     			//读标志

#define EEPROM_Size 64		//EEPROM大小
#define EEPROM_Page_Size 16 //EEPROM页大小
#define Reg_Size 64		    //EEEPROM大小

#define First_PageAddr 0x80		//EEPROM page1首地址
#define Second_PageAddr 0x90	//EEPROM page2首地址
#define Third_PageAddr 0xA0		//EEPROM page3首地址
#define Forth_PageAddr 0xB0		//EEPROM page4首地址

enum BackValue
{
	SPT50_BackValue_default = 0,
	AddrNoAck = 1,
	KeyNoAck = 2,
	WriteFailed = 3,
	WriteSucessed = 4,
	ReadSucessed = 5,
	Earsefailed = 6
};
enum BackValue SPT50_BackValue = SPT50_BackValue_default;


/**
 **************************************************************************
 * @函数名：IIC_Read_register
 * @作者：普林芯驰方案部
 * @版本号：V0.1
 * @日期：2020年5月1日
 * @简介：写读取SPT50寄存器
 * @传参：SPT50地址，读数据缓冲区地址，读SPT50寄存器地址，读数据长度
 * @回参：返回0表示读取成功，返回1表示SPT50无应答
 **************************************************************************
 */
//AT(.com_text.spt50)
uint8_t IIC_Read_Register(uint8_t DevAddr,uint8_t *ReadData,uint8_t RegAddr,uint8_t length)
{
	uint8_t num = 0x00;

	IIC_Start();							//IIC 起始信号
	IIC_Send_Byte((DevAddr<<1)+Write);      //设备地址 + 写
	if(IIC_Wait_Ack())
	{
		return AddrNoAck;
	}
	IIC_Send_Byte(RegAddr);     			//寄存器地址
	IIC_Wait_Ack();	 			 			//IIC等待设备回ACK
	IIC_Stop();					 			//IIC停止信号


	IIC_Start();
	IIC_Send_Byte((DevAddr<<1)+Read);		//设备地址 + 读
	IIC_Wait_Ack();
	for(num = 0;num<length;num++)
	{
		ReadData[num] = IIC_Read_Byte();
		if (num < (length -1))
			IIC_Ack();
		else
			IIC_NAck();						//最后一个读数据，主机不返回ACK
	}
	IIC_Stop();

	return ReadSucessed;
}
/**
 **************************************************************************
 * @函数名：IIC_Write_register
 * @作者：普林芯驰方案部
 * @版本号：V0.1
 * @日期：2020年5月1日
 * @简介：写SPT50寄存器
 * @传参：SPT50地址，写数据，写SPT50寄存器地址，写数据长度
 * @回参：返回0表示写成功，返回1表示SPT50无应答，3表示写失败
 **************************************************************************
 */
//AT(.com_text.spt50)
uint8_t IIC_Write_Register(uint8_t DevAddr,uint8_t *WriteData,uint8_t RegAddr,uint8_t length)
{
	uint8_t num = 0x00;

	IIC_Start();
	IIC_Send_Byte((DevAddr<<1)+Write);      				//设备地址 写
	if(IIC_Wait_Ack())
	{
		return AddrNoAck;
	}
	IIC_Send_Byte(RegAddr);      							//寄存器地址
	IIC_Wait_Ack();

	for(num=0;num<length;num++)
	{
		IIC_Send_Byte(WriteData[num]);						//写数据
		IIC_Wait_Ack();
	}
	IIC_Stop();


	return WriteSucessed;
}
/**
 **************************************************************************
 * @函数名：IIC_Read_EEPROM
 * @作者：普林芯驰方案部
 * @版本号：V0.1
 * @日期：2020年5月1日
 * @简介：写SPT50寄存器
 * @传参：SPT50地址，读数据，读SPT50寄存器起始地址，读数据长度
 * @回参：返回0表示读取成功，返回1表示SPT50无应答
 **************************************************************************
 */
//AT(.com_text.spt50)
uint8_t IIC_Read_EEPROM(uint8_t DevAddr,uint8_t *ReadData,uint8_t EepAddr,uint8_t length)
{
	uint8_t num = 0x00;

	IIC_Start();							//IIC起始信号
	IIC_Send_Byte((DevAddr<<1)+Write);      //地址 + 写
	if(IIC_Wait_Ack())
	{
		return AddrNoAck ;
	}
	IIC_Send_Byte(0xE0);      				//地址 + 写
	IIC_Wait_Ack();
	IIC_Send_Byte(0x10);      				//地址 + 写
	IIC_Wait_Ack();
	IIC_Stop();								//IIC终止信号

	IIC_Start();
	IIC_Send_Byte((DevAddr<<1)+Write);      //地址 + 写
	IIC_Wait_Ack();
	IIC_Send_Byte(EepAddr);      			//地址 + 写
	IIC_Wait_Ack();
	IIC_Stop();

	IIC_Start();
	IIC_Send_Byte((DevAddr<<1)+Read);      //地址 + 读
	IIC_Wait_Ack();
	for(num = 0;num<length;num++)
	{
		ReadData[num] = IIC_Read_Byte();	//读数据
		if(num < (length - 1))
            IIC_Ack();							//ACK
        else
        {
            IIC_Stop();
        }
	}

	IIC_Start();
	IIC_Send_Byte((DevAddr<<1)+Write);      //地址 + 写
	IIC_Wait_Ack();
	IIC_Send_Byte(0xE0);      //地址 + 写
	IIC_Wait_Ack();
	IIC_Send_Byte(0x00);      //地址 + 写
	IIC_Wait_Ack();
	IIC_Stop();

	return ReadSucessed ;
}
/**
 **************************************************************************
 * @函数名：IIC_Write_Page_EEPROM
 * @作者：普林芯驰方案部
 * @版本号：V0.1
 * @日期：2020年5月1日
 * @简介：写SPT50寄存器
 * @传参：SPT50地址，写数据，写SPT50页寄存器起始地址，写数据缓冲器起始索引
 * @回参：返回0表示写成功，返回1表示SPT50无应答，3表示页写失败
 **************************************************************************
 */
//AT(.com_text.spt50)
uint8_t IIC_Write_Page_EEPROM(uint8_t DevAddr,uint8_t *WriteData,uint8_t PageAddr,uint8_t start_index)
{
	uint8_t num = 0;
	//uint8_t ReadData_Page[EEPROM_Page_Size]={0x00,0x00};
	uint8_t WriteData_Page[EEPROM_Page_Size] = {0x00,0x00};


	for(num = 0;num < EEPROM_Page_Size;num++)
	{
		WriteData_Page[num] = WriteData[num+start_index];
	}


	IIC_Start();
	IIC_Send_Byte((DevAddr<<1)+Write);      //地址 + 写
	IIC_Wait_Ack();
	IIC_Send_Byte(0xE0);      //地址 + 写
	IIC_Wait_Ack();
	IIC_Send_Byte(0x30);      //地址 + 写
	IIC_Wait_Ack();
	IIC_Stop();

	//--------------------读SPT50的EEPROM地址
	IIC_Start();
	IIC_Send_Byte((DevAddr<<1)+Write);      //地址 + 写
	IIC_Wait_Ack();
	IIC_Send_Byte(PageAddr);      //地址 + 写
	IIC_Wait_Ack();

	for(num = 0;num < EEPROM_Page_Size;num++)
	{
		IIC_Send_Byte(WriteData_Page[num]);
		IIC_Wait_Ack();
	}
	IIC_Stop();

	delay_ms(8);

	return WriteSucessed;
}
/**
 **************************************************************************
 * @函数名：IIC_Write_ALL_EEPROM
 * @作者：普林芯驰方案部
 * @版本号：V0.1
 * @日期：2020年5月1日
 * @简介：写SPT50寄存器
 * @传参：SPT50地址，写数据，写SPT50寄存器地址，写数据长度
 * @回参：返回0表示读取成功，返回1表示SPT50无应答，3表示写失败，4表示擦除失败
 **************************************************************************
 */
//AT(.com_text.spt50)
uint8_t IIC_Write_ALL_EEPROM(uint8_t DevAddr,uint8_t *WriteData,uint8_t EepAddr,uint8_t length)
{
	uint8_t num = 0;
	uint8_t ReadData[EEPROM_Size]={0x00,0x00};
	//--------------------擦除EEPROM chip
	IIC_Start();
	IIC_Send_Byte((DevAddr<<1)+0x00);      //地址 + 写
	if(IIC_Wait_Ack())
	{
		return AddrNoAck;
	}
	IIC_Send_Byte(0xE0);      //地址 + 写
	IIC_Wait_Ack();
	IIC_Send_Byte(0x50);      //地址 + 写
	IIC_Wait_Ack();
	IIC_Stop();

	delay_ms(5);


	IIC_Read_EEPROM(DevAddr,ReadData,0x80,64);

	for(num = 0;num < EEPROM_Size;num++)
	{
		if (ReadData[num] != 0xff)
		{
			return Earsefailed;
		}
	}
	IIC_Write_Page_EEPROM(DevAddr,WriteData,First_PageAddr,0);
	IIC_Write_Page_EEPROM(DevAddr,WriteData,Second_PageAddr,16);
	IIC_Write_Page_EEPROM(DevAddr,WriteData,Third_PageAddr,32);
	IIC_Write_Page_EEPROM(DevAddr,WriteData,Forth_PageAddr,48);

	return WriteSucessed;
}
/**
 **************************************************************************
 * @函数名：SPT50_Always_Upgrade
 * @作者：普林芯驰方案部
 * @版本号：V0.1
 * @日期：2020年5月1日
 * @简介：一直upgrade
 * @传参：SPT50地址
 * @回参：返回0表示写成功，返回1表示SPT50无应答，3表示写失败
 **************************************************************************
 */
//AT(.com_text.spt50)
uint8_t SPT51_Always_Upgrade(uint8_t DevAddr)
{
	uint8_t data = 0x80;  //bit
	uint8_t PRST_CTL = 0x18;
    uint8_t PWR_RTL = 0x0B;


    IIC_Read_Register(DevAddr,&data,PWR_RTL,1);
    data |= 0x08;
	IIC_Write_Register(DevAddr,&data,PWR_RTL,1);
    delay_ms(1);

    IIC_Read_Register(DevAddr,&data,PRST_CTL,1);
    data |= 0x80;
    IIC_Write_Register(DevAddr,&data,PRST_CTL,1);
    delay_ms(1);

	return 0;
}
/**
 **************************************************************************
 * @函数名：SPT50_Reboot
 * @作者：普林芯驰方案部
 * @版本号：V0.1
 * @日期：2020年5月1日
 * @简介：重启SPT50
 * @传参：SPT50地址
 * @回参：返回0表示写成功，返回1表示SPT50无应答，3表示写失败
 **************************************************************************
 */
//AT(.com_text.spt50)
uint8_t SPT51_Reboot(uint8_t DevAddr)
{
	//7:1 RESERVED WO Reserved 0
	//1 RE_BOOT WO System Reboot
	//Write 1 to Reboot
	//0
	//0 SYS_ACK WO System Acknowledge
	//I2C_CTRL.INTO_TYPE=0,
	//When INTO=0,need write 1 to acknowledge

	uint8_t SYS_CTRL_Reboot = 0x02;  //bit
	uint8_t SYS_CTRL_Addr = 0xC8;

	return (IIC_Write_Register(DevAddr,&SYS_CTRL_Reboot,SYS_CTRL_Addr,1));
}
/**
 **************************************************************************
 * @函数名：SPT50_Reset
 * @作者：普林芯驰方案部
 * @版本号：V0.1
 * @日期：2020年5月1日
 * @简介：复位SPT50
 * @传参：无
 * @回参：无
 **************************************************************************
 */
void SPT51_Reset(void)
{
    iic_SDA_OUT();

	iic_SCL_L();//IIC_SCL=0;
	iic_SDA_L();//IIC_SDA=0;
	delay_ms(10);
	iic_SCL_H();//IIC_SCL=1;
	iic_SDA_H();//IIC_SDA=1;
	delay_ms(30);
}
/**
 **************************************************************************
 * @函数名：SPT50_GetAccess
 * @作者：普林芯驰方案部
 * @版本号：V0.1
 * @日期：2020年5月1日
 * @简介：复位SPT50
 * @传参：无
 * @回参：0表示成功，1表示失败
 **************************************************************************
 */
//AT(.com_text.spt50)
uint8_t SPT51_GetAccess(uint8_t DevAddr,uint8_t *WriteData)
{
    uint8_t BackData_00 = 0x00;
    uint8_t BackData_80 = 0x00;
	IIC_Write_Register(DevAddr,&WriteData[0],0xC0,1);
	delay_ms(1);
	IIC_Write_Register(DevAddr,&WriteData[1],0xC1,1);
    delay_ms(1);
    IIC_Read_Register(DevAddr,&BackData_00,0x00,1);
    delay_ms(1);
    IIC_Read_Register(DevAddr,&BackData_80,0xCA,1);
    delay_ms(1);
    IIC_Read_EEPROM(DevAddr,&BackData_80,0x80,1);
    delay_ms(1);
    if((BackData_00 == 0x56)&&(BackData_80 == 0x56)){
        return 0;
    }
    else{
        return 1;
    }
}
//AT(.com_text.spt50)
void SPT51_INTI(void)
{
    iic_SCL_L();
    delay_ms(1);
    iic_SCL_H();
    delay_ms(10);
    return;
}

void SPT51_SDA_Reset(void)
{
    iic_SDA_OUT();

    iic_SDA_L();
    delay_ms(10);
    iic_SDA_L();
    delay_ms(30);
    return;
}

void SPT51_SDA_Reset_Main(void)
{
    iic_SDA_OUT();

    iic_SDA_L();
    delay_ms(10);
    iic_SDA_L();

    return;
}
uint8_t SPT51_Addr = 0x60;		//存储接收数据的地址
bool FristAccessIIC = false;
uint8_t SPT51_FlagData = 0x00;
//AT(.com_text.spt50)
void user_bt_app_cmd_process(u8 *ptr, u16 size)
{
    ///用户的spp数据处理
    uint8_t Read_EEPROM_Data[64] = {0x00,0x00};  // 读取EEPRPROM
    uint8_t Write_EEPROM_Data[64] = {0x00,0x00};// 写EEPRPROM

    uint8_t SPT51_Addr = 0x60;		//存储接收数据的地址

    uint8_t ptr_ACK[80] = {0x00,0x00};
    uint8_t RxCheck = 0x00;
    uint8_t num = 0;
    uint8_t num_num = 0;

    uint8_t SPT51_Key[2]={0x01,0x00};    // 允许访问key


    if((ptr[0] == 0x50)&&(ptr[size-1] == 0x4C))
    {
        for(num = 1;num<(ptr[3]+4);num++)
        {
            RxCheck ^= ptr[num];
        }
        if(RxCheck == ptr[size-2])
        {
            if(ptr[2] == 0x82)//Reset
            {
                SPT51_SDA_Reset();
                ptr_ACK[0] = 0x50;
                ptr_ACK[1] = 0x00;
                ptr_ACK[2] = 0x01;
                ptr_ACK[3] = 0x01;
                ptr_ACK[4] = 0x00;
                ptr_ACK[5] = 0x00;
                ptr_ACK[6] = 0x4C;

                for(num_num = 1;num_num<5;num_num++)
                {
                    ptr_ACK[5] ^= ptr_ACK[num_num];
                }
                spt51_key_Init();
                FristAccessIIC = false;
                bt_spp_tx(ptr_ACK, 7);
            }
            else if(ptr[2] == 0x83)//Write EEPROM
            {
                for(num = 0;num<64;num++)
                {
                    Write_EEPROM_Data[num] = ptr[num+4];
                }

                SPT51_SDA_Reset();
                iic_Init();

                if(SPT51_GetAccess(SPT51_Addr,SPT51_Key) == 0)
                {
                    SPT51_Always_Upgrade(SPT51_Addr);

                    IIC_Read_EEPROM(SPT51_Addr,Read_EEPROM_Data,0x80,16);

                    Write_EEPROM_Data[0x07] &= ~(0x1F);
                    Write_EEPROM_Data[0x08] &= ~(0x3F);
                    Write_EEPROM_Data[0x07] |= Read_EEPROM_Data[0x07];
                    Write_EEPROM_Data[0x08] |= Read_EEPROM_Data[0x08];

                    IIC_Write_ALL_EEPROM(SPT51_Addr,Write_EEPROM_Data,0x80,64);

                    IIC_Read_EEPROM(SPT51_Addr,Read_EEPROM_Data,0x80,64);

                    for(num = 0;num<64;num++)
                    {
                        if(Write_EEPROM_Data[num] == Read_EEPROM_Data[num])
                        {
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }
                    ptr_ACK[0] = 0x50;
                    ptr_ACK[1] = 0x00;
                    ptr_ACK[2] = 0x01;
                    ptr_ACK[3] = 0x01;
                    ptr_ACK[5] = 0x00;
                    ptr_ACK[6] = 0x4C;
                    if(num < 64)
                    {
                        ptr_ACK[4] = 0x01;

                    }
                    else
                    {
                        ptr_ACK[4] = 0x00;
                    }
                    for(num_num = 1;num_num<5;num_num++)
                    {
                        ptr_ACK[5] ^= ptr_ACK[num_num];
                    }
                    SPT51_Reboot(SPT51_Addr);
                    FristAccessIIC = false;
                    bt_spp_tx(ptr_ACK, 7);
                }
            }
            else if(ptr[2] == 0x84)//Read EEPROM data
            {
                SPT51_SDA_Reset();
                iic_Init();
                if(SPT51_GetAccess(SPT51_Addr,SPT51_Key) == 0)
                {
                    SPT51_Always_Upgrade(SPT51_Addr);

                    ptr_ACK[0] = 0x50;
                    ptr_ACK[1] = 0x00;
                    ptr_ACK[2] = 0x02;
                    ptr_ACK[3] = 0x40;

                    IIC_Read_EEPROM(SPT51_Addr,Read_EEPROM_Data,0x80,0x40);
                    SPT51_Reboot(SPT51_Addr);
                    FristAccessIIC = false;
                    for(num=0;num<64;num++)
                    {
                        ptr_ACK[4+num] = Read_EEPROM_Data[num];
                    }
                    for(num=1;num<68;num++)
                    {
                        ptr_ACK[68] ^= ptr_ACK[num];
                    }
                    ptr_ACK[69] = 0x4C;
                    bt_spp_tx(ptr_ACK, ptr_ACK[3]+6);
                }
            }
            else if(ptr[2] == 0x85)//Read debug data
            {
                if(FristAccessIIC == false)
                {
                    SPT51_INTI();//SPT51_INTI();
                    iic_Init();

                    if(SPT51_GetAccess(SPT51_Addr,SPT51_Key) == 0)
                    {
                        FristAccessIIC = true;
                        SPT51_Always_Upgrade(SPT51_Addr);
                    }
                    else
                    {
                        return;
                    }
                }
                IIC_Read_Register(SPT51_Addr,&num,0x00,1);
                if(num == 0x56)
                {
                    IIC_Read_Register(SPT51_Addr,&ptr_ACK[4],0x40,35);
                    ptr_ACK[0] = 0x50;
                    ptr_ACK[1] = 0x00;
                    ptr_ACK[2] = 0x03;
                    ptr_ACK[3] = 0x23;

                    for(num=1;num<(4+35);num++)
                    {
                        ptr_ACK[4+35] ^= ptr_ACK[num];
                    }
                    ptr_ACK[4+35+1] = 0x4C;
                    bt_spp_tx(ptr_ACK, ptr_ACK[3]+6);
                }
                else
                {
                    FristAccessIIC = false;
                }
            }
            else if(ptr[2] == 0x87)
            {
                SPT51_SDA_Reset();
                iic_Init();

                if(SPT51_GetAccess(SPT51_Addr,SPT51_Key) == 0)
                {
                    SPT51_Always_Upgrade(SPT51_Addr);

                    ptr_ACK[0] = 0x50;
                    ptr_ACK[1] = 0x00;
                    ptr_ACK[2] = 0x04;
                    ptr_ACK[3] = ptr[3]+ptr[5];

                    if((ptr[4] <= 0xBF)&&(ptr[4] >= 0x80))
                    {
                        return;
                    }
                    else
                    {
                        IIC_Read_Register(SPT51_Addr,&ptr_ACK[6],ptr[4],ptr[5]);
                    }
                    ptr_ACK[4] = ptr[4];
                    ptr_ACK[5] = ptr[5];

                    for(num=1;num<(ptr[5] + 6);num++)
                    {
                        ptr_ACK[ptr[5] + 6] ^= ptr_ACK[num];
                    }
                    ptr_ACK[ptr_ACK[5] + 7] = 0x4C;

                    bt_spp_tx(ptr_ACK, ptr_ACK[5] + 8);
                }
                else
                {
                    return;
                }
            }
        }
    }
}

uint8_t EEPROM[64]={0};

//uint8_t EEPROM[64]=
//{
//	0x56,0x00,0x00,0x00,0xA7,0x03,0x77,0x1F,0x3F,0x00,0xC2,0x00,0x01,0x00,0x3F,0x3F,
//	0x00,0xF0,0x02,0x00,0x00,0x00,0xBB,0x44,0x05,0x40,0x21,0x3E,0x3B,0x3E,0x00,0x00,
//	0x20,0x18,0xDB,0x2F,0x5B,0x10,0x20,0xA8,0x30,0x20,0xA8,0x11,0x18,0x88,0x16,0x16,
//	0x14,0x20,0xA8,0x22,0x28,0x88,0x28,0x16,0x28,0x20,0x20,0x11,0x18,0x88,0x22,0x22
//};
uint8_t EEPROMRelease(void)//-----------return 1(Error);return 0(success)
{
	printf("EEPROMRelease\n");
	uint8_t num = 0;
	uint8_t cnt = 0;
	uint8_t num1 = 0;
	uint8_t Errornum = 0;
	uint8_t FirstPageData[16] = {0x00,0x00};
	uint8_t ReadEEPROMData[64] = {0x00,0x00};
	uint8_t SPT51_Addr = 0x60;
	uint8_t SPT51_Key[2] = {0x01,0x00};
	//EEPROM = (uint8_t)sys_cb.wl_spt51_errpom;
#if PLXC_UP_EN
	for(num1 = 0;num1<64;num1++)
	{

		EEPROM[num1] = sys_cb.wl_spt51_errpom[num1];
		printf("%x ",EEPROM[num1]);
	}
#endif
	SPT51_SDA_Reset();
	iic_Init();

	printf("EEPROMRelease 1\n");
	for(cnt = 0;cnt<3;cnt++)
	{

		if(SPT51_GetAccess(SPT51_Addr,SPT51_Key) == 0){
			break;
		}else{
			SPT51_SDA_Reset();
		}

	}


	if(SPT51_GetAccess(SPT51_Addr,SPT51_Key) == 0)
	{

		printf("EEPROMRelease 2\n");
		SPT51_Always_Upgrade(SPT51_Addr);
		printf("EEPROMRelease 2.5\n");
		IIC_Read_EEPROM(SPT51_Addr,FirstPageData,0x80,16);

		printf("EEPROMRelease 2.6\n");
		if(FirstPageData[0] == 0x56)
		{
			printf("EEPROMRelease 2.7\n");
			for(num = 0;num<16;num++)
			{

				printf("EEPROMRelease 3\n");
				EEPROM[num] = FirstPageData[num];
			}
			EEPROM[0x0C] = 0x01;
			EEPROM[0x0D] = 0x00;
			EEPROM[0x0E] = 0x3F;
			EEPROM[0x0F] = 0x3F;

			IIC_Write_ALL_EEPROM(SPT51_Addr,EEPROM,0x80,64);
			delay_ms(10);
			IIC_Read_EEPROM(SPT51_Addr,ReadEEPROMData,0x80,64);
			SPT51_Reboot(SPT51_Addr);
			for(num=0;num<64;num++)
			{
				if(EEPROM[num] != ReadEEPROMData[num])
				{
					Errornum ++;
					printf("num = %d\n",num);
					printf("Errornum = %d\n",Errornum);
					break;
				}
				else
				{
					continue;
				}
			}
			if(Errornum > 0)
			{
				printf("Errornum = 111111111111111111\n");
				return 1;
			}
			else
			{

				printf("Errornum = 000000000000000000000\n");
				return 2;
			}
		}
		SPT51_Reboot(SPT51_Addr);
		return 1;
	}
	return 1;
}

//uint8_t EEPROM_GUDING[64]=
//{
//	0x56,0x00,0x00,0x00,0xA7,0x03,0x77,0x1F,0x3F,0x00,0xC2,0x00,0x01,0x00,0x3F,0x3F,
//	0x00,0xF0,0x02,0x00,0x00,0x00,0xBB,0x44,0x05,0x40,0x21,0x3E,0x3B,0x3E,0x00,0x00,
//	0x20,0x18,0xDB,0x2F,0x5B,0x10,0x20,0xA8,0x30,0x20,0xA8,0x11,0x18,0x88,0x16,0x16,
//	0x14,0x20,0xA8,0x22,0x28,0x88,0x28,0x16,0x28,0x20,0x20,0x11,0x18,0x88,0x22,0x22
//};
uint8_t EEPROM_GUDING_V11[64]={
	0x56 ,0x00 ,0x00,0x00,0xA7,0x03,0x77,0x1F,0x3F,0x00,0xC2,0x00,0x01,0x00,0x3F,0x3F,
	0x00 ,0xF0 ,0x02,0x00,0x00,0x00,0xBB,0x44,0x05,0x40,0x21,0x3E,0x3B,0x3E,0x00,0x00,
	0x20 ,0x18 ,0xDB,0x2F,0x5B,0x10,0x20,0xA8,0x30,0x20,0xA8,0x11,0x18,0x99,0x16,0x0F,
	0x14 ,0x20 ,0xA8,0x22,0x28,0x88,0x28,0x0F,0x28,0x20,0x20,0x11,0x18,0x88,0x22,0x22
};

//version V1.2参数
uint8_t EEPROM_GUDING_V12[64]={
	0x56,0x00,0x00,0x00,0xA7,0x03,0x77,0x1F,0x3F,0x00,0xC2,0x00,0x01,0x00,0x3F,0x3F,
	0x00,0xF0,0x02,0x00,0x00,0x00,0xBB,0x44,0x05,0x40,0x21,0x3E,0x3B,0x3E,0x00,0x00,
	0x20,0x18,0xDB,0x2F,0x5B,0x10,0x20,0xA8,0x30,0x20,0xA8,0x11,0x18,0x99,0x16,0x0F,
	0x14,0x20,0xA8,0x22,0x28,0x88,0x16,0x0C,0x28,0x20,0x20,0x11,0x18,0x88,0x22,0x22
};
//800参数
uint8_t EEPROM_GUDING_800V[64]={
	0x56,0x00,0x00,0x00,0xA7,0x03,0x77,0x1F,0x3F,0x00,0xC2,0x00,0x01,0x00,0x3F,0x3F,
	0x00,0xF0,0x02,0x00,0x00,0x00,0xBB,0x44,0x05,0x40,0x21,0x3E,0x3B,0x3E,0x00,0x00,
	0x20,0x18,0xDB,0x2F,0x5B,0x10,0x20,0xA8,0x30,0x20,0xA8,0x11,0x18,0x99,0x16,0x0F,
	0x14,0x20,0xA8,0x22,0x28,0x88,0x22,0x0F,0x28,0x20,0x20,0x11,0x18,0x88,0x22,0x22
};

uint8_t EEPROM_GUDING_816V[64]={
0x56,0x00,0x00,0x00,0xA7,0x03,0x77,0x0D,0x25,0x00,0xC2,0x00,0x01,0x00,0x3F,0x3F,
0x00,0xF0,0x02,0x00,0x00,0x00,0xBB,0x44,0x05,0x40,0x21,0x3E,0x3B,0x3E,0x00,0x00,
0x20,0x18,0xDB,0x2F,0x5B,0x10,0x20,0xA8,0x30,0x20,0xA8,0x11,0x18,0x99,0x23,0x16,
0x14,0x20,0xA8,0x22,0x28,0x88,0x23,0x16,0x28,0x20,0x20,0x11,0x18,0x88,0x22,0x22
};

uint8_t EEPROM_GUDING_V15[64]={
0x56,0x00,0x00,0x00,0xA7,0x03,0x77,0x0D,0x25,0x00,0xC2,0x00,0x01,0x00,0x3F,0x3F,
0x00,0xF0,0x02,0x00,0x00,0x00,0xBB,0x44,0x05,0x40,0x21,0x3E,0x3B,0x3E,0x00,0x00,
0x20,0x18,0xDB,0x2F,0x5B,0x10,0x20,0xA8,0x30,0x20,0xA8,0x11,0x18,0x88,0x23,0x16,
0x14,0x20,0xA8,0x22,0x28,0x8A,0x35,0x16,0x28,0x20,0x20,0x11,0x18,0x88,0x22,0x22
};


uint8_t EEPROMRelease2(void)//-----------return 1(Error);return 0(success)
{
	printf("EEPROMRelease2\n");
	uint8_t num = 0;
	uint8_t cnt = 0;
	uint8_t Errornum = 0;
	uint8_t FirstPageData[16] = {0x00,0x00};
	uint8_t ReadEEPROMData[64] = {0x00,0x00};
	uint8_t SPT51_Addr = 0x60;
	uint8_t SPT51_Key[2] = {0x01,0x00};
	SPT51_SDA_Reset();
	iic_Init();

	//printf("EEPROMRelease 1\n");
	for(cnt = 0;cnt<3;cnt++)
	{

		if(SPT51_GetAccess(SPT51_Addr,SPT51_Key) == 0){
			break;
		}else{
			SPT51_SDA_Reset();
		}

	}


	if(SPT51_GetAccess(SPT51_Addr,SPT51_Key) == 0)
	{

		//printf("EEPROMRelease 2\n");
		SPT51_Always_Upgrade(SPT51_Addr);
		//printf("EEPROMRelease 2.5\n");
		IIC_Read_EEPROM(SPT51_Addr,FirstPageData,0x80,16);

		//printf("EEPROMRelease 2.6\n");
		if(FirstPageData[0] == 0x56)
		{
			//printf("EEPROMRelease 2.7\n");
			for(num = 0;num<16;num++)
			{

				//printf("EEPROMRelease 3\n");
				EEPROM_GUDING_V11[num] = FirstPageData[num];
			}
			EEPROM_GUDING_V11[0x0C] = 0x01;
			EEPROM_GUDING_V11[0x0D] = 0x00;
			EEPROM_GUDING_V11[0x0E] = 0x3F;
			EEPROM_GUDING_V11[0x0F] = 0x3F;

			IIC_Write_ALL_EEPROM(SPT51_Addr,EEPROM_GUDING_V11,0x80,64);
			delay_ms(10);
			IIC_Read_EEPROM(SPT51_Addr,ReadEEPROMData,0x80,64);
			SPT51_Reboot(SPT51_Addr);
			for(num=0;num<64;num++)
			{
				if(EEPROM_GUDING_V11[num] != ReadEEPROMData[num])
				{
					Errornum ++;
					//printf("num = %d\n",num);
					//printf("Errornum = %d\n",Errornum);
					break;
				}
				else
				{
					continue;
				}
			}
			if(Errornum > 0)
			{
				printf("Errornum = 111111111111111111\n");
				return 1;
			}
			else
			{

				printf("Errornum = 000000000000000000000\n");
				return 2;
			}
		}
		SPT51_Reboot(SPT51_Addr);
		return 1;
	}
	return 1;
}

uint8_t EEPROMRelease3(void)//-----------return 1(Error);return 0(success)
{
	printf("EEPROMRelease2\n");
	uint8_t num = 0;
	uint8_t cnt = 0;
	uint8_t Errornum = 0;
	uint8_t FirstPageData[16] = {0x00,0x00};
	uint8_t ReadEEPROMData[64] = {0x00,0x00};
	uint8_t SPT51_Addr = 0x60;
	uint8_t SPT51_Key[2] = {0x01,0x00};
	SPT51_SDA_Reset();
	iic_Init();

	//printf("EEPROMRelease 1\n");
	for(cnt = 0;cnt<3;cnt++)
	{

		if(SPT51_GetAccess(SPT51_Addr,SPT51_Key) == 0){
			break;
		}else{
			SPT51_SDA_Reset();
		}

	}


	if(SPT51_GetAccess(SPT51_Addr,SPT51_Key) == 0)
	{

		//printf("EEPROMRelease 2\n");
		SPT51_Always_Upgrade(SPT51_Addr);
		//printf("EEPROMRelease 2.5\n");
		IIC_Read_EEPROM(SPT51_Addr,FirstPageData,0x80,16);

		//printf("EEPROMRelease 2.6\n");
		if(FirstPageData[0] == 0x56)
		{
			//printf("EEPROMRelease 2.7\n");
			for(num = 0;num<16;num++)
			{

				//printf("EEPROMRelease 3\n");
				EEPROM_GUDING_V12[num] = FirstPageData[num];
			}
			EEPROM_GUDING_V12[0x0C] = 0x01;
			EEPROM_GUDING_V12[0x0D] = 0x00;
			EEPROM_GUDING_V12[0x0E] = 0x3F;
			EEPROM_GUDING_V12[0x0F] = 0x3F;

			IIC_Write_ALL_EEPROM(SPT51_Addr,EEPROM_GUDING_V12,0x80,64);
			delay_ms(10);
			IIC_Read_EEPROM(SPT51_Addr,ReadEEPROMData,0x80,64);
			SPT51_Reboot(SPT51_Addr);
			for(num=0;num<64;num++)
			{
				if(EEPROM_GUDING_V12[num] != ReadEEPROMData[num])
				{
					Errornum ++;
					//printf("num = %d\n",num);
					//printf("Errornum = %d\n",Errornum);
					break;
				}
				else
				{
					continue;
				}
			}
			if(Errornum > 0)
			{
				return 1;
			}
			else
			{
				return 2;
			}
		}
		SPT51_Reboot(SPT51_Addr);
		return 1;
	}
	return 1;
}


uint8_t EEPROMRelease_800v(void)//-----------return 1(Error);return 0(success)
{
	printf("EEPROMRelease2\n");
	uint8_t num = 0;
	uint8_t cnt = 0;
	uint8_t Errornum = 0;
	uint8_t FirstPageData[16] = {0x00,0x00};
	uint8_t ReadEEPROMData[64] = {0x00,0x00};
	uint8_t SPT51_Addr = 0x60;
	uint8_t SPT51_Key[2] = {0x01,0x00};
	SPT51_SDA_Reset();
	iic_Init();

	//printf("EEPROMRelease 1\n");
	for(cnt = 0;cnt<3;cnt++)
	{

		if(SPT51_GetAccess(SPT51_Addr,SPT51_Key) == 0){
			break;
		}else{
			SPT51_SDA_Reset();
		}

	}


	if(SPT51_GetAccess(SPT51_Addr,SPT51_Key) == 0)
	{

		//printf("EEPROMRelease 2\n");
		SPT51_Always_Upgrade(SPT51_Addr);
		//printf("EEPROMRelease 2.5\n");
		IIC_Read_EEPROM(SPT51_Addr,FirstPageData,0x80,16);

		//printf("EEPROMRelease 2.6\n");
		if(FirstPageData[0] == 0x56)
		{
			//printf("EEPROMRelease 2.7\n");
			for(num = 0;num<16;num++)
			{

				//printf("EEPROMRelease 3\n");
				EEPROM_GUDING_800V[num] = FirstPageData[num];
			}
			EEPROM_GUDING_800V[0x0C] = 0x01;
			EEPROM_GUDING_800V[0x0D] = 0x00;
			EEPROM_GUDING_800V[0x0E] = 0x3F;
			EEPROM_GUDING_800V[0x0F] = 0x3F;

			IIC_Write_ALL_EEPROM(SPT51_Addr,EEPROM_GUDING_800V,0x80,64);
			delay_ms(10);
			IIC_Read_EEPROM(SPT51_Addr,ReadEEPROMData,0x80,64);
			SPT51_Reboot(SPT51_Addr);
			for(num=0;num<64;num++)
			{
				if(EEPROM_GUDING_800V[num] != ReadEEPROMData[num])
				{
					Errornum ++;
					//printf("num = %d\n",num);
					//printf("Errornum = %d\n",Errornum);
					break;
				}
				else
				{
					continue;
				}
			}
			if(Errornum > 0)
			{

				SPT51_Reboot(SPT51_Addr);
				return 1;
			}
			else
			{
				SPT51_Reboot(SPT51_Addr);
				return 2;
			}
		}
		SPT51_Reboot(SPT51_Addr);
		return 1;
	}
	return 1;
}



uint8_t EEPROMRelease_816v(void)//-----------return 1(Error);return 0(success)
{
	printf("EEPROMRelease2\n");
	uint8_t num = 0;
	uint8_t cnt = 0;
	uint8_t Errornum = 0;
	uint8_t FirstPageData[16] = {0x00,0x00};
	uint8_t ReadEEPROMData[64] = {0x00,0x00};
	uint8_t SPT51_Addr = 0x60;
	uint8_t SPT51_Key[2] = {0x01,0x00};
	SPT51_SDA_Reset();
	iic_Init();

	//printf("EEPROMRelease 1\n");
	for(cnt = 0;cnt<3;cnt++)
	{

		if(SPT51_GetAccess(SPT51_Addr,SPT51_Key) == 0){
			break;
		}else{
			SPT51_SDA_Reset();
		}

	}


	if(SPT51_GetAccess(SPT51_Addr,SPT51_Key) == 0)
	{

		//printf("EEPROMRelease 2\n");
		SPT51_Always_Upgrade(SPT51_Addr);
		//printf("EEPROMRelease 2.5\n");
		IIC_Read_EEPROM(SPT51_Addr,FirstPageData,0x80,16);

		//printf("EEPROMRelease 2.6\n");
		if(FirstPageData[0] == 0x56)
		{
			//printf("EEPROMRelease 2.7\n");
			for(num = 0;num<16;num++)
			{

				//printf("EEPROMRelease 3\n");
				EEPROM_GUDING_816V[num] = FirstPageData[num];
			}
			EEPROM_GUDING_816V[0x0C] = 0x01;
			EEPROM_GUDING_816V[0x0D] = 0x00;
			EEPROM_GUDING_816V[0x0E] = 0x3F;
			EEPROM_GUDING_816V[0x0F] = 0x3F;

			IIC_Write_ALL_EEPROM(SPT51_Addr,EEPROM_GUDING_816V,0x80,64);
			delay_ms(10);
			IIC_Read_EEPROM(SPT51_Addr,ReadEEPROMData,0x80,64);
			SPT51_Reboot(SPT51_Addr);
			for(num=0;num<64;num++)
			{
				if(EEPROM_GUDING_816V[num] != ReadEEPROMData[num])
				{
					Errornum ++;
					//printf("num = %d\n",num);
					//printf("Errornum = %d\n",Errornum);
					break;
				}
				else
				{
					continue;
				}
			}
			if(Errornum > 0)
			{

				SPT51_Reboot(SPT51_Addr);
				return 1;
			}
			else
			{
				SPT51_Reboot(SPT51_Addr);
				return 2;
			}
		}
		SPT51_Reboot(SPT51_Addr);
		return 1;
	}
	return 1;
}


uint8_t EEPROMRelease_v15(void)//-----------return 1(Error);return 0(success)
{
	printf("EEPROMRelease2\n");
	uint8_t num = 0;
	uint8_t cnt = 0;
	uint8_t Errornum = 0;
	uint8_t FirstPageData[16] = {0x00,0x00};
	uint8_t ReadEEPROMData[64] = {0x00,0x00};
	uint8_t SPT51_Addr = 0x60;
	uint8_t SPT51_Key[2] = {0x01,0x00};
	SPT51_SDA_Reset();
	iic_Init();

	//printf("EEPROMRelease 1\n");
	for(cnt = 0;cnt<3;cnt++)
	{

		if(SPT51_GetAccess(SPT51_Addr,SPT51_Key) == 0){
			break;
		}else{
			SPT51_SDA_Reset();
		}

	}


	if(SPT51_GetAccess(SPT51_Addr,SPT51_Key) == 0)
	{

		//printf("EEPROMRelease 2\n");
		SPT51_Always_Upgrade(SPT51_Addr);
		//printf("EEPROMRelease 2.5\n");
		IIC_Read_EEPROM(SPT51_Addr,FirstPageData,0x80,16);

		//printf("EEPROMRelease 2.6\n");
		if(FirstPageData[0] == 0x56)
		{
			//printf("EEPROMRelease 2.7\n");
			for(num = 0;num<16;num++)
			{

				//printf("EEPROMRelease 3\n");
				EEPROM_GUDING_V15[num] = FirstPageData[num];
			}
			EEPROM_GUDING_V15[0x0C] = 0x01;
			EEPROM_GUDING_V15[0x0D] = 0x00;
			EEPROM_GUDING_V15[0x0E] = 0x3F;
			EEPROM_GUDING_V15[0x0F] = 0x3F;

			IIC_Write_ALL_EEPROM(SPT51_Addr,EEPROM_GUDING_V15,0x80,64);
			delay_ms(10);
			IIC_Read_EEPROM(SPT51_Addr,ReadEEPROMData,0x80,64);
			SPT51_Reboot(SPT51_Addr);
			for(num=0;num<64;num++)
			{
				if(EEPROM_GUDING_V15[num] != ReadEEPROMData[num])
				{
					Errornum ++;
					//printf("num = %d\n",num);
					//printf("Errornum = %d\n",Errornum);
					break;
				}
				else
				{
					continue;
				}
			}
			if(Errornum > 0)
			{

				SPT51_Reboot(SPT51_Addr);
				return 1;
			}
			else
			{
				SPT51_Reboot(SPT51_Addr);
				return 2;
			}
		}
		SPT51_Reboot(SPT51_Addr);
		return 1;
	}
	return 1;
}


#endif

