#include "include.h"

#if CHARGE_EN

#define TRACE_EN                0

#if TRACE_EN
#define TRACE(...)              printf(__VA_ARGS__)
#else
#define TRACE(...)
#endif

#define DC_IN()                  ((RTCCON >> 20) & 0x01)            //VUSB Online state:    0->not online, 1->online

typedef struct {
    u8 chag_sta   : 3;          //充电状态
    u8 dcin_reset : 1;          //DC插入复位
    u8 const_curr : 4,          //恒流充电电流
       trick_curr : 4;          //涓流充电电流

    u8 stop_curr : 3,           //充电截止电流
       stop_volt : 1;           //充电截止电压
    u8 trick_curr_en    : 1;    //是否使能涓流充电
    u8 trick_stop_volt  : 1;    //涓流充电截止电压
    u8 bled_on_pr;
    volatile u16 stop_time;     //仅电压充满的超时时间
} charge_cfg_t;
static charge_cfg_t charge_cfg;

typedef struct {
    u8  out2pwr_en;              //拿起开机
    u8  leakage_sta;
    u8  dc_in;
volatile u8  inbox_rtt_check;
    u32 counter;
} ch_box_cb_t;
ch_box_cb_t chbox_cb;

inbox_cb_t inbox_cb;
extern u32 user_timer_isr_en;

void charge_stop(u8 mode);
void charge_init(charge_cfg_t *p);
void charge_set_vddio(u8 en);
void unlock_code_charge(void);
//充满电亮蓝灯时间table表
#if CHARGE_FULL_LED_EN
AT(.rodata.charge)
const u8 ch_bled_timeout_tbl[8] = {0, 2, 20, 30, 60, 120, 180, 255};
#else
AT(.rodata.charge)
const u8 ch_bled_timeout_tbl[8] = {0, 10, 20, 30, 60, 120, 180, 255};
#endif

//充满电亮蓝灯时间控制
AT(.com_text.charge)
void bsp_charge_bled_ctrl(void)
{
    if (sys_cb.charge_bled_flag) {
        if (sys_cb.ch_bled_cnt) {
            if (sys_cb.ch_bled_cnt != 255) {
                sys_cb.ch_bled_cnt--;
            }
        } else {
            sys_cb.charge_bled_flag = 0;
#if CHARGE_FULL_LED_EN
			printf("charge_finish  close rled \n");
			bled_set_off(); 			//充电时，控制红灯灭
			//rled_set_off();
			LED_PWR_SET_OFF();
#else
			bled_set_off();
#endif

        }
    }
}

///设置leakage状态， 返回值 0：disable, 1: enable
AT(.com_text.bsp.charge)
void bsp_charge_leakage_set(u8 sta, u8 val)
{
    u32 rtccon8 = RTCCON8;
    u32 leakage = (rtccon8 >> 16) & 0x07;
    u32 level = (val) ? val : xcfg_cb.ch_leakage_sel;

    chbox_cb.leakage_sta = sta & 0xfc;
    sta &= 0x03;
    rtccon8 = rtccon8 & ~(0x07 << 16);
    if (sta) {
        if (leakage != level) {
            RTCCON8 = rtccon8 | (level << 16);
        }
    } else {
        if (leakage != 0) {
            RTCCON8 = rtccon8;
        }
    }
}

///使能出仓唤醒功能
AT(.text.bsp.charge)
void bsp_charge_inbox_wakeup_enable(void)
{
    RTCCON3 |= BIT(12);                                                         //RTCCON3[12], INBOX Wakeup enable
    if ((xcfg_cb.ch_box_type_sel == 2) || (xcfg_cb.ch_box_type_sel == 0)) {     //有维持电压的充电仓关闭leakage漏电
        bsp_charge_leakage_set(0, 0);                                           //leakage disable
    }
}

