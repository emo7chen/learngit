#include "include.h"
#include "func.h"
#include "func_bt.h"


func_bt_t f_bt;
void uart_cmd_process(void);

ALIGNED(128)
u16 func_bt_chkclr_warning(u16 bits)
{
    u16 value;
    GLOBAL_INT_DISABLE();
    value = f_bt.warning_status & bits;
    if(value != 0) {
        f_bt.warning_status &= ~value;
        GLOBAL_INT_RESTORE();
        return value;
    }
    GLOBAL_INT_RESTORE();
    return value;
}


#if FUNC_BT_EN
#if BT_HFP_REC_EN
void bt_sco_rec_start(void);
void bt_sco_rec_stop(void);
void bt_sco_rec_mix_do(u8 *buf, u32 samples);
void bt_sco_rec_init(void);

AT(.com_text.bt_rec)
void bt_sco_rec_mix(u8 *buf, u32 samples)
{
    bt_sco_rec_mix_do(buf, samples);
}

AT(.text.func.bt)
void func_bt_sco_rec_init(void)
{
    rec_src.spr = SPR_8000;
    rec_src.nchannel = 0x01;
    rec_src.source_start = bt_sco_rec_start;
    rec_src.source_stop  = bt_sco_rec_stop;
    f_bt.rec_pause = 0;
    rec_cb.sco_flag = 1;
    bt_sco_rec_init();
}
#endif

#if BT_REC_EN
void bt_music_rec_start(void);
void bt_music_rec_stop(void);

AT(.text.func.bt)
void bt_music_rec_init(void)
{
    rec_src.spr = SPR_44100;
    if (DAC_OUT_SPR == DAC_OUT_48K) {
        rec_src.spr = SPR_48000;
    }
    rec_src.nchannel = 0x82;
    rec_src.source_start = bt_music_rec_start;
    rec_src.source_stop  = bt_music_rec_stop;
    f_bt.rec_pause = 0;
    rec_cb.sco_flag = 0;
}

AT(.text.func.bt)
bool bt_rec_status_process(void)
{
    if (func_cb.sta == FUNC_BT) {
        func_bt_status();
        if ((f_bt.disp_status > BT_STA_PLAYING) || (f_bt.disp_status < BT_STA_CONNECTED)) {
            if ((f_bt.disp_status > BT_STA_PLAYING) && rec_cb.sco_flag) {
                return true;
            }
            f_bt.rec_pause = 1;
            return false;       //结束录音
        }
    }
    return true;
}

AT(.text.func.bt)
void bt_music_rec_continue(void)
{
#if !BT_HFP_REC_EN
    if ((f_bt.rec_pause) && ((f_bt.disp_status == BT_STA_PLAYING) || (f_bt.disp_status == BT_STA_CONNECTED))) {
        msg_enqueue(KU_REC);    //继续录音
        f_bt.rec_pause = 0;
    }
#endif
}

#endif // BT_REC_EN

void func_bt_set_dac(u8 enable)
{
    if (bsp_dac_off_for_bt_conn()) {
        if (enable) {
            if (sys_cb.dac_sta == 0) {
                sys_cb.dac_sta = 1;
                dac_restart();
            }
        } else {
            if (sys_cb.dac_sta == 1) {
                sys_cb.dac_sta = 0;
                dac_power_off();
            }
        }
    }
}

void func_bt_mp3_res_play(u32 addr, u32 len)
{
    if (len == 0) {
        return;
    }
#if BT_REC_EN
    sfunc_record_pause();
#endif

    bt_audio_bypass();
    mp3_res_play(addr, len);
    bt_audio_enable();
#if BT_REC_EN
    sfunc_record_continue();
#endif
}

//切换提示音语言
void func_bt_switch_voice_lang(void)
{
#if (LANG_SELECT == LANG_EN_ZH)
    if (xcfg_cb.lang_id >= LANG_EN_ZH) {
        sys_cb.lang_id = (sys_cb.lang_id) ? 0 : 1;
        multi_lang_init(sys_cb.lang_id);
        param_lang_id_write();
        param_sync();
        if (xcfg_cb.bt_tws_en) {
            bt_tws_sync_setting();                                                      //同步语言
            bsp_tws_res_music_play(TWS_RES_LANGUAGE_EN + sys_cb.lang_id);               //同步播放语言提示音
        } else {
            func_bt_mp3_res_play(RES_BUF_LANGUAGE_MP3, RES_LEN_LANGUAGE_MP3);
        }
    }
#endif
}

