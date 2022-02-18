#include "include.h"

func_cb_t func_cb AT(.buf.func_cb);
void sco_wait_sync();

#if VBAT_DETECT_EN
AT(.text.func.msg)
void lowpower_vbat_process(void)
{
    int lpwr_vbat_sta = is_lowpower_vbat_warning();

    if (lpwr_vbat_sta == 1) {
#if LOWPWR_MP3_EN
	   // if (func_cb.mp3_res_play) {
		//func_cb.mp3_res_play(RES_BUF_LOW_BATTERY_MP3, RES_LEN_LOW_BATTERY_MP3);
	   // }
	   	wav_res_play(RES_BUF_LOW_BATTERY_WAV, RES_LEN_LOW_BATTERY_WAV);
		sys_cb.discon_reason = 0;	//同步关机
#endif
        //bsp_piano_warning_play(WARNING_TONE, TONE_LOW_BATTERY);
        sys_cb.pwrdwn_tone_en = 1;
        func_cb.sta = FUNC_PWROFF;     //低电，进入关机或省电模式
        return;
    }
    if ((func_cb.mp3_res_play == NULL) || (lpwr_vbat_sta != 2)) {
        if ((sys_cb.lowbat_flag) && (sys_cb.vbat > 3800)) {
            sys_cb.vbat_nor_cnt++;
            if (sys_cb.vbat_nor_cnt > 40) {
                sys_cb.lowbat_flag = 0;
                sys_cb.lpwr_warning_times = LPWR_WARING_TIMES;
                plugin_lowbat_vol_recover();    //离开低电, 恢复音乐音量
            }
        }
        return;
    }

    //低电提示音播放
    sys_cb.vbat_nor_cnt = 0;
#if LOWPWR_MP3_EN
    if (sys_cb.lpwr_warning_cnt > 300) {
        sys_cb.lpwr_warning_cnt = 0;
    }
    if (sys_cb.lpwr_warning_cnt == 10) {
        // if (sys_cb.lpwr_warning_cnt > xcfg_cb.lpwr_warning_period) {
        // sys_cb.lpwr_warning_cnt = 0;
#else
    if (sys_cb.lpwr_warning_cnt > xcfg_cb.lpwr_warning_period) {
        sys_cb.lpwr_warning_cnt = 0;
#endif
        if (sys_cb.lpwr_warning_times) {        //低电语音提示次数
            if (RLED_LOWBAT_FOLLOW_EN) {
                led_lowbat_follow_warning();
            }

            sys_cb.lowbat_flag = 1;
//            if (func_cb.mp3_res_play) {
//                func_cb.mp3_res_play(RES_BUF_LOW_BATTERY_MP3, RES_LEN_LOW_BATTERY_MP3);
//            }
		   wav_res_play(RES_BUF_LOW_BATTERY_WAV, RES_LEN_LOW_BATTERY_WAV);

           // bsp_piano_warning_play(WARNING_TONE, TONE_LOW_BATTERY);
            plugin_lowbat_vol_reduce();         //低电降低音乐音量

			#if 0
            if (RLED_LOWBAT_FOLLOW_EN) {
                while (get_led_sta(1)) {        //等待红灯闪完
                    delay_5ms(2);
                }
                led_lowbat_recover();
            }
			#endif

            if (sys_cb.lpwr_warning_times != 0xff) {
                sys_cb.lpwr_warning_times--;
            }
        }
    }
}
#endif // VBAT_DETECT_EN
void pwrkey10s_counter_clr(void);
AT(.text.func.process)
void func_process(void)
{
    WDT_CLR();

    pwrkey10s_counter_clr(); //增加死机复位处理

#if NO_CONNECT_MP3_EN
    {
        static u32 tick = 0;
        if (tick_check_expire(tick, 100)) {
            tick = tick_get();
            if (sys_cb.delay_connect_flag > 1 && !bt_tws_is_slave()) {
                if ((--sys_cb.delay_connect_flag) == 1) {
                    printf("func_process               connected --------------------2\n");
                    f_bt.warning_status |= BT_WARN_CON;
                }
            }
        }
    }
#endif

#if VBAT_DETECT_EN
    lowpower_vbat_process();
#endif // VBAT_DETECT_EN

#if VUSB_TBOX_QTEST_EN
    if(qtest_get_mode()){
        qtest_process();
    }
    qtest_other_usage_process();
#endif

#if BT_BACKSTAGE_EN
    if (func_cb.sta != FUNC_BT) {
        func_bt_warning();
        uint status = bt_get_status();
#if BT_BACKSTAGE_PLAY_DETECT_EN
        if (status >= BT_STA_PLAYING) {
#else
        if (status > BT_STA_PLAYING) {
#endif
            func_cb.sta_break = func_cb.sta;
            func_cb.sta = FUNC_BT;
        }
    }
#endif
    if (sys_cb.pwroff_autoset_flag != AUTOSET_IDLE) {
        if (sys_cb.pwroff_autoset_flag == AUTOSET_EN) {
            en_auto_pwroff();
            sys_cb.sleep_en = BT_PAIR_SLEEP_EN;
        } else if (sys_cb.pwroff_autoset_flag == AUTOSET_OFF) {
            dis_auto_pwroff();
            sys_cb.sleep_en = 1;
        }
        sys_cb.pwroff_autoset_flag = AUTOSET_IDLE;
    }
    //PWRKEY模拟硬开关关机处理
    if ((PWRKEY_2_HW_PWRON) && (sys_cb.pwrdwn_hw_flag)) {
        sys_cb.pwrdwn_tone_en = 1;
        func_cb.sta = FUNC_PWROFF;
        sys_cb.pwrdwn_hw_flag = 0;
    }

#if 0//USER_NTC
    if(sys_cb.ntc_2_pwrdwn_flag){
        sys_cb.pwrdwn_tone_en = 1;
        func_cb.sta = FUNC_PWROFF;
        //printf("hello WIKI\n");
        sys_cb.ntc_2_pwrdwn_flag = 0;
    }
#endif

#if CHARGE_EN
    if (xcfg_cb.charge_en) {
        charge_detect(1);
        bsp_charge_inbox_process();
    }
#endif // CHARGE_EN

#if SYS_KARAOK_EN
    karaok_process();
#endif

#if ANC_FADE_IN_EN
    anc_vol_fade_process();
#endif 

#if LE_APP_EN
    bsp_ble_process();
#endif
#if FOT_EN
    bsp_fot_process();
#endif

    bsp_tws_sync_process();

    gsensor_process();
}
uint8_t EEPROMRelease2(void);
uint8_t EEPROMRelease(void);
uint8_t EEPROMRelease3(void);
uint8_t EEPROMRelease_800v(void); //-----------return 1(Error);return 0(success)
uint8_t EEPROMRelease_816v(void); //-----------return 1(Error);return 0(success)
uint8_t EEPROMRelease_v15(void);
void spt51_key_Init(void);

//func common message process
AT(.text.func.msg)
void func_message(u16 msg)
{
#if USER_INEAR_DETECT_EN
	u8 status = bt_get_disp_status();   //获取当前状态
#endif
    switch (msg) {
#if USER_INEAR_DETECT_EN
    case EVT_SPT51_11_GUDING:
        if (sys_cb.i2c_flag != 2) {
            sys_cb.i2c_flag = EEPROMRelease2();
        }
        spt51_key_Init();
        break;
    case EVT_SPT51_12_GUDING:
        if (sys_cb.i2c_flag != 2) {
            sys_cb.i2c_flag = EEPROMRelease3();
        }
        spt51_key_Init();
        break;

        case EVT_SPT51:

        if (sys_cb.i2c_flag != 2) {
            sys_cb.i2c_flag = EEPROMRelease();
        }
        spt51_key_Init();
        break;
    case EVT_SPT51_800_GUDING:
        if (sys_cb.i2c_flag != 2) {
            sys_cb.i2c_flag = EEPROMRelease_800v();
        }

        spt51_key_Init();
        break;
    case EVT_SPT51_816_GUDING:
        if (sys_cb.i2c_flag != 2) {
            sys_cb.i2c_flag = EEPROMRelease_816v();
        }

        spt51_key_Init();
        break;
    case EVT_SPT51_V15_GUDING:
        if (sys_cb.i2c_flag != 2) {
            sys_cb.i2c_flag = EEPROMRelease_v15();
        }

        spt51_key_Init();
        break;

#endif


			case EVT_PRIVATE_CALL:
#if FACTROY_TEST_EN
        if (sys_cb.ruer_cmd_open) {
            break;
        }
#endif
        if (sco_is_connected() && sys_cb.rem_ear_sta) {
			printf("bt_call_private_switch  1\n");
            bt_call_private_switch();
        }
        break;


			case EVT_SHIPMODE:
				printf("EVT_SHIPMODE  \n");
#if GT_TEST_EN
        if (sys_cb.spp_ship_mode_flag) {
            GPIOBDIR &= ~BIT(4); //关闭蓝灯
            sys_cb.pwr_led_on_ctl = 1;
            delay_5ms(2);
            LED_SET_OFF();
            LED_PWR_SET_OFF();
            delay_5ms(2);
            LED_PWR_SET_ON();
            delay_5ms(60);
            LED_PWR_SET_OFF();
            delay_5ms(60);
            LED_PWR_SET_ON();
            delay_5ms(60);
            LED_PWR_SET_OFF();
            delay_5ms(60);
        }
#endif
#if CHUANYUN_MODE
				APM_PA5_SET_H();
				delay_5ms(80);

				APM_PA5_SET_L();
				printf("APM_PE5_SET_H \n");
#endif
				break;


#if WL_SMART_HOUSE_EN
			case EVT_CLEAR_INFO:
					printf("EVT_CLEAR_INFO \n");
					func_bt_exit();
					//f_bt.disp_status = 0xff;
					//sys_cb.clear_info_flag = 1;
					if(!bt_tws_is_slave())
					{
						printf("********************************************master\n");
					}
					GPIOBDIR &=~ BIT(4);  //关闭蓝灯
					sys_cb.pwr_led_on_ctl = 1;		//关闭指示灯扫描
					bt_clr_all_link_info(); 						//删除所有配对信息
					delay_5ms(2);
					LED_SET_OFF();
					LED_PWR_SET_OFF();
					delay_5ms(2);
					LED_PWR_SET_ON();
					delay_5ms(60);
					LED_PWR_SET_OFF();
					delay_5ms(60);
					LED_PWR_SET_ON();
					delay_5ms(60);
					LED_PWR_SET_OFF();
					delay_5ms(60);
					LED_PWR_SET_ON();
					delay_5ms(60);
					LED_PWR_SET_OFF();
					delay_5ms(100);
					//f_bt.disp_status = 0xff;
					//led_clear_info();
					//func_cb.sta =FUNC_NULL;
#if CELAR_INFO_ID_EN
					sys_cb.clear_info_id = 1;
					param_clearinfo_write(1);
#endif
					WDT_RST();
					//msg_enqueue(EVT_TWS_PAIR);
				break;

				case EVT_ENTER_PAIR_MODE:
						sys_cb.into_pair_flag = 1; //长按3S仓，进入配对的话，需要把红灯常亮刷新成配对灯
						 if( (bt_tws_is_connected() && !bt_tws_is_slave()) || (!bt_tws_is_connected()))
						{
							if(bt_nor_is_connected())
							{
								bt_nor_disconnect();
							}
							else
							{
								if(f_bt.disp_status ==5)
								{
									sys_cb.is_need_pairing = 0;
									bt_set_scan(0x02);
									bt_abort_reconnect();
									f_bt.warning_status |= BT_WARN_PAIRING;
								}
								//f_bt.disp_status =0xff;
								msg_enqueue(EVT_BT_UPDATE_STA);
							}
						}
						 break;
#endif
#if SCO_WAIT_ON_EN
                case EVT_SYNC_SCO_WAIT:
                    sco_wait_sync();
 
                break;

                case EVT_SYNC_SCO_ON:
					bsp_sys_unmute();
					sys_cb.sco_wait_timeout = 0;
                    break;
#endif
#if TWS_CONN_FIRST_PAIR
				case PAIRIG_MP3:
					f_bt.warning_status |= BT_WARN_PAIRING;
					break;
#endif
#if CHARGE_INBOX_CHECK
				case EVT_FADE_IN:
					delay_5ms(20);
					bsp_sys_mute();
					break;
				case EVT_FADE_OUT:
					delay_5ms(20);
					bsp_sys_unmute();

					f_bt.disp_status = 0xff;	//刷新显示
					if(bt_tws_is_slave())		//从机出仓，需要告诉主机，让主机刷新状态
					{
						printf("----------\n");
						bt_tws_user_key(TWS_USER_FLUSH_STA);
					}
					break;
				case EVT_ENTER_SLEEP_MODE:				//入仓超时进入睡眠状态
					if(xcfg_cb.bt_tswi_kpwr_en) {		//按键关机是否主从切换
						if(sys_cb.discon_reason == 0xff) {
							sys_cb.discon_reason = 0;	//不同步关机
						}
					}
					sys_cb.pwrdwn_tone_en = 1;
					func_cb.sta = FUNC_PWROFF;


					break;

#endif
#if WL_SMART_HOUSE_EN
				case EVT_PWR_OFF:		//通讯发码关机
					if(xcfg_cb.bt_tswi_kpwr_en) {       //按键关机是否主从切换
		                if(sys_cb.discon_reason == 0xff) {
		                    sys_cb.discon_reason = 0;   //不同步关机
		                }
	            	}
           	 		sys_cb.pwrdwn_tone_en = 1;
            		func_cb.sta = FUNC_PWROFF;
					break;
#endif

#if USER_TWS_PAIR_MODE
			case EVT_TWS_PAIR:
				 if(!bt_tws_is_connected())
	            {
	                bt_tws_search_slave(15000);
	            }
				break;
#endif

#if USER_INEAR_DETECT_EN
    case EVT_TWS_SWITCH:
        //bt_audio_bypass();
        bsp_sys_mute();
        if (bt_get_disp_status() <= BT_STA_PLAYING) {
            break;
        }
        if (sys_cb.from_box_power_on != 1) //仓内开机时，不进行主从切换
        {
            printf("EVT_TWS_SWITCH \n");
            bt_tws_switch();
        }
        break;
    case EVT_CALL_PRIVATE_SWITCH:
        bt_tws_switch();
        break;
    case EVT_INEAR:

        printf("EVT_INEAR \n");
        if (CHARGE_INBOX()) { //如果不在仓里才解mute,在仓里不解mute
            sys_cb.ruer_close_touch_flag = 0;
            break;
        } else {
            bsp_sys_unmute();
        }
        sys_cb.ruer_close_touch_flag = 0;
#if GT_TEST_EN
        gt_ruer_in_spp_tx();
#endif
        if (!bt_nor_is_connected()) { //未跟手机连接，不触发入耳检测
            break;
        }
#if FIRST_RUER_NO_RESPON
        if (sys_cb.first_connect_ruer && bt_get_disp_status() <= BT_STA_PLAYING) //第一次连接不触发入耳时间
        {
            break;
        }
#endif
        if (status <= BT_STA_PLAYING) {
            bt_music_play();
        }
        if (status > BT_STA_INCOMING) {
            sys_cb.ruer_call_cnt = RUER_CNT;
            sys_cb.outear_call_cnt = 0;
#if SINGLE_RUER_EN
            if (!bt_tws_is_connected() && !sco_is_connected()) {

				printf("bt_call_private_switch  1\n");
                bt_call_private_switch();
                break;
            }
#endif
            if (!sco_is_connected()) {
#if FACTROY_TEST_EN
                if (sys_cb.ruer_cmd_open) {
                    break;
                }
#endif

				printf("bt_call_private_switch	3\n");
                bt_call_private_switch();
            }
//            if (!sys_cb.loc_ear_sta && sys_cb.rem_ear_sta && bt_tws_is_slave()) //如果两个耳机都出耳，本地耳机入耳
//            {
//                sys_cb.private_call_switch_time = 15;
//            }
        }
        break;

    case EVT_OUTEAR:
        printf("EVT_OUTEAR \n");
        if (CHARGE_INBOX()) { //如果不在仓里才解mute,在仓里不解mute
            if (bt_nor_is_connected()) //如果是连接手机的状态，出耳触摸无效
            {
                sys_cb.ruer_close_touch_flag = 1;
            }
            break;
        }
#if GT_TEST_EN
        gt_ruer_out_spp_tx();
#endif
        if (bt_nor_is_connected()) {
            sys_cb.ruer_close_touch_flag = 1;
        } else {
            sys_cb.ruer_close_touch_flag = 0;
        }
#if FIRST_RUER_NO_RESPON
        if (!bt_tws_is_playing() && bt_get_disp_status() <= BT_STA_PLAYING && !sys_cb.outear_pause) {
            sys_cb.first_connect_ruer = 1;
        }
        if (sys_cb.first_connect_ruer && bt_get_disp_status() <= BT_STA_PLAYING) {
            break;
        }
#endif
        if (!bt_nor_is_connected()) { //未跟手机连接，不触发入耳检测
            break;
        }
        if ((status == BT_STA_PLAYING) && (bt_get_siri_status() != 1)) {
            sys_cb.outear_pause = 2;
            bt_music_pause();
            bt_tws_user_key(0x30);
        }
        if (status > BT_STA_INCOMING) {
            sys_cb.outear_call_cnt = RUER_CNT;
            sys_cb.ruer_call_cnt = 0;
#if SINGLE_RUER_EN
            if (!bt_tws_is_connected() && sco_is_connected()) {

				printf("bt_call_private_switch  4\n");
                bt_call_private_switch();
                break;
            }
#endif
            //msg_enqueue(EVT_PRIVATE_CALL);

//            if (sys_cb.loc_ear_sta && !sys_cb.rem_ear_sta && !bt_tws_is_slave()) //如果两个耳机都入耳，本地耳机入耳
//            {
//                sys_cb.private_call_switch_time = 15;
//            }
        }

			    break;
#endif //USER_INEAR_DETECT_EN


#if QUERY_VERSION_EN
    case EVT_LONGPRESS_3S:
        printf("EVT_LONGPRESS_3S\n");
#if USER_INEAR_DETECT_EN			//出耳 触摸失效
			if(sys_cb.ruer_close_touch_flag){
					break;
			}
#endif
        if (!bt_tws_is_connected() && !bt_nor_is_connected() && sys_cb.double_click_query_version_flag) {
            sys_cb.double_click_query_version_flag = 0; //双击查看版本号标志
            func_cb.mp3_res_play(RES_BUF_VER_MP3, RES_LEN_VER_MP3);
        }
        break;
#endif //WL_VERSION_EN

#if BT_TWS_EN
        case EVT_BT_UPDATE_STA:
			printf("fuslh status\n");
            f_bt.disp_status = 0xff;    //刷新显示
            break;
#endif
        case KL_NEXT_VOL_UP:
        case KH_NEXT_VOL_UP:
        case KL_PREV_VOL_UP:
        case KH_PREV_VOL_UP:
        case KL_VOL_UP:
        case KH_VOL_UP:
            sys_cb.maxvol_fade = 1;
        case KU_VOL_UP_NEXT:
        case KU_VOL_UP_PREV:
        case KU_VOL_UP:
        case KU_VOL_UP_DOWN:
            if(bt_is_support_vol_ctrl() && bsp_bt_hid_vol_change(HID_KEY_VOL_UP)){
                if (!sys_cb.incall_flag) {
            #if WARNING_MAX_VOLUME
                    if (sys_cb.vol == VOL_MAX) {
                        maxvol_tone_play();
                    }
            #endif // WARNING_MAX_VOLUME
                 }
            }else{
                if (sys_cb.incall_flag) {
                    bsp_bt_call_volume_msg(KU_VOL_UP);
                } else {
                    bsp_set_volume(bsp_volume_inc(sys_cb.vol));
                    bsp_bt_vol_change();
                    printf("current volume: %d\n", sys_cb.vol);
            #if WARNING_MAX_VOLUME
                    if (sys_cb.vol == VOL_MAX) {
                        maxvol_tone_play();
                    }
            #endif // WARNING_MAX_VOLUME
                    if (func_cb.set_vol_callback) {
                        func_cb.set_vol_callback(1);
                    }
                }
            }
            break;

        case KLU_VOL_UP:
        case KLU_NEXT_VOL_UP:
            if (sys_cb.maxvol_fade == 2) {
                dac_fade_in();
            }
            sys_cb.maxvol_fade = 0;
            break;

        case KL_PREV_VOL_DOWN:
        case KH_PREV_VOL_DOWN:
        case KL_NEXT_VOL_DOWN:
        case KH_NEXT_VOL_DOWN:
        case KU_VOL_DOWN_PREV:
        case KU_VOL_DOWN_NEXT:
        case KU_VOL_DOWN:
        case KL_VOL_DOWN:
        case KH_VOL_DOWN:
        case KL_VOL_UP_DOWN:
        case KH_VOL_UP_DOWN:
            if(bt_is_support_vol_ctrl() && bsp_bt_hid_vol_change(HID_KEY_VOL_DOWN)){
                if (!sys_cb.incall_flag) {
            #if WARNING_MIN_VOLUME
                    if (sys_cb.vol == 0) {
                        if (func_cb.mp3_res_play) {
                            func_cb.mp3_res_play(RES_BUF_MAX_VOL_MP3, RES_LEN_MAX_VOL_MP3);
                        }
                    }
            #endif // WARNING_MIN_VOLUME
                }
            }else{
                if (sys_cb.incall_flag) {
                    bsp_bt_call_volume_msg(KU_VOL_DOWN);
                } else {
                    bsp_set_volume(bsp_volume_dec(sys_cb.vol));
                    bsp_bt_vol_change();
                    printf("current volume: %d\n", sys_cb.vol);
            #if WARNING_MIN_VOLUME
                    if (sys_cb.vol == 0) {
                        if (func_cb.mp3_res_play) {
                            func_cb.mp3_res_play(RES_BUF_MAX_VOL_MP3, RES_LEN_MAX_VOL_MP3);
                        }
                    }
            #endif // WARNING_MIN_VOLUME
                    if (func_cb.set_vol_callback) {
                        func_cb.set_vol_callback(0);
                    }
                }
            }
            break;

    //长按PP/POWER软关机(通过PWROFF_PRESS_TIME控制长按时间)
    case KLH_POWER:
    case KLH_MODE_PWR:
    case KLH_PLAY_PWR_USER_DEF:
        break;
    case EVT_LONGPRESS_5S:
        printf("EVT_LONGPRESS_5S \n");
#if USER_INEAR_DETECT_EN			//出耳 触摸失效
		if(sys_cb.ruer_close_touch_flag){
				break;
		}
#endif
#if GT_TEST_EN //入耳测试模式不能按键关机
        if (sys_cb.ruer_cmd_open && IS_PWRKEY_PRESS()) {
            break;
        }
#endif
            if(xcfg_cb.bt_tswi_kpwr_en) {       //按键关机是否主从切换
                if(sys_cb.discon_reason == 0xff) {
                    sys_cb.discon_reason = 0;   //不同步关机
                }
            }
#if WL_TIME_OUT_PWROFF
        if (!bt_tws_is_slave()) {
            if (!sys_cb.pwr_off_timeout_cnt) {
                sys_cb.discon_reason = 0xff;
            }
        }
#endif
           	 	sys_cb.pwrdwn_tone_en = 1;
            	func_cb.sta = FUNC_PWROFF;
            break;

#if WL_TIME_OUT_PWROFF					//配对超时关机独立消息
	case EVT_300_PWR_OFF:
            if(xcfg_cb.bt_tswi_kpwr_en) {       //按键关机是否主从切换
	            if(sys_cb.discon_reason == 0xff) {
	                sys_cb.discon_reason = 0;   //不同步关机
	            }
            }
	        if (!bt_tws_is_slave()) {
	            if (!sys_cb.pwr_off_timeout_cnt) {
	                sys_cb.discon_reason = 0xff;
	            }
	        }
   	 	sys_cb.pwrdwn_tone_en = 1;
    	func_cb.sta = FUNC_PWROFF;
	break;
#endif





#if IRRX_HW_EN
        case KU_IR_POWER:
            func_cb.sta = FUNC_SLEEPMODE;
            break;
#endif

        case KU_MODE:
        case KU_MODE_PWR:
            func_cb.sta = FUNC_NULL;
            break;

#if EQ_MODE_EN
        case KU_EQ:
            sys_set_eq();
            break;
#endif // EQ_MODE_EN

        case KU_MUTE:
            if (sys_cb.mute) {
                bsp_sys_unmute();
            } else {
                bsp_sys_mute();
            }
            break;

#if SYS_KARAOK_EN
        case KU_VOICE_RM:
            karaok_voice_rm_switch();
            break;
#if SYS_MAGIC_VOICE_EN
        case KL_VOICE_RM:
            magic_voice_switch();
            break;
#endif
#endif

#if ANC_EN
        case KU_ANC:
        case KD_ANC:
        case KL_ANC:
            sys_cb.anc_user_mode++;
            if (sys_cb.anc_user_mode > 2) {
                sys_cb.anc_user_mode = 0;
            }
            bsp_anc_set_mode(sys_cb.anc_user_mode);
            break;
#endif

        case MSG_SYS_500MS:
            break;

#if MUSIC_UDISK_EN
        case EVT_UDISK_INSERT:
            if (dev_is_online(DEV_UDISK)) {
                if (dev_udisk_activation_try(0)) {
                    sys_cb.cur_dev = DEV_UDISK;
                    func_cb.sta = FUNC_MUSIC;
                }
            }
            break;
#endif // MUSIC_UDISK_EN

#if MUSIC_SDCARD_EN
        case EVT_SD_INSERT:
            if (dev_is_online(DEV_SDCARD)) {
                sys_cb.cur_dev = DEV_SDCARD;
                func_cb.sta = FUNC_MUSIC;
            }
            break;
#endif // MUSIC_SDCARD_EN

#if MUSIC_SDCARD1_EN
        case EVT_SD1_INSERT:
            if (dev_is_online(DEV_SDCARD1)) {
                sys_cb.cur_dev = DEV_SDCARD1;
                func_cb.sta = FUNC_MUSIC;
            }
            break;
#endif // MUSIC_SDCARD1_EN

#if FUNC_USBDEV_EN
        case EVT_PC_INSERT:
            if (dev_is_online(DEV_USBPC)) {
                func_cb.sta = FUNC_USBDEV;
            }
            break;
#endif // FUNC_USBDEV_EN

#if LINEIN_DETECT_EN
        case EVT_LINEIN_INSERT:
            if (dev_is_online(DEV_LINEIN)) {
#if LINEIN_2_PWRDOWN_EN
                sys_cb.pwrdwn_tone_en = LINEIN_2_PWRDOWN_TONE_EN;
                func_cb.sta = FUNC_PWROFF;
#else
                func_cb.sta = FUNC_AUX;
#endif // LINEIN_2_PWRDOWN_EN
            }
            break;
#endif // LINEIN_DETECT_EN

        case EVT_A2DP_VOL_SAVE:
            param_sys_vol_write();
#if  BT_A2DP_RECORD_DEVICE_VOL
            a2dp_set_cur_dev_vol(sys_cb.vol);
#endif
            sys_cb.cm_times = 0;
            sys_cb.cm_vol_change = 1;
            break;
#if BT_A2DP_RECORD_DEVICE_VOL
        case EVT_A2DP_SAVE_DEV_VOL:
            a2dp_save_dev_vol();
            break;
#endif
        case EVT_TWS_SET_VOL:
        case EVT_A2DP_SET_VOL:
        	{
	            if(bsp_tws_sync_is_play())	//wait
				{
					msg_enqueue(msg);
				} else {
		            if((sys_cb.incall_flag & INCALL_FLAG_SCO) == 0) {
		                printf("A2DP SET VOL: %d\n", sys_cb.vol);
		                bsp_change_volume(sys_cb.vol);
		                if (sys_cb.incall_flag == 0) {
		                    gui_box_show_vol();
		                }
		            }
		            param_sys_vol_write();
#if  BT_A2DP_RECORD_DEVICE_VOL
                    a2dp_set_cur_dev_vol(sys_cb.vol);
#endif
		            sys_cb.cm_times = 0;
		            sys_cb.cm_vol_change = 1;
				}
        	}
            break;
        case EVT_BT_SCAN_START:
            if (bt_get_status() < BT_STA_SCANNING) {
                bt_scan_enable();
            }
            break;
#if EQ_DBG_IN_UART || EQ_DBG_IN_SPP
        case EVT_ONLINE_SET_EQ:
            bsp_eq_parse_cmd();
            break;
#endif
#if ANC_EN
        case EVT_ONLINE_SET_ANC:
            bsp_anc_parse_cmd();
            break;
#endif

#if DMIC_DBG_EN  && (BT_SNDP_DMIC_EN || BT_SCO_BCNS_EN || BT_SCO_DMNS_EN)
        case EVT_ONLINE_SET_ENC:
            bsp_enc_parse_cmd();
            break;
#endif

#if SMIC_DBG_EN && (BT_SCO_AINS2_EN || BT_SNDP_EN || BT_SCO_DNN_EN)
        case EVT_ONLINE_SET_SMIC:
            bsp_smic_parse_cmd();
            break;
#endif


#if SYS_KARAOK_EN
        case EVT_ECHO_LEVEL:
//            printf("echo level:%x\n", sys_cb.echo_level);
            bsp_echo_set_level();
            break;

        case EVT_MIC_VOL:
//            printf("mic vol:%x\n", sys_cb.mic_vol);
            bsp_karaok_set_mic_volume();
            break;

        case EVT_MUSIC_VOL:
//            printf("music vol:%x\n", sys_cb.music_vol);
            bsp_karaok_set_music_volume();
            break;
#endif
#if LANG_SELECT == LANG_EN_ZH
        case EVT_BT_SET_LANG_ID:
            param_lang_id_write();
            param_sync();
            break;
#endif

#if EQ_MODE_EN
        case EVT_BT_SET_EQ:
            music_set_eq_by_num(sys_cb.eq_mode);
            break;
#endif

#if MIC_DETECT_EN
        case EVT_MIC_INSERT:
            karaok_mic_unmute();
            break;

        case EVT_MIC_REMOVE:
            karaok_mic_mute();
            break;
#endif

#if CHARGE_EN
        //耳机入仓关机
        case EVT_CHARGE_INBOX:
            if(sys_cb.discon_reason == 0xff) {
                sys_cb.discon_reason = 0;   //不同步关机
            }
            sys_cb.pwrdwn_tone_en = 0;
            sys_cb.inbox_pwrdwn_flag = 1;
            bsp_charge_inbox_wakeup_enable();
            func_cb.sta = FUNC_PWROFF;
            break;
#endif // CHARGE_EN

#if BT_TWS_MS_SWITCH_EN
        case EVT_CHARGE_DCIN:
            bt_switch_exit();
            break;
#endif

#if VUSB_TBOX_QTEST_EN
        case EVT_QTEST_PICKUP_PWROFF:
            func_cb.sta = FUNC_PWROFF;
            break;
#endif

        case EVT_HFP_SET_VOL:
            if(sys_cb.incall_flag & INCALL_FLAG_SCO){
                bsp_change_volume(bsp_bt_get_hfp_vol(sys_cb.hfp_vol));
                param_hfp_vol_write();
                sys_cb.cm_times = 0;
                sys_cb.cm_vol_change = 1;
            }
            break;

        case EVT_A2DP_LLTY_EN:
#if DAC_DNR_EN
            dac_dnr_set_sta(0);
#endif
            break;

        case EVT_A2DP_LLTY_DIS:
#if DAC_DNR_EN
            if (bt_get_status() < BT_STA_OUTGOING) {
                dac_dnr_set_sta(sys_cb.dnr_sta);
            }
#endif
            break;

        case EVT_AUTO_PWFOFF_EN:
            en_auto_pwroff();
            sys_cb.sleep_en = BT_PAIR_SLEEP_EN;
            break;

        case EVT_AUTO_PWFOFF_DIS:
            dis_auto_pwroff();
            sys_cb.sleep_en = 1;
            break;
    }

    //调节音量，3秒后写入flash
    if ((sys_cb.cm_vol_change) && (sys_cb.cm_times >= 6)) {
        sys_cb.cm_vol_change = 0;
        cm_sync();
    }

#if SD_SOFT_DETECT_EN
    sd_soft_cmd_detect(120);
#endif
}

///进入一个功能的总入口
AT(.text.func)
void func_enter(void)
{
    gui_box_clear();
    param_sync();
    reset_sleep_delay();
    reset_pwroff_delay();
    func_cb.mp3_res_play = NULL;
    func_cb.set_vol_callback = NULL;
    bsp_clr_mute_sta();
    sys_cb.voice_evt_brk_en = 1;    //播放提示音时，快速响应事件。
    AMPLIFIER_SEL_D();
#if SYS_KARAOK_EN
    karaok_voice_rm_disable();
    bsp_karaok_echo_reset_buf(func_cb.sta);
#endif
}

AT(.text.func)
void func_exit(void)
{
    u8 func_num;
    u8 funcs_total = get_funcs_total();

    for (func_num = 0; func_num != funcs_total; func_num++) {
        if (func_cb.last == func_sort_table[func_num]) {
            break;
        }
    }
    func_num++;                                     //切换到下一个任务
    if (func_num >= funcs_total) {
        func_num = 0;
    }
    func_cb.sta = func_sort_table[func_num];        //新的任务
#if SYS_MODE_BREAKPOINT_EN
    param_sys_mode_write(func_cb.sta);
#endif // SYS_MODE_BREAKPOINT_EN
}

AT(.text.func)
void func_run(void)
{
    printf("%s\n", __func__);

    func_bt_chk_off();
    while (1) {
        func_enter();
        switch (func_cb.sta) {
#if FUNC_MUSIC_EN
        case FUNC_MUSIC:
            func_music();
            break;
#endif // FUNC_MUSIC_EN

#if EX_SPIFLASH_SUPPORT
        case FUNC_EXSPIFLASH_MUSIC:
            func_exspifalsh_music();
            break;
#endif

#if FUNC_CLOCK_EN
        case FUNC_CLOCK:
            func_clock();
            break;
#endif // FUNC_CLOCK_EN

#if FUNC_BT_EN
        case FUNC_BT:
            func_bt();
            break;
#endif

#if FUNC_BTHID_EN
        case FUNC_BTHID:
            func_bthid();
            break;
#endif // FUNC_BTHID_EN

#if FUNC_AUX_EN
        case FUNC_AUX:
            func_aux();
            break;
#endif // FUNC_AUX_EN

#if FUNC_USBDEV_EN
        case FUNC_USBDEV:
            func_usbdev();
            break;
#endif

#if FUNC_SPDIF_EN
        case FUNC_SPDIF:
            func_spdif();
            break;
#endif

#if FUNC_FMAM_FREQ_EN
        case FUNC_FMAM_FREQ:
            func_fmam_freq();
            break;
#endif // FUNC_FMAM_FREQ_EN

#if FUNC_SPEAKER_EN
        case FUNC_SPEAKER:
            func_speaker();
            break;
#endif // FUNC_SPEAKER_EN

#if FUNC_IDLE_EN
        case FUNC_IDLE:
            func_idle();
            break;
#endif // FUNC_IDLE_EN

#if FUNC_BT_DUT_EN
        case FUNC_BT_DUT:
            func_bt_dut();
            break;

        case FUNC_BT_FCC:
            func_bt_fcc();
            break;
#endif // IODM_TEST_MODE

        case FUNC_PWROFF:
            func_pwroff(sys_cb.pwrdwn_tone_en);
            break;

        case FUNC_SLEEPMODE:
            func_sleepmode();
            break;

#if FUNC_SINGLE_TEST_MODE
		case FUNC_SINGLE:
			func_bt_single();
			break;
#endif
        default:
            func_exit();
            break;
        }
    }
}