///检测INBOX状态   返回值  0: 正在检测, 1：检测完成,INBOX不在线， 2：检测完成,INBOX在线
AT(.com_text.bsp.charge)
u8 bsp_charge_inbox_check(void)
{
    inbox_cb_t *s = &inbox_cb;
    if (CHARGE_INBOX()) {
        s->off_cnt = 0;
        if (s->on_cnt < s->on_delay) {              //等待VUSB放电，确认INBOX是否在线
            s->on_cnt++;
            return 0;
        }
        if (s->on_cnt == s->on_delay) {
            s->on_cnt = s->on_delay + 5;
            return 2;                               //inbox在线
        }
        return 0x82;
    } else {
        s->on_cnt = 0;
        if (s->off_cnt < s->off_delay) {
            s->off_cnt++;
            if ((s->off_cnt == 4) && (chbox_cb.leakage_sta & BIT(7))) {
                bsp_charge_leakage_set(0, 0);
            }
            return 0;
        }
        if (s->off_cnt == s->off_delay) {
            s->off_cnt = s->off_delay + 5;
            return 1;                               //inbox不在线
        }
        return 0x81;
    }
}

///用于兼容短暂掉0V充电仓（充满后 5V -> 0V -> 2.xV）
AT(.text.bsp.charge)
int bsp_charge_outbox_stable_check(void)
{
    int i;
    u8 inbox_sta = 0;

    if (xcfg_cb.ch_box_type_sel) {
        return 1;                                                           //非此类型仓, 直接返回出仓开机
    }
    bsp_charge_leakage_set(0, 0);                                           //disable leakage
    delay_5ms(60);                                                          //delay 300ms
    bsp_charge_inbox_chk_init(12+(u16)xcfg_cb.chbox_out_delay*10, 20);
    for (i = 0; i < 200; i++) {
        delay_5ms(2);
        WDT_CLR();
        inbox_sta = bsp_charge_inbox_check();                               //检测INBOX状态判断是否为充满状态
        if (inbox_sta) {
            break;
        }
    }

    if (inbox_sta == 2) {                                                   //inbox stable online
        if (DC_IN()) {
            bsp_charge_leakage_set(1, 0);                                   //enable leakage
            return 0;                                                       //5V online, 继续充电
        }
        bsp_charge_inbox_wakeup_enable();                                   //INBOX Wakeup enable
        saradc_exit(adc_cb.channel);
        sfunc_pwrdown(1);                                                   //开VUSB复位，且并机
    }
    bsp_charge_leakage_set(1, 0);                                           //enable leakage
    return 1;                                                               //出仓开机
}

AT(.text.bsp.charge)
void bsp_charge_inbox_chk_init(u16 off_delay, u16 on_delay)
{
    inbox_cb.off_cnt = 0;
    inbox_cb.on_cnt = 0;
    inbox_cb.off_delay = off_delay;
    inbox_cb.on_delay = on_delay;
}

///兼容某些维持电压的仓: 耳机入仓, 电池仓的电压不会自动升到5V, 需要手动按仓的按键。
AT(.com_text.bsp.charge)
void bsp_charge_inbox_process(void)
{
    static u32 inbox_tick = 0;
    u8 sta;
    if ((chbox_cb.inbox_rtt_check) && (!CHARGE_DC_IN())) {
        if (!tick_check_expire(inbox_tick, 10)) {
            return;
        }
        inbox_tick = tick_get();
        sta = bsp_charge_inbox_check();
#if VUSB_SMART_VBAT_HOUSE_EN
        bsp_vhouse_inbox_sta(sta);
#endif
        if ((2 == sta) && (xcfg_cb.chg_inbox_pwrdwn_en)) {
            msg_enqueue(EVT_CHARGE_INBOX);
        }
    }
}

AT(.text.bsp.charge)
u8 get_inbox_auto_pwroff(void)
{
    if (xcfg_cb.chg_inbox_pwrdwn_en) {
        if (sys_cb.pwroff_delay != 0) {
            return sys_cb.inbox_pwrdwn_flag;
        }
    }
    return 0;
}