#if BT_TWS_EN
static void func_bt_tws_set_channel(void)
{
    if(f_bt.tws_status & FEAT_TWS_FLAG) {   //对箱状态.
        tws_get_lr_channel();
        dac_mono_init(0, sys_cb.tws_left_channel);
    } else {
        dac_mono_init(1, 0);
    }
}
#endif

u8 func_bt_tws_get_channel(void)
{
#if BT_TWS_EN
    return sys_cb.tws_left_channel;
#else
    return false;
#endif
}

void func_bt_warning(void)
{
#if VUSB_TBOX_QTEST_EN
    if(qtest_get_mode()){
        func_bt_chkclr_warning(0xffff);
        return;
    }
#endif

    if(func_bt_chkclr_warning(BT_WARN_TWS_DISCON | BT_WARN_TWS_CON)) {
#if BT_TWS_EN
        if(xcfg_cb.bt_tws_en) {
            if(xcfg_cb.bt_tws_lr_mode != 0) {
                func_bt_tws_set_channel();
            }
        }
#endif
    }

    if(func_bt_chkclr_warning(BT_WARN_DISCON)) {
#if WARNING_BT_DISCONNECT
        bsp_tws_res_music_play(TWS_RES_DISCONNECT);
#endif // WARNING_BT_DISCONNECT
#if WARNING_BT_WAIT_CONNECT
        func_cb.mp3_res_play(RES_BUF_WAIT4CONN_MP3, RES_LEN_WAIT4CONN_MP3);
#endif // WARNING_BT_WAIT_CONNECT
    }

    if(func_bt_chkclr_warning(BT_WARN_PAIRING)) {

        bsp_tws_res_music_play(TWS_RES_PAIRING);
    }

#if MP3_PWRON_FIX_EN		// TWS配对打断开机提示音问题
	static u16 time;

	if(bt_tws_is_connected()){				//对耳
		if(sys_cb.tws_conn_flg == 1)
		{
			if(time++ >= 500){
				time = 0;
				sys_cb.tws_conn_flg = 2;
				bsp_tws_res_music_play(TWS_RES_TWS_CONN);
			}

		}else if(sys_cb.tws_conn_flg == 2 || sys_cb.tws_conn_flg == 3){ 	//无手机信息报tws再报paring、断开蓝牙报paring
			sys_cb.tws_conn_flg = 0;
			if(func_bt_chkclr_warning(BT_WARN_PAIRING)) {

				bsp_tws_res_music_play(TWS_RES_PAIRING);
			}
		}else if(sys_cb.tws_conn_flg == 4){ 							//有手机信息，回连失败报paring
			if(func_bt_chkclr_warning(BT_WARN_PAIRING)) {
				bsp_tws_res_music_play(TWS_RES_PAIRING);
				sys_cb.tws_conn_flg = 0;
			}
		}
	}else { 						//单耳

		if(func_bt_chkclr_warning(BT_WARN_PAIRING)) {
			bsp_tws_res_music_play(TWS_RES_PAIRING);
		}
	}

#endif


#if BT_TWS_EN
    if(xcfg_cb.bt_tws_en) {
        u16 tws_warning = func_bt_chkclr_warning(BT_WARN_TWS_SCON | BT_WARN_TWS_MCON );
        if(tws_warning != 0) {
            f_bt.tws_had_pair = 1;
            if (xcfg_cb.bt_tws_lr_mode != 0) {
                func_bt_tws_set_channel();
            }
#if TWS_CONNECT_MP3_EN
			if (tws_warning & BT_WARN_TWS_MCON) {
#if MP3_PWRON_FIX_EN		// TWS配对打断开机提示音问题
                sys_cb.tws_conn_flg = 1;
#else
				delay_5ms(200);
				bsp_tws_res_music_play(TWS_RES_TWS_CONN);
#endif
            }

#else
            ///固定声道方案，TWS连接后异步播放声音提示音。否则同步播放连接提示音
            if (xcfg_cb.bt_tws_lr_mode >= 8) {
                func_bt_tws_set_channel();
                tws_get_lr_channel();

                if(sys_cb.tws_left_channel) {
                    func_cb.mp3_res_play(RES_BUF_LEFT_CH_MP3, RES_LEN_LEFT_CH_MP3);
                } else {
                    bt_audio_bypass();
                    delay_5ms(200);
                    func_cb.mp3_res_play(RES_BUF_RIGHT_CH_MP3, RES_LEN_RIGHT_CH_MP3);
                    bt_audio_enable();
                }
            } else {
                if (tws_warning & BT_WARN_TWS_MCON) {
                    bsp_tws_res_music_play(TWS_RES_CONNECTED);
                }
            }
#endif ////TWS_CONNECT_MP3_EN
        }
    }
#endif

	if(func_bt_chkclr_warning(BT_WARN_CON)) {
#if WARNING_BT_CONNECT
        bsp_tws_res_music_play(TWS_RES_CONNECTED);
#if 0//NO_CONNECT_MP3_EN        
        //if (!CHARGE_DC_IN()) {
            if (sys_cb.delay_connect_flag == 0) {
                printf("connect reset\n");
                sys_cb.delay_connect_flag = 41;
            } else if (sys_cb.delay_connect_flag == 1){
                printf("CONNNECTED\n");
                //if (!a2dp_is_playing_fast() || !sco_is_connected()||bt_get_disp_status()<BT_STA_PLAYING){
                if (bt_get_disp_status()<BT_STA_PLAYING){
					printf("bt_get_disp_status == %d\n",bt_get_disp_status());
					printf("bsp_tws_res_music_play 11111 TWS_RES_CONNECTED \n");
                    bsp_tws_res_music_play(TWS_RES_CONNECTED);
                }
                sys_cb.delay_connect_flag = 0;
            }
        //}

#endif


		
#endif
    }

#if BT_HID_MANU_EN
    //按键手动断开HID Profile的提示音
    if (xcfg_cb.bt_hid_manu_en) {
    #if WARNING_BT_HID_MENU
        if (func_bt_chkclr_warning(BT_WARN_HID_CON)) {
            func_cb.mp3_res_play(RES_BUF_CAMERA_ON_MP3, RES_LEN_CAMERA_ON_MP3);
        }
    #endif

    #if WARNING_BT_HID_MENU
        if (func_bt_chkclr_warning(BT_WARN_HID_DISCON)) {
            func_cb.mp3_res_play(RES_BUF_CAMERA_OFF_MP3, RES_LEN_CAMERA_OFF_MP3);
        }
    #endif

    #if BT_HID_DISCON_DEFAULT_EN
        if (f_bt.hid_discon_flag) {
            if (bt_hid_is_ready_to_discon()) {
                f_bt.hid_discon_flag = 0;
                bt_hid_disconnect();
            }
        }
    #endif // BT_HID_DISCON_DEFAULT_EN
    }
#endif // BT_HID_MANU_EN
}

