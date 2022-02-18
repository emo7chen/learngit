#if PLXC_UP_EN
#include "include.h"


#define DebugSPT51 1


#define SCL 1
#define SDA 5


#define iic_SCL_H()                     GPIOBSET = BIT(SCL)
#define iic_SCL_L()                     GPIOBCLR = BIT(SCL)

#define iic_SDA_H()                     GPIOBSET = BIT(SDA)
#define iic_SDA_L()                     GPIOBCLR = BIT(SDA)


#define READ_SDA GPIOB & BIT(SDA)
enum SPT51
{
    noneMode = 0,
    iicMode = 1,
    uartMode = 2,
    keyMode = 3,
};

void iic_Init(void);
void spt51_key_Init(void);
void iic_SDA_IN(void);
void iic_SDA_OUT();
void IIC_Start(void);
void IIC_Stop(void);
uint8_t IIC_Wait_Ack(void);
void IIC_Ack(void);
void IIC_NAck(void);
void IIC_Send_Byte(uint8_t txd);
uint8_t IIC_Read_Byte(void);


uint8_t IIC_Read_Register(uint8_t DevAddr,uint8_t *ReadData,uint8_t RegAddr,uint8_t length);
uint8_t IIC_Write_Register(uint8_t DevAddr,uint8_t *WriteData,uint8_t RegAddr,uint8_t length);
uint8_t IIC_Read_EEPROM(uint8_t DevAddr,uint8_t *ReadData,uint8_t EepAddr,uint8_t length);
uint8_t IIC_Write_Page_EEPROM(uint8_t DevAddr,uint8_t *WriteData,uint8_t PageAddr,uint8_t start_index);
uint8_t IIC_Write_ALL_EEPROM(uint8_t DevAddr,uint8_t *WriteData,uint8_t EepAddr,uint8_t length);
uint8_t SPT51_Always_Upgrade(uint8_t DevAddr);
uint8_t SPT51_Reboot(uint8_t DevAddr);
void SPT51_Reset(void);
void SPT51_INTI(void);
uint8_t SPT51_GetAccess(uint8_t DevAddr,uint8_t *WriteData);
void SPT51_SDA_Reset(void);
void SPT51_SDA_Reset_Main(void);

void IIC_LOW(void);
void user_bt_app_cmd_process(u8 *ptr, u16 size);
void SPT51_IIC_GetCapData(uint8_t *Data);
uint8_t EEPROMRelease(void);
uint8_t EEPROMRelease2(void);
uint8_t EEPROMRelease3(void);
uint8_t EEPROMRelease_800v(void);//-----------return 1(Error);return 0(success)
uint8_t EEPROMRelease_816v(void);//-----------return 1(Error);return 0(success)
uint8_t EEPROMRelease_v15(void);//-----------return 1(Error);return 0(success)

#endif