///耳机在充电仓充满自动关机，拿起自动开机处理。
AT(.text.bsp.charge)
u8 bsp_charge_box_sta(void)
{
    if ((sys_cb.charge_sta == 2) && (!sys_cb.charge_bled_flag)) {
        //充满且蓝灯灭时进入关机
        if (xcfg_cb.ch_full_auto_pwrdwn_en) {
            //printf("charge full auto pwrdwn\n");
            bsp_charge_inbox_wakeup_enable();
            return 2;                                                   //充满关机
        }
    }
    return 0;
}

///降低充电时主控耗电
u32 clkcon0_tmp = 0;
AT(.text.bsp.charge)
void bsp_charge_power_save(void)
{
#if CHARGE_LOW_POWER_EN
    set_sys_clk(SYS_2M);                       //跑XOSC 26M
#if VUSB_SMART_VBAT_HOUSE_EN
    set_uart1_baud_rc2m_clk(9600);
#endif
    GLOBAL_INT_DISABLE();
    RTCCON0 &= ~BIT(8);                     //CLK2M Select RTC 2M
    clkcon0_tmp = CLKCON0;
    CLKCON0 |= BIT(0);                      //enable RC
    CLKCON0 = (CLKCON0 & ~(0x3 << 2));      //system clk select RC
    PLL1CON &= ~BIT(6);
    PLL1CON &= ~BIT(18);
    XO26MCON &= ~(0xf<<24);                 //X26 output DIS
    XO26MCON &= ~BIT(10);                   //X26 DIS
    XO26MCON &= ~BIT(9);                    //X26 LDO DIS
    CLKCON0 &= ~BIT(28);
    CLKCON0 &= ~(7 << 23);                 //timer0 clk
    CLKCON0 |= BIT(24) | BIT(23);          //timer0 CLK Select RC
    GLOBAL_INT_RESTORE();
#else
    set_sys_clk(SYS_24M);
    dac_clk_source_sel(2);                      //dac clk select xosc26m_clk
#endif

    if (PLL0CON1 & BIT(15)) {
        PLL0CON &= ~BIT(18);                //pll0 sdm disable
        PLL0CON &= ~BIT(6);                 //pll0 analog diable
        PLL0CON &= ~BIT(12);                //pll0 ldo disable
    }
}

AT(.text.bsp.charge)
void bsp_charge_power_recover(void)
{
#if CHARGE_LOW_POWER_EN
    GLOBAL_INT_DISABLE();
    XO26MCON |= (0xf<<24);                 //X26 output DIS
    XO26MCON |= BIT(10);                   //X26 DIS
    XO26MCON |= BIT(9);                    //X26 LDO DIS
    delay_us(100);
    RTCCON0 |= BIT(8);
    CLKCON0 = clkcon0_tmp;
    PLL1CON |= BIT(6);
    PLL1CON |= BIT(18);
    GLOBAL_INT_RESTORE();
#endif
    if (PLL0CON1 & BIT(15)) {
        PLL0CON |= BIT(12);                 //PLL0 LDO enable
        delay_us(100);
        PLL0CON |= BIT(18);                 //PLL0 SDM enable
        PLL0CON |= BIT(6);                  //PLL0 Analog enable
        delay_us(100);
    }
    set_sys_clk(SYS_CLK_SEL);
#if !CHARGE_LOW_POWER_EN
    dac_clk_source_sel(1);                      //dac clk select adpll_div(PLL0)
#else
#if VUSB_SMART_VBAT_HOUSE_EN
    set_uart1_baud(9600);
#endif
#endif
}

#if TRACE_EN
AT(.com_rodata.tkey)
const char charge_on_str[] = "Charge on\n";
AT(.com_rodata.tkey)
const char charge_off_str[] = "Charge off: %d\n";
#endif