void func_bt_disp_status(void)
{
    uint status = bt_get_disp_status();
#if CONNECT_TWS_1S_LED
	if(sys_cb.tws_conn_1s_flag)
	{
		sys_cb.tws_conn_1s_flag = 0;
		if(bt_tws_is_slave())
		{
			sys_cb.close_led = 1;
			/*从机发送消息，向主机说明自己已经准备完成，可以进行同步亮灯*/
			bt_tws_user_key(TWS_USER_KEY_TWS_CONNECT_1S);
		}
	}
#endif

#if CONNECT_PHONE_1S_LED
	if(sys_cb.connect_phone_flag)
	{
		sys_cb.connect_phone_flag = 0;
		sys_cb.close_led = 1;
		sys_cb.connect_flush_cnt = 1;
		led_connect_1s();
		bt_tws_user_key(TWS_USER_KEY_CONNECT_PHONE_1S);
	}
#endif

#if 0//CHARGE_INBOX_CHECK
	if((sys_cb.inbox_led_flag && !sys_cb.from_box_power_on) &&!sys_cb.into_pair_flag)
	{
		led_on();
		rled_off();
		return;
	}
#endif
#if CHARGE_INBOX_CHECK
		if((sys_cb.inbox_led_flag && !sys_cb.from_box_power_on) &&!sys_cb.into_pair_flag &&func_cb.sta !=FUNC_BT_DUT)
		{
			sys_cb.tws_connect_fail_close_flush = 0;
			if(func_cb.sta != FUNC_SINGLE && sys_cb.i2c_flag!=2){  //优化单耳模式不需要白灯常亮
				led_on();
				rled_off();
			}
		    if(f_bt.disp_status != status) {
		        f_bt.disp_status = status;
//		        f_bt.sta_update = 1;
				if(f_bt.disp_status >= BT_STA_CONNECTED) {
					f_bt.need_pairing = 1;
					func_bt_set_dac(1);
				} else {
					func_bt_set_dac(0);
				}
		    }
			return;
		}
#endif

	if(sys_cb.tws_connect_fail_close_flush>1)
	{

		if(func_cb.sta == FUNC_SINGLE)		//优化单耳模式无法闪灯
		{
			sys_cb.tws_connect_fail_close_flush = 0;
			if(status < BT_STA_CONNECTED)
			{
				led_bt_idle();
			}
		}
		if(f_bt.disp_status == 0xff) sys_cb.tws_connect_fail_close_flush = 0xff;
		if(f_bt.disp_status != status) {
		        f_bt.disp_status = status;
//		        f_bt.sta_update = 1;
				if(f_bt.disp_status >= BT_STA_CONNECTED) {
					f_bt.need_pairing = 1;
					func_bt_set_dac(1);
				} else {
					func_bt_set_dac(0);
				}
		 }
		if(sys_cb.tws_connect_fail_close_flush != 0xff)
			return;
	}


    if(f_bt.disp_status != status) {
        f_bt.disp_status = status;
//        f_bt.sta_update = 1;
//        if(!bt_is_connected()) {
//            en_auto_pwroff();
//            sys_cb.sleep_en = BT_PAIR_SLEEP_EN;
//        } else {
//            dis_auto_pwroff();
//            sys_cb.sleep_en = 1;
//        }


#if CONNECT_TWS_1S_LED||CONNECT_PHONE_1S_LED
		if(sys_cb.close_led)
		{
			return;
		}
#endif
#if WL_SINGLE_PWR_ON		//消抖，防止单耳开机时，一直走connect start fail ,刷新灯
		if(sys_cb.tws_connect_fail_close_flush>1){
			return;
		}
#endif
        switch (f_bt.disp_status) {
        case BT_STA_CONNECTING:
			printf("BT_STA_CONNECTING \n");
#if WL_SINGLE_PWR_ON		//消抖，防止单耳开机时，一直走connect start fail ,刷新灯
			if(sys_cb.tws_connect_fail_close_flush>0){
				break;
			}
#endif
			if(!sys_cb.single_linkloss_flag && sys_cb.tws_disccon_flag)
			{
				break;
			}
			bt_set_scan(2);
			led_bt_reconnect();
#if CHARGE_INBOX_CHECK
			if(bt_tws_is_connected() && !bt_tws_is_slave() )
			{
					sys_cb.led_master_sta = 0x22;			//回连灯状态
			}
#endif
            if (BT_RECONN_LED_EN) {
                led_bt_reconnect();
                break;
            }
			break;
        case BT_STA_INITING:
			break;
        case BT_STA_IDLE:
#if WL_LINKLOSS_EN
			if(sys_cb.link_loss_flag)
			{
				bt_set_scan(2);
				led_bt_reconnect();
#if CHARGE_INBOX_CHECK
				if(bt_tws_is_connected() && !bt_tws_is_slave() )
				{
					sys_cb.led_master_sta = 0x22;			//回连灯状态
				}
#endif
			}
			else
#endif
            {
				printf("BT_STA_IDLE\n");
				printf("sys_cb.is_need_pairing  == %d\n",sys_cb.is_need_pairing);
				bt_set_scan(3);
				led_bt_idle();
#if CHARGE_INBOX_CHECK
				if(bt_tws_is_connected() && !bt_tws_is_slave() )
				{
					sys_cb.led_master_sta = 0x21;			//配对状态灯
				}
#endif
#if DISCONN_PAIR_CLOSE_MP3
				if(sys_cb.is_need_pairing && f_bt.disp_status == BT_STA_IDLE)
				{
					sys_cb.is_need_pairing = 0;
#if TWS_CONN_FIRST_PAIR					//修改TWS连接成功提示音比pairing延迟播报的问题
					printf("pairing ----------------------------------------------1\n");
					msg_enqueue(PAIRIG_MP3);
#else
					f_bt.warning_status |= BT_WARN_PAIRING;
#endif
				}
#endif


#if 0//WARNING_BT_PAIR
				if(f_bt.need_pairing && f_bt.disp_status == BT_STA_IDLE) {
					f_bt.need_pairing = 0;
					if(xcfg_cb.warning_bt_pair && xcfg_cb.bt_tws_en) {
						f_bt.warning_status |= BT_WARN_PAIRING;
						printf("pairing pairing pairing pairing\n");
					}
				}
#endif
			}

            break;
        case BT_STA_SCANNING:
			printf("BT_STA_SCANNING\n");
#if DUT_MODE_EN
			if(func_cb.sta == FUNC_BT_DUT)
			{
				bt_set_scan(3);
				break;
			}
#endif
#if SLNGLE_LINKLOSS
			if(sys_cb.single_linkloss_flag || sys_cb.link_loss_flag)
			{
				bt_set_scan(2);
				led_bt_reconnect();
				break;
			}
#endif			

#if USER_TWS_PAIR_MODE
			if(sys_cb.tws_pair_led_flag == 1  && !sys_cb.pwrkey_5s_flag &&func_cb.sta!=FUNC_SINGLE)			//TWS未连接上时，TWS配对灯
			{
				printf("led_bt_tws pair led\n");
				bt_set_scan(2);
				led_bt_reconnect();
			}
			else
			{
				printf("led_bt_scan \n");

				led_bt_idle();
				bt_set_scan(3);
#if DISCONN_PAIR_CLOSE_MP3
				if(sys_cb.pwrkey_5s_flag)
				{
					break;
				}
				if(sys_cb.is_need_pairing && f_bt.disp_status == BT_STA_SCANNING)
				{

					sys_cb.is_need_pairing = 0;

					printf("pairing ----------------------------------------------2\n");
					if(sys_cb.pairing_two_blem){			//修改1只耳机入仓关盖，然后手动断链报两次pairing得问题
						sys_cb.pairing_two_blem = 0;
						break;
					}
#if TWS_PAIRING_BLEM
					if(sys_cb.tws_disccon_flag){			//双耳主动断链，播报一次pairing ,单耳入仓关盖，TWS断链会播报第二次
						//sys_cb.tws_disccon_flag = 0;
						break;
					}
#endif
					f_bt.warning_status |= BT_WARN_PAIRING;
					//msg_enqueue(PAIRIG_MP3);
				}
#endif
			}

#else
            led_bt_scan();
#endif
            break;

        case BT_STA_DISCONNECTING:
            led_bt_connected();
            break;

        case BT_STA_CONNECTED:
			printf("BT_STA_CONNECTED    1\n");
			if(!bt_tws_is_slave())
			{
				//printf("BT_STA_CONNECTED   2\n");
				led_bt_connected();
			}
#if CHARGE_INBOX_CHECK
			else if(bt_tws_is_slave() && bt_nor_is_connected())
			{		//主机入仓主从切换，状态切换不过来，会导致灯常亮
				//printf("slaver BT_STA_CONNECTED   3\n");
				led_bt_connected();
			}
			if(bt_tws_is_connected() && !bt_tws_is_slave() )
			{
					sys_cb.led_master_sta = 0x23;			//已连接状态
			}
#endif
            break;
        case BT_STA_INCOMING:
            led_bt_ring();
#if CHARGE_INBOX_CHECK
			if(bt_tws_is_connected() && !bt_tws_is_slave() )
			{
					sys_cb.led_master_sta = 0x24;			//来电灯状态
			}
#endif
            break;
        case BT_STA_PLAYING:
            led_bt_play();
#if CHARGE_INBOX_CHECK
			if(bt_tws_is_connected() && !bt_tws_is_slave() )
			{
					sys_cb.led_master_sta = 0x25;			//播放灯状态
			}
#endif
            break;
        case BT_STA_OUTGOING:
        case BT_STA_INCALL:
            led_bt_call();
#if CHARGE_INBOX_CHECK
			if(bt_tws_is_connected() && !bt_tws_is_slave() )
			{
					sys_cb.led_master_sta = 0x26;			//通话中灯状态
			}
#endif
            break;
        }

        if(f_bt.disp_status >= BT_STA_CONNECTED) {
            f_bt.need_pairing = 1;
            func_bt_set_dac(1);
        } else {
            func_bt_set_dac(0);
        }

#if BT_BACKSTAGE_EN
        if (f_bt.disp_status < BT_STA_PLAYING && func_cb.sta_break != FUNC_NULL) {
            func_cb.sta = func_cb.sta_break;
        }
#endif
    }
}