#if USER_NTC
AT(.charge_text.det)
u16 get_ntc_adc_ch_val(void)
{
   return sys_cb.ntc_val;
}
AT(.charge_text.det)
u8 ntc_check_charge_sta(void)
{
   static u8 off_cnt = 0,on_cnt = 0,ntc_sta = false;
   u16 ntc_temper_ad = 0;
   static u32 ntc_ticks = 0;
   if(tick_check_expire(ntc_ticks, 1000)){
       ntc_ticks = tick_get();									//作用？
       ntc_temper_ad = get_ntc_adc_ch_val();					//获得NTC功能的  ADC通道的值
       printf("ntc_temper_ad : %d\n",ntc_temper_ad);
        if((ntc_temper_ad <= NTC_LOW_TEMP_AD)||(ntc_temper_ad >= NTC_HIGH_TEMP_AD)){
           off_cnt++;
           on_cnt = 0;
           if(off_cnt > NTC_CHECK_TIME){
              off_cnt = 0;
              ntc_sta = true;
           }
       }else{
             off_cnt = 0;
             if(ntc_temper_ad > (NTC_LOW_TEMP_AD + NTC_AD_VALUE_FILTER) && ntc_temper_ad < (NTC_HIGH_TEMP_AD - NTC_AD_VALUE_FILTER)){
                 on_cnt++;
                 if(on_cnt > NTC_CHECK_TIME){
                    ntc_sta = false;
                 }
             }
       }
   }
   return ntc_sta;
}
AT(.charge_text.det)
void ntc_contrl_charge_sta(void)
{
    static u8 display_sta = 0,counter = 0;
    if(display_sta == ntc_check_charge_sta()){
       counter = 0;
    }else{
       counter++;
       if(counter > 10){						//50ms调用一次
          counter = 0;
          display_sta = ntc_check_charge_sta();

          if(display_sta){
            printf("stop_chargeing\n");
            bsp_charge_off();
            //RTCCON8 &=~ BIT(6);					//第7位置零（左移第几位）
            //RTCCON8 |= BIT(1);					//第2位置一
            RTCCON8 &=~ BIT(6);					//第7位置零（左移第几位）
            //RTCCON8 |= BIT(1);					//第2位置一
            //RTCCON8 &= ~BIT(0);					//第2位置一
            printf("BIT(6) %d BIT(1) %d\n", (RTCCON8 & BIT(6)) , (RTCCON8 & BIT(1)));
          }else{
            printf("start_chargeing\n");		//恢复充电
            if (!(RTCCON8 & BIT(6))){
				RTCCON8 |= BIT(6);
			}
            //RTCCON8 &=~ BIT(1);
			printf("BIT(6) %d BIT(1) %d\n", (RTCCON8 & BIT(6)) , (RTCCON8 & BIT(1)));
          }

       }
    }
}
AT(.com_text.port.key)
u16 get_channel_adc (u16 *ptr)		//中位值平均滤波法,(消除脉冲影响)比较浪费ram
{
    u16 max = ptr[0], min = ptr[0], all = 0;

    for (u8 i = 0; i < 10; i++) {
        if (ptr[i] > max) {
            max = ptr[i];
        }
        if (ptr[i] < min) {
            min = ptr[i];
        }
        all += ptr[i];
    }

    return (all - max - min) >> 3;	//去掉一个最高值，去掉一个最低值再除以8（10-2）,求均值
    								//利用了位操作，优化了求均值时间
}
AT(.com_text.port.key)
void get_ntc_adc_val(u16 val)							//记录10个ADC的值，求他们的均值
{
    static u8 adc_cov_cnt = 0;
    static u16 ntc_ch[10] = {0};

    if (adc_cov_cnt < 10) {
        ntc_ch[adc_cov_cnt] = val;						//一个10个数值的数组
        adc_cov_cnt++;
    }
    if (adc_cov_cnt >= 10) {
        sys_cb.ntc_val = (u16)get_channel_adc(ntc_ch);	//根据这个10位数组去掉最高最低求均值，放到ntc_val中
        adc_cov_cnt = 0;
    }
}
#endif // USER_NTC




AT(.charge_text.det)
void bsp_charge_sta(u8 sta)
{
	static u8 pwr_id = 0;
    if (sta == 1) {
        //充电开启
        sys_cb.charge_sta = 1;
        sys_cb.charge_bled_flag = 0;
#if CHARGE_FULL_LED_EN
	pwr_id = param_inbox_read();
	//printf("pwr_id  =======================%d\n",pwr_id);
	if(sys_cb.from_box_power_on == 0)
	{
		if(pwr_id)		//仓外关机
		{
			if(sys_cb.charge_led_cnt == 0)
			{
				//printf("outbox pwroff need led on  1111111111111111111111\n");
				charge_bled_on();				//充电时，亮红灯
			}
			sys_cb.charge_led_cnt++;
			if(sys_cb.charge_led_cnt > 200)
			{
				sys_cb.charge_led_cnt = 100;
			}
		}
		else
		{
			LED_SET_OFF();
		}
	}
	else
	{		//仓内关机充电以及关盖充电灯灭
			//printf("inbox pwroff led off off off 222222222222222222\n");
			LED_SET_OFF();
	}
#else
			charge_led_on();
#endif

        TRACE(charge_on_str);
    } else {
        //充电关闭
        TRACE(charge_off_str, sta);
        sys_cb.charge_sta = 0;
        sys_cb.charge_bled_flag = 0;
        charge_led_off();
        if (sta == 2) {
            sys_cb.charge_sta = 2;
            //充电满亮蓝灯
            if (BLED_CHARGE_FULL) {
                sys_cb.charge_bled_flag = 1;
                sys_cb.ch_bled_cnt = charge_cfg.bled_on_pr;
#if CHARGE_FULL_LED_EN
				//if(!sys_cb.charge_open_window_flag)
				printf("charge_ led on on on \n");
				charge_led_on();				//充电时，亮红灯
#else
				charge_bled_on();
#endif
            }
        }
    }
}
AT(.text.bsp.charge)
void bsp_charge_off(void)
{
    if (charge_cfg.chag_sta >= 3) {
        charge_stop(0);
    }
}
AT(.text.bsp.charge)
void bsp_set_stop_time(u16 stop_time)
{
    charge_cfg.stop_time = stop_time;
}

void bsp_charge_dis(void)
{
    RTCCON8 |= BIT(1);
    RTCCON8 &= ~BIT(6);

    RTCCON |= BIT(6);
    RTCCON11 |= BIT(6);
}

void bsp_charge_init(void)
{
    charge_cfg_t *p = &charge_cfg;
    memset(p, 0, sizeof(charge_cfg_t));

    p->trick_curr_en = CHARGE_TRICK_EN;
    p->const_curr = CHARGE_CONSTANT_CURR & 0x0f;
    p->trick_curr = CHARGE_TRICKLE_CURR & 0x0f;
    p->stop_curr = CHARGE_STOP_CURR & 0x07;
    p->stop_volt = CHARGE_STOP_VOLT & 0x03;
    p->dcin_reset = CHARGE_DC_RESET & 0x01;
    p->trick_stop_volt = CHARGE_TRICK_STOP_VOLT;
    p->stop_time = 18000;               //默认30分钟
    p->bled_on_pr = ch_bled_timeout_tbl[BLED_CHARGE_FULL];

    RTCCON7 = (RTCCON7 & ~(0x0f << 4)) | 0x20;          //PWRUPICH = 30mA
    if (RTCCON3 & BIT(12)) {
        sys_cb.inbox_wken_flag = 1;                     //修正触摸唤醒拿出不能开机问题
    }
    RTCCON3 &= ~BIT(12);                                //RTCCON3[12], INBOX Wakeup disable
    if (xcfg_cb.ch_inbox_sel) {
        RTCCON8 |= BIT(4);                              //inbox voltage selection.  1'b1,  1.7v;   1'b0, 1.1v
    } else {
        RTCCON8 &= ~BIT(4);
    }
    RTCCON8 = (RTCCON8 & ~(0x07 << 16)) | ((u32)xcfg_cb.ch_leakage_sel << 16); //leakage configure
    charge_init(p);

#if VUSB_SMART_VBAT_HOUSE_EN
    bsp_smart_vhouse_init();
#endif
#if VUSB_TBOX_QTEST_EN
    bsp_qtest_init();
#endif
	sys_cb.charge_led_cnt = 0;
}