AT(.text.func.bt)
void func_bt_status(void)
{
    func_bt_disp_status();

#if FUNC_BTHID_EN
    if(is_bthid_mode()) {
        func_bt_hid_warning();
    } else
#endif
    {
        func_bt_warning();
    }
}


#if USER_INEAR_DETECT_EN
AT(.text.func.bt)
void func_bt_inear_process(void)
{
    static u32 ruer_tick = 0;
	static u32 outer_tick = 0;
	if(sys_cb.dc_out_flag > 0)
	{
		//printf("func_bt_inear_process111111111111111111111111	ruer_cnt  == %d\n",ruer_cnt);
		sys_cb.dc_out_flag = 0;
		sys_cb.ruer_cnt = 0;
		sys_cb.outear_cnt=0;
	}
	
#if USER_INEAR_DETECT_EN
	if (dev_is_online(DEV_EARIN)) {
			if(sys_cb.ruer_cnt == RUER_100_TIME)
			{
				//gt_ruer_in_spp_tx();
				if (sys_cb.loc_ear_sta) {
					bt_set_ear_sta(0);			//入耳
					//printf("func_bt_inear_process   ---- in \n");
					msg_enqueue(EVT_INEAR);
				}
			}
		if(tick_check_expire(ruer_tick, 300))
		{     //延时30ms再发
#if GT_TEST_EN
			if(!IS_PWRKEY_PRESS()){
				gt_ruer_in_spp_tx();
			}
			else
			{
				gt_touch_in_spp_tx();
			}
#endif
	   		ruer_tick = tick_get();
   		}
		}
		else
		{
			if(sys_cb.outear_cnt == RUER_100_TIME)
			{
				//gt_ruer_out_spp_tx();
				if (!sys_cb.loc_ear_sta)
				{
					printf("func_bt_inear_process   ---- out  \n");
					bt_set_ear_sta(1);			//摘下
					msg_enqueue(EVT_OUTEAR);
				}
			}

		if(tick_check_expire(outer_tick, 300))
		{     //延时30ms再发
#if GT_TEST_EN
			if(!IS_PWRKEY_PRESS()){
				gt_ruer_out_spp_tx();
			}
			else
			{
				gt_touch_in_spp_tx();
			}
			outer_tick = tick_get();
#endif
   		}
		}

#else

    if (dev_is_online(DEV_EARIN)) {
        if (sys_cb.loc_ear_sta) {
            if (sys_cb.rem_ear_sta) {     //检测到对耳已经入耳，不用播放入耳提示音
                func_cb.mp3_res_play(RES_BUF_INEAR_DU_MP3, RES_LEN_INEAR_DU_MP3);
            }
            bt_set_ear_sta(0);          //入耳
//            bt_music_play();          //播放音乐，需要时打开
        }
    } else {
        if (!sys_cb.loc_ear_sta) {
            bt_set_ear_sta(1);          //摘下
//            bt_music_pause();         //暂停播放
        }
    }
#endif
}
#endif // USER_TKEY_INEAR
#if BT_2ACL_AUTO_SWITCH
struct {
    uint32_t check_tick;
    uint16_t play_timer_cnt;
    uint16_t clear_timer_cnt;
} bt_silence;
bool bt_play_data_check(void)
{
    bool ret = false;
    if (tick_check_expire(bt_silence.check_tick, 10)) {
        bt_silence.check_tick = tick_get();
    } else {
        return ret;
    }
    //消抖
    if (!bt_is_silence()) {
        bt_silence.clear_timer_cnt = 0;
        bt_silence.play_timer_cnt++;
        if (bt_silence.play_timer_cnt > 100) {
            ret = true;
            bt_silence.play_timer_cnt = 0;
        }
    } else {
        bt_silence.clear_timer_cnt++;
        if (bt_silence.clear_timer_cnt > 100) {
            bt_silence.play_timer_cnt = 0;
        }
    }
    return ret;
}