///5ms调用一次
AT(.text.bsp.charge)
static u32 bsp_charge_box_process_do(void)
{
    u8 chbox_sta, vhouse_sta = 0;
#if BOXIN_PWRON_BLEM
	//static u8 dcout_count = 0;
#endif
    chbox_cb.counter++;
    if ((chbox_cb.counter % 20) == 0) {
        charge_detect(0);
    }
    if ((chbox_cb.counter % 180) == 0) {
        if ((user_timer_isr_en & BIT(1)) == 0) {
            bsp_charge_bled_ctrl();
        }
    }
#if VUSB_SMART_VBAT_HOUSE_EN
    vhouse_sta = bsp_smart_vhouse_process(1);
#endif
    if ((bsp_charge_box_sta() == 2) || (vhouse_sta == 2)) {             //是否充满

        //bsp_charge_box_exit();
        unlock_code_charge();
        bsp_charge_inbox_wakeup_enable();                               //INBOX Wakeup enable
        saradc_exit(adc_cb.channel);
		if(func_cb.sta == FUNC_PWROFF)
		{
			func_pwroff(0);
		}
		else
		{
#if USB_CHARGE_NO_FULL		//修改充电仓插USB充电时耳机充不满的问题
			 static u32 tick = 0;
	 		 if (tick_check_expire(tick, 10000)) {
 					tick = tick_get();
 					if (++sys_cb.charge_time_out >= 240) {
 						WDT_RST();
 					}
	 		 }
#endif
			//sfunc_pwrdown(0);
			//charge_stop(0);
			//printf("charge finish \n");
			LED_SET_OFF();
			bsp_charge_off();
            RTCCON8 &=~ BIT(6);					//第7位置零（左移第几位）
            //RTCCON8 |= BIT(1);					//第2位置一
            RTCCON8 &= ~BIT(0);					//第2位置一
		}
    }
#if USER_NTC
		if(xcfg_cb.ntc_en){
			if(CHARGE_DC_IN()){
			   ntc_contrl_charge_sta();
			}
		}
#endif
    if (!CHARGE_DC_IN()) {                                              //DC IN online?
			//dcout_count++;
//        if ((chbox_cb.dc_in == 3) && CHARGE_INBOX()) {
//            bsp_charge_leakage_set(0x81, 6);                            //用大电流leakage加速VUSB掉电, 在检测到低于INBOX电压后关闭leakage(防止VUSB电容导致掉电慢)
//        }
//        if (chbox_cb.dc_in < 10) {
//            chbox_cb.dc_in++;
//        }
        bsp_charge_leakage_set(0, 0);
        chbox_sta = bsp_charge_inbox_check() & 0x3;                     //检查inbox online状态
#if VUSB_SMART_VBAT_HOUSE_EN
        if (vhouse_sta) {                                               //智能充电仓已开盖, 需要退出充电流程
            bsp_charge_leakage_set(1, 0);
            if (chbox_sta == 1) {                                       //dc_in = 0, 且inbox = 0
                sys_cb.outbox_pwron_flag = 1;
            }
#if BOXIN_PWRON_BLEM
			//if(dcout_count> 254){
            //	return 2;
			//}
			//printf("dconut === %d\n",dcout_count);
#else
			return 2;
#endif
        }
#endif
        if (chbox_sta == 1) {                                           //dc_in = 0, 且inbox = 0
            bsp_charge_leakage_set(1, 0);
            if ((chbox_cb.out2pwr_en) || (vhouse_sta)) {                //拿起开机
                sys_cb.outbox_pwron_flag = 1;
                return 2;
            }
            return 1;
		//	else if (chbox_sta == 2){
      	    } else if (chbox_sta == 2 && vhouse_cb.status != 0) {                                    //dc_in = 0, inbox = 1, 需要关机
            bsp_charge_box_exit();
            unlock_code_charge();
            bsp_charge_inbox_wakeup_enable();                           //INBOX Wakeup enable
            saradc_exit(adc_cb.channel);
            sfunc_pwrdown(1);
        }
    } else {
    	//dcout_count = 0;
        chbox_cb.dc_in = 1;
        inbox_cb.off_cnt = 0;
        inbox_cb.on_cnt = 0;
        bsp_charge_leakage_set(1, 0);
        if ((chbox_cb.counter % 1000) == 0) {                           //5秒
            bsp_charge_bcnt_calibration((VUSB_SMART_VBAT_HOUSE_EN) ? 100 : 500);
        }
    }
    return 0;
}

#if USER_INEAR_DETECT_EN
void power_lowper_15ms()
{
	printf("ear in rest gpio");

	//sys_cb.charge_open_window_flag = 1;
	GPIOBDE |= (BIT(1)|BIT(5));
	GPIOBCLR |= (BIT(1)|BIT(5));
	GPIOBDIR &=~(BIT(1)|BIT(5));
	delay_5ms(3);
	GPIOBDE |= (BIT(1)|BIT(5));
	GPIOBPU |= (BIT(1)|BIT(5));
	GPIOBDIR |=(BIT(1)|BIT(5));
}
#endif

AT(.text.bsp.charge)
u32 bsp_charge_box_process(void)
{
#if USER_INEAR_DETECT_EN
	if(CHARGE_DC_IN()){
		if(!sys_cb.ruer_reset_flag)
		{	
			sys_cb.ruer_reset_flag = 1;
			power_lowper_15ms();
		}
	}
#endif

    u32 charge_sta = bsp_charge_box_process_do();
    bsp_charge_tebcnt_calibration();
    return charge_sta;
}

AT(.text.bsp.charge)
void bsp_charge_box_enter(u32 out_auto_pwron)
{
    memset(&chbox_cb, 0, sizeof(chbox_cb));
    chbox_cb.out2pwr_en = out_auto_pwron;
    bsp_charge_inbox_chk_init(20, 120);                                 //inbox online检测长点，等待VUSB电容放电
    if ((chbox_cb.out2pwr_en) && (!xcfg_cb.ch_box_type_sel)) {          //短暂掉0V的仓
        inbox_cb.off_delay = 100+(u16)xcfg_cb.chbox_out_delay*20;       //inbox offline check 500ms + n*100ms
    }
	bsp_charge_power_save();
}

AT(.text.bsp.charge)
void bsp_charge_box_reinit(void)
{
    sys_cb.inbox_wken_flag = 0;
    bsp_charge_inbox_chk_init(10, 20);
    chbox_cb.leakage_sta = 0;
    if ((xcfg_cb.ch_box_type_sel == 2) || (xcfg_cb.ch_box_type_sel == 0)) {
        chbox_cb.inbox_rtt_check = 1;
    }
}

AT(.text.bsp.charge)
void bsp_charge_box_exit(void)
{
    bsp_charge_power_recover();
    bsp_charge_box_reinit();
    bsp_charge_tebcnt_calibration_stop();
}

#endif // CHARGE_EN