void bt_play_data_init(void)
{
    memset(&bt_silence, 0, sizeof(bt_silence));
}
#endif
AT(.text.func.bt)
void func_bt_sub_process(void)
{
	static u32 tick = 0 ;
    func_bt_status();
#if USER_INEAR_DETECT_EN
    func_bt_inear_process();
#endif
#if USER_TKEY_DEBUG_EN
    bsp_tkey_spp_tx();
#endif
#if VUSB_SMART_VBAT_HOUSE_EN
    bsp_smart_vhouse_process(0);
#endif
#if BT_2ACL_AUTO_SWITCH
    if (bt_play_data_check()) {
        bt_music_play_switch();
    }
#endif
#if CHARGE_INBOX_CHECK
	if(tick_check_expire(tick, 500))
	{
		charge_inbox_check();	//充电仓入仓检测
		tick = tick_get();
	}

#endif
}

AT(.text.func.bt)
void func_bt_process(void)
{
    func_process();
    func_bt_sub_process();
#if BT_TWS_MS_SWITCH_EN
    if ((xcfg_cb.bt_tswi_msc_en) && bt_tws_need_switch(0) &&  !bt_tws_is_waiting_res_sync()) {
        printf("AUDIO SWITCH\n");
        bt_tws_switch();
    }
#endif

    if(f_bt.disp_status == BT_STA_INCOMING) {
		printf("func_bt_process  ---- ring\n");
        sfunc_bt_ring();
        reset_sleep_delay();
        reset_pwroff_delay();
        f_bt.siri_kl_flag = 0;
        f_bt.user_kl_flag = 0;
#if BT_REC_EN
        bt_music_rec_continue();
#endif
    } else if(f_bt.disp_status == BT_STA_OTA) {
        sfunc_bt_ota();
        reset_sleep_delay();
        reset_pwroff_delay();
    } else if(f_bt.disp_status >= BT_STA_OUTGOING) {
		if(f_bt.disp_status < 0xfe)
		{
	        sfunc_bt_call();
	        reset_sleep_delay();
	        reset_pwroff_delay();
	        f_bt.siri_kl_flag = 0;
	        f_bt.user_kl_flag = 0;
#if BT_REC_EN
	        bt_music_rec_continue();
#endif
		}
    }

    if(sys_cb.pwroff_delay == 0) {
        sys_cb.pwrdwn_tone_en = 1;      //连接超时关主从切换,同步关机
        func_cb.sta = FUNC_PWROFF;
        return;
    }
    if(sleep_process(bt_is_sleep)) {
        f_bt.disp_status = 0xff;
    }
}

AT(.text.func.bt)
void func_bt_init(void)
{
    if (!f_bt.bt_is_inited) {
        msg_queue_clear();
        func_bt_set_dac(0);
        bsp_bt_init();
        f_bt.bt_is_inited = 1;
    }
}

AT(.text.func.bt)
void func_bt_chk_off(void)
{
    if ((func_cb.sta != FUNC_BT) && (f_bt.bt_is_inited)) {
#if BT_PWRKEY_5S_DISCOVER_EN
        bsp_bt_pwrkey5s_clr();
#endif
        bt_disconnect(0);
        bt_off();
        func_bt_set_dac(1);
        f_bt.bt_is_inited = 0;
    }
}
void iis2bt_sco_init(void);
AT(.text.func.bt)
void func_bt_enter(void)
{
#if DUT_PIN_CODE_EN
	if(func_cb.sta == FUNC_BT_DUT)
	{
		cfg_bt_simple_pair_mode = 0;
	}
#endif
    func_cb.mp3_res_play = func_bt_mp3_res_play;
    func_bt_enter_display();
    led_bt_init();
    func_bt_init();
    //en_auto_pwroff();

#if WARNING_FUNC_BT
    mp3_res_play(RES_BUF_BT_MODE_MP3, RES_LEN_BT_MODE_MP3);
#endif // WARNING_FUNC_BT

#if WARNING_BT_WAIT_CONNECT
    mp3_res_play(RES_BUF_WAIT4CONN_MP3, RES_LEN_WAIT4CONN_MP3);
#endif // WARNING_BT_WAIT_CONNECT

    f_bt.disp_status = 0xfe;
    f_bt.rec_pause = 0;
    f_bt.pp_2_unmute = 0;
    sys_cb.key2unmute_cnt = 0;

    bt_redial_init();
    bt_audio_enable();
#if DAC_DNR_EN
    dac_dnr_set_sta(1);
    sys_cb.dnr_sta = 1;
#endif

#if BT_PWRKEY_5S_DISCOVER_EN
    if(bsp_bt_pwrkey5s_check()) {
        f_bt.need_pairing = 0;  //已经播报了
        func_bt_disp_status();
        func_bt_mp3_res_play(RES_BUF_PAIRING_MP3, RES_LEN_PAIRING_MP3);
    } else {
        func_bt_disp_status();
#if WARNING_BT_PAIR
        if (xcfg_cb.warning_bt_pair && !xcfg_cb.bt_tws_en) {
            func_bt_mp3_res_play(RES_BUF_PAIRING_MP3, RES_LEN_PAIRING_MP3);
        }
#endif // WARNING_BT_PAIR
    }
#endif

#if BT_REC_EN
    bt_music_rec_init();
#endif // BT_REC_EN

#if I2S_EXT_EN && I2S_2_BT_SCO_EN
    iis2bt_sco_init();
#endif // I2S_EXT_EN
#if BT_2ACL_AUTO_SWITCH
    bt_play_data_init();
#endif
}

AT(.text.func.bt)
void func_bt_exit(void)
{
    if(sys_cb.discon_reason == 0xff) {
        sys_cb.discon_reason = 1;   //默认同步关机
    }
#if BT_A2DP_RECORD_DEVICE_VOL
    a2dp_save_dev_vol();
#endif
#if BT_REC_EN
    sfunc_record_stop();
#endif // BT_REC_EN

    dac_fade_out();
#if DAC_DNR_EN
    dac_dnr_set_sta(0);
    sys_cb.dnr_sta = 0;
#endif
#if BT_PWRKEY_5S_DISCOVER_EN
    bsp_bt_pwrkey5s_clr();
#endif
    func_bt_exit_display();
    bt_audio_bypass();
#if BT_TWS_EN
    dac_mono_init(1, 0);
#endif
#if !BT_BACKSTAGE_EN
    bt_disconnect(sys_cb.discon_reason);
    bt_off();
#else
    if (bt_get_status() == BT_STA_PLAYING && !bt_is_testmode()) {        //蓝牙退出停掉音乐
        delay_5ms(10);
        if(bt_get_status() == BT_STA_PLAYING) {     //再次确认play状态
            u32 timeout = 850; //8.5s
            bt_music_pause();
            while (bt_get_status() == BT_STA_PLAYING && timeout > 0) {
                timeout--;
                delay_5ms(2);
            }
        }
    }
#endif
    f_bt.rec_pause = 0;
    f_bt.pp_2_unmute = 0;
    sys_cb.key2unmute_cnt = 0;
    f_bt.bt_is_inited = 0;
    func_bt_set_dac(1);
    func_cb.last = FUNC_BT;
}

#if BT_TWS_MS_SWITCH_EN
void bt_switch_exit(void)
{
#if VUSB_TBOX_QTEST_EN
    u32 qtest_5v_tick = tick_get();
    if(qtest_get_mode()){
        return ;
    }

    if(sys_cb.qtest_sta){
        while(!tick_check_expire(qtest_5v_tick,1000)){
            if(!DC_IN()){
                return;
            }
        }
        sys_cb.qtest_sta = 0;
    }
#endif
    if ((xcfg_cb.bt_tswi_charge_rst_en) || (func_cb.sta != FUNC_BT)) {
		sys_cb.discon_reason = 0;
        func_bt_exit();
        sw_reset_kick(SW_RST_DC_IN);                    //直接复位进入充电
    } else {
#if CHARGE_INBOX_CHECK
	   if(sys_cb.from_box_power_on==0)			//出仓之后，不进入充电流程
		{
			printf("no enter charge\n");
	   }
	   else if(sys_cb.from_box_power_on == 2)
	   {
			bt_tws_switch_for_charge();
	   }
#else
        bt_tws_switch_for_charge();
#endif
    }
}
#else
void bt_switch_exit(void){}
#endif // BT_TWS_MS_SWITCH_EN

AT(.text.func.bt)
void func_bt(void)
{
    printf("%s\n", __func__);

    func_bt_enter();
	sys_cb.inbox_cnt = 0;
    while (func_cb.sta == FUNC_BT) {
        func_bt_process();
        func_bt_message(msg_dequeue());
        func_bt_display();
    }

    func_bt_exit();
}

#endif //FUNC_BT_EN
