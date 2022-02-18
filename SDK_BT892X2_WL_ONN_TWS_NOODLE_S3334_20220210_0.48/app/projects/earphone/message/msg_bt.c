#include "include.h"
#include "func.h"
#include "func_bt.h"

u16 get_user_def_vol_msg(u8 func_sel)
{
    u16 msg = NO_MSG;

    if(func_sel == UDK_VOL_UP) {            //VOL+
        if(xcfg_cb.user_def_lr_en) {
            msg = func_bt_tws_get_channel()? KU_VOL_UP : KU_VOL_DOWN;
        } else {
            msg = KU_VOL_UP;
        }
    } else if(func_sel == UDK_VOL_DOWN) {   //VOL-
        if(xcfg_cb.user_def_lr_en) {
            msg = func_bt_tws_get_channel()? KU_VOL_DOWN : KU_VOL_UP;
        } else {
            msg = KU_VOL_DOWN;
        }
    }
    return msg;
}

void user_def_track_msg(u16 msg)
{
    if (msg == KU_PREV) {
        bt_music_prev();
    } else {
        bt_music_next();
    }
}

///检查USER_DEF按键配置功能
bool user_def_func_is_ready(u8 func_sel)
{
    if (func_sel == UDK_NONE) {
        return false;
    }
    if ((func_sel <= UDK_NEXT) || (func_sel == UDK_PLAY_PAUSE)) {
        if ((!bt_nor_is_connected()) && (f_bt.disp_status != BT_STA_PLAYING)) {
            return false;
        }
    }
    return true;
}

///检查USER_DEF按键消息处理
bool user_def_key_msg(u8 func_sel)
{
    u16 msg = NO_MSG;

    if (!user_def_func_is_ready(func_sel)) {
        return false;
    }

    if (func_sel == UDK_REDIALING) {
        bt_call_redial_last_number();                   //回拨电话
        if (func_cb.mp3_res_play) {
            func_cb.mp3_res_play(RES_BUF_REDIALING_MP3, RES_LEN_REDIALING_MP3);
        }
    } else if (func_sel == UDK_SIRI) {                  //SIRI
        bt_hfp_siri_switch();
    } else if (func_sel == UDK_NR) {                    //NR
        bt_ctl_nr_sta_change();
    } else if (func_sel == UDK_PREV) {                  //PREV
        if(xcfg_cb.user_def_lr_en) {
            msg = func_bt_tws_get_channel()? KU_PREV : KU_NEXT;
        } else {
            msg = KU_PREV;
        }
        user_def_track_msg(msg);
    } else if (func_sel == UDK_NEXT) {                  //NEXT
        if(xcfg_cb.user_def_lr_en) {
            msg = func_bt_tws_get_channel()? KU_NEXT : KU_PREV;
        } else {
            msg = KU_NEXT;
        }
        user_def_track_msg(msg);
    } else if (func_sel == UDK_MODE) {                  //MODE
        func_message(KU_MODE);
    } else if (func_sel == UDK_PHOTO) {
        return bsp_bt_hid_photo(HID_KEY_VOL_UP);        //拍照
    } else if (func_sel == UDK_HOME) {
        return bt_hid_consumer(HID_KEY_IOS_HOME);       //IOS Home按键功能
    } else if (func_sel == UDK_LANG) {
        func_bt_switch_voice_lang();                    //中英文切换
    } else if (func_sel == UDK_PLAY_PAUSE) {
        bt_music_play_pause();
    } else if (func_sel == UDK_DUT) {                  //CBT 测试模式
        if(func_cb.sta != FUNC_BT_DUT){
            func_cb.sta = FUNC_BT_DUT;
            sys_cb.discon_reason = 0;
        }
    } else if (func_sel == UDK_LOW_LATENCY) {
        bool low_latency = bt_is_low_latency();
        if (low_latency) {
            bsp_tws_res_music_play(TWS_RES_MUSIC_MODE);
        } else {
            bsp_tws_res_music_play(TWS_RES_GAME_MODE);
        }
    }else {                                            //VOL+, VOL-
        func_message(get_user_def_vol_msg(func_sel));
    }
    return true;
}

bool user_def_lkey_tone_is_enable(u8 func_sel)
{
    if (!xcfg_cb.user_def_kl_tone_en) {
        return false;
    }
    return user_def_func_is_ready(func_sel);
}

#if BT_HID_MANU_EN
//双击VOL-, VOL+的功能处理
void bt_hid_vol_msg(u8 sel)
{
    if (sel == 1) {
        bsp_bt_hid_photo(HID_KEY_VOL_UP);
    } else if (sel == 2) {
        bsp_bt_hid_photo(HID_KEY_VOL_DOWN);
    } else if (sel == 3) {
        bsp_bt_hid_tog_conn();
    }
}
#endif

bool bt_tws_pair_mode(u8 method)
{
    if ((xcfg_cb.bt_tws_en) && (xcfg_cb.bt_tws_pair_mode == method) && (!bt_nor_is_connected())) {
        if(bt_tws_is_connected()) {
            bt_tws_disconnect();
        } else {
            bt_tws_search_slave(15000);
        }
        return true;
    }
    return false;
}

AT(.text.func.bt.msg)
void func_bt_message(u16 msg)
{
    //int klu_flag = 0;

    switch (msg) {
#if 0//WL_SMART_HOUSE_EN
	case EVT_CLEAR_INFO:
			printf("EVT_CLEAR_INFO \n");
			if(bt_tws_is_connected()){
				bt_tws_disconnect();
				delay_5ms(2);
			}
			//sys_cb.clear_info_flag = 1;
			if(!bt_tws_is_slave())
			{
				printf("********************************************master\n");
			}
			bt_clr_all_link_info();                         //删除所有配对信息
			delay_5ms(2);
			led_off();
			LED_SET_OFF();
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
						printf("pair_mode_2\n");
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
						printf("pair_mode-1\n");
					}
				}
				 break;
#endif
	case EVT_LONGPRESS_1_5S:
			printf("EVT_LONGPRESS_1_5S： \n");
#if USER_INEAR_DETECT_EN			//出耳 触摸失效
			if(sys_cb.ruer_close_touch_flag){
					break;
			}
#endif
#if OUTBOX_NO_SIRI
			if(sys_cb.outbox_siri_cnt > 0)
			{
				break;
			}
#endif


#if UI_FUNCTION_EN					//基本功能
			if(bt_get_siri_status() == 1)
			{
				//printf("longpress key don't exit siri \n");
			}
			else{

				if(sys_cb.tws_left_channel)
				{
					bt_music_prev();		//左耳上一曲
				}
				else
				{
					bt_music_next();		//右耳下一曲
				}
			}
#endif //UI_FUNCTION_EN


		break;




    case KU_PLAY:
    case KU_PLAY_USER_DEF:
    case KU_PLAY_PWR_USER_DEF:
		sys_cb.touch_flag = 1;
		printf("one kill\n");
		wlink_touch_key_flag = 1;	//按键按下标志
#if USER_INEAR_DETECT_EN			//出耳 触摸失效
		if(sys_cb.ruer_close_touch_flag){
				break;
		}
#endif
//        if(sys_cb.key_en){
//            if(sys_cb.key_short_val){
//                user_def_key_msg(sys_cb.key_short_val);
//            }else{
//                if (!bt_nor_is_connected()) {
//                    bt_tws_pair_mode(3);           //单击PLAY按键手动配对
//                } else {
//                    //bt_music_play_pause();
//                    //f_bt.pp_2_unmute = sys_cb.mute;
//                }
//            }
//        }
#if 0//CLICK_ENTER_DUT
			if(!bt_nor_is_connected())		//	未跟手机连接，左右耳都可以进入DUT模式
			{
				if(func_cb.sta != FUNC_BT_DUT){
					printf("FUNC_BT_DUT\n");
					sys_cb.discon_reason = 0;	//不同步关机
					bt_clr_all_link_info();
					func_cb.sta = FUNC_BT_DUT;
				}
			}
#endif

        break;

    case KL_PLAY_PWR_USER_DEF:
			break;
        //klu_flag = 1;                                                       //长按抬键的时候呼SIRI
    case KL_PLAY_USER_DEF:
		printf("KL_PLAY_PWR_USER_DEF \n");
		
#if USER_INEAR_DETECT_EN			//出耳 触摸失效
		if(sys_cb.ruer_close_touch_flag){
				break;
		}
#endif
#if OUTBOX_NO_SIRI
		if(sys_cb.outbox_siri_cnt > 0)
		{
			break;
		}
#endif

	
#if UI_FUNCTION_EN					//基本功能
				if(sys_cb.tws_left_channel)
				{
					bt_music_prev();		//左耳上一曲
				}
				else
				{
					bt_music_next();		//右耳下一曲
				}
#else //UI_FUNCTION_EN
        f_bt.user_kl_flag = 0;
        if (!bt_tws_pair_mode(4)) {                                         //是否长按配对功能
            if (user_def_lkey_tone_is_enable(xcfg_cb.user_def_kl_sel)) {
                bsp_piano_warning_play(WARNING_TONE, 2);                    //长按“滴”一声
            }
            if (klu_flag) {
                f_bt.user_kl_flag = user_def_func_is_ready(xcfg_cb.user_def_kl_sel);     //长按抬键的时候再处理
            } else {
                user_def_key_msg(xcfg_cb.user_def_kl_sel);
            }
        }
#endif
        break;

        //SIRI, NEXT, PREV在长按抬键的时候响应,避免关机前切歌或呼SIRI了
    case KLU_PLAY_PWR_USER_DEF:
#if USER_INEAR_DETECT_EN			//出耳 触摸失效
		if(sys_cb.ruer_close_touch_flag){
				break;
		}
#endif
#if OUTBOX_NO_SIRI
				if(sys_cb.outbox_siri_cnt > 0)
				{
					break;
				}
#endif

        if (f_bt.user_kl_flag) {
            user_def_key_msg(xcfg_cb.user_def_kl_sel);
            f_bt.user_kl_flag = 0;
        }
        break;

    case KH_PLAY_USER_DEF:
#if USER_INEAR_DETECT_EN			//出耳 触摸失效
		if(sys_cb.ruer_close_touch_flag){
				break;
		}
#endif
		printf("KH_PLAY_USER_DEF\n");
        func_message(get_user_def_vol_msg(xcfg_cb.user_def_kl_sel));
        break;

    case KD_PLAY_USER_DEF:
    case KD_PLAY_PWR_USER_DEF:
		printf("KD_PLAY_PWR_USER_DEF \n");
#if USER_INEAR_DETECT_EN			//出耳 触摸失效
		if(sys_cb.ruer_close_touch_flag){
				break;
		}
#endif
#if OUTBOX_NO_SIRI
		if(sys_cb.outbox_siri_cnt > 0)
		{
			break;
		}
#endif

#if QUERY_VERSION_EN
		if(!bt_tws_is_connected() && !bt_nor_is_connected())
			sys_cb.double_click_query_version_flag = 1; 		//双击查看版本号标志
#endif	//WL_VERSION_EN

#if UI_FUNCTION_EN			//基本功能
		if(bt_get_siri_status() == 1)
		{
			printf("double key don't exit siri \n");
		}
		else
		{
			bt_music_play_pause();
			f_bt.pp_2_unmute = sys_cb.mute;
		}
#else	//UI_FUNCTION_EN
        if (user_def_key_msg(xcfg_cb.user_def_kd_sel)) {
#if BT_TWS_EN
        } else if(bt_tws_pair_mode(2)) {
#endif
        } else if (xcfg_cb.user_def_kd_lang_en) {
            func_bt_switch_voice_lang();
        }
#endif //UI_FUNCTION_EN
        break;

    ///三击按键处理
    case KTH_PLAY_USER_DEF:
    case KTH_PLAY_PWR_USER_DEF:
		printf("KTH_PLAY_PWR_USER_DEF\n");
#if USER_INEAR_DETECT_EN			//出耳 触摸失效
		if(sys_cb.ruer_close_touch_flag){
				break;
		}
#endif
	
#if OUTBOX_NO_SIRI
		if(sys_cb.outbox_siri_cnt > 0)
		{
			break;
		}
#endif
#if UI_FUNCTION_EN	//三击进入SIRI
		if(bt_get_siri_status() == 0)
		{
			user_def_key_msg(2); //语音助手
		}
		else
		{
			bt_music_pause();  //退出语音助手
		}
#else
        user_def_key_msg(xcfg_cb.user_def_kt_sel);
#endif

        break;

    ///四击按键处理
    case KFO_PLAY_USER_DEF:
    case KFO_PLAY_PWR_USER_DEF:
#if USER_INEAR_DETECT_EN			//出耳 触摸失效
		if(sys_cb.ruer_close_touch_flag){
				break;
		}
#endif		
        user_def_key_msg(xcfg_cb.user_def_kfour_sel);
        break;

    ///五击按键处理
    case KFI_PLAY_USER_DEF:
    case KFI_PLAY_PWR_USER_DEF:
#if USER_INEAR_DETECT_EN			//出耳 触摸失效
		if(sys_cb.ruer_close_touch_flag){
				break;
		}
#endif	
#if DUT_MODE_EN
//				if(!bt_nor_is_connected())		//	未跟手机连接，左右耳都可以进入DUT模式
//				{
//					if(func_cb.sta != FUNC_BT_DUT){
//						printf("FUNC_BT_DUT\n");
//						sys_cb.discon_reason = 0;	//不同步关机
//						bt_clr_all_link_info();
//						func_cb.sta = FUNC_BT_DUT;
//					}
//				}
#else		//DUT_FUNCTION_EN
        if (xcfg_cb.user_def_kfive_sel) {
            user_def_key_msg(xcfg_cb.user_def_kfive_sel);
        }
#endif
        break;

    case KU_SIRI:
    case KL_SIRI:
    case KL_HOME:
        if (bt_nor_is_connected()) {
            bt_hfp_siri_switch();
        }
        break;

    case KU_HOME:
        bt_hid_consumer(HID_KEY_IOS_HOME);
        break;

//    case KL_PLAY:
//        if (xcfg_cb.bt_key_discon_en) {
//            bt_disconnect(0);
//            break;
//        }

    case KU_PREV_VOL_DOWN:
    case KU_PREV_VOL_UP:
    case KL_VOL_DOWN_PREV:
    case KL_VOL_UP_PREV:
    case KU_PREV:
        //bt_music_prev();
        sys_cb.key2unmute_cnt = 15 * sys_cb.mute;
        break;

    case KU_NEXT:
    case KU_NEXT_VOL_UP:
    case KU_NEXT_VOL_DOWN:
    case KL_VOL_UP_NEXT:
    case KL_VOL_DOWN_NEXT:
        //bt_music_next();
        sys_cb.key2unmute_cnt = 15 * sys_cb.mute;
        break;
    case KL_PREV:
        bt_music_rewind();
        break;
    case KLU_PREV:
        bsp_clr_mute_sta();
        bt_music_rewind_end();
        break;
    case KL_NEXT:
        bt_music_fast_forward();
        break;
    case KLU_NEXT:
        bsp_clr_mute_sta();
        bt_music_fast_forward_end();
        break;

    case KD_MODE:
    case KD_MODE_PWR:
        bt_tws_pair_mode(5);
        break;

    case KD_HSF:
        if (bt_nor_is_connected()) {
            bt_call_redial_last_number();           //回拨电话
            func_bt_mp3_res_play(RES_BUF_REDIALING_MP3, RES_LEN_REDIALING_MP3);
        }
        break;

#if BT_HID_MANU_EN
    case KD_NEXT_VOL_UP:
    case KD_PREV_VOL_UP:
    case KD_VOL_UP_NEXT:
    case KD_VOL_UP_PREV:
    case KD_VOL_UP:
        bt_hid_vol_msg(3);
        break;

    case KD_PREV_VOL_DOWN:
    case KD_NEXT_VOL_DOWN:
    case KD_VOL_DOWN_PREV:
    case KD_VOL_DOWN_NEXT:
    case KD_VOL_DOWN:
        bt_hid_vol_msg(1);
        break;
#endif

    case MSG_SYS_1S:
		printf("status: %d  mater: %d pwroff_time : %d vbat : %d\n",bt_get_disp_status(),!bt_tws_is_slave(),sys_cb.pwr_off_timeout_cnt,sys_cb.vbat);
		printf("tws_left_channel : %d \n",sys_cb.tws_left_channel);
#if GT_TEST_EN
//			if(IS_PWRKEY_PRESS()){
//				gt_touch_in_spp_tx();
//			}
#endif
#if WL_TIME_OUT_PWROFF
		if(bt_nor_is_connected())
		{
		sys_cb.pwr_off_timeout_cnt = POWEROFF_TIME_VALUE;
#endif
		}
        bt_hfp_report_bat();
#if BT_TWS_DBG_EN
        bt_tws_report_dgb();
#endif
#if PLXC_UP_EN
		if(sys_cb.i2c_flag == 2)
		{
			//printf("pxlc\n");
			rled_on();
			led_on();
		}
#endif		

#if DUT_MODE_EN			//DUT模式下，红灯常亮
		if(func_cb.sta == FUNC_BT_DUT)
		{
			bt_set_scan(3);
			rled_on();
			led_off();
#if CLICK_ENTER_DUT				//TWS从机超距在过5分钟关机
		sys_cb.pwr_off_timeout_cnt = POWEROFF_TIME_VALUE;
#endif
		}
#endif
#if USER_INEAR_DETECT_EN
		if(!bt_nor_is_connected())			//未连接手机，触发都有效
		{
			sys_cb.ruer_close_touch_flag = 0;
		}
		if(!bt_tws_is_playing())
		{
			if(bt_tws_is_connected()){
				if(sys_cb.outear_pause != 2 && !sys_cb.loc_ear_sta) // 出耳暂停得
				{
					sys_cb.first_connect_ruer = 1;
				}
				if(!sys_cb.loc_ear_sta && !sys_cb.rem_ear_sta)
				{
					sys_cb.first_connect_ruer = 1;
				}
			}
			else
			{
				if(!sys_cb.loc_ear_sta && !sys_cb.outear_pause)			//单耳情况下
				{
					sys_cb.first_connect_ruer = 1;
				}
			}
		}
		else
		{
			sys_cb.first_connect_ruer = 0;
		}


#endif

#if NO_CONNECT_PHONE_SLAVER_NO_SLEEP			// 未连接手机，从机不休眠，防止未连接状态指示灯不同步
				if(!bt_nor_is_connected()){ 				//避免从机休眠,导致关机
					reset_sleep_delay();
				}
#endif
        break;

    case EVT_A2DP_MUSIC_PLAY:
#if  BT_A2DP_RECORD_DEVICE_VOL
        if (sys_cb.restore_dev_vol & BIT(7))
        {
            msg_enqueue(EVT_A2DP_SET_VOL);
            bt_tws_vol_change();
            sys_cb.restore_dev_vol = 0;
        }
#endif
#if NO_CONNECT_MP3_EN
	sys_cb.delay_connect_flag = 0;
#endif
		if(CHARGE_INBOX())
		{
			//printf("EVT_A2DP_MUSIC_PLAY ******************\n");
			break;
		}
        if (!sbc_is_bypass()) {

			//printf("EVT_A2DP_MUSIC_PLAY 				 1\n");
            dac_fade_in();
        }
        if (f_bt.pp_2_unmute) {
			//printf("EVT_A2DP_MUSIC_PLAY                  2\n");
            f_bt.pp_2_unmute = 0;
            bsp_clr_mute_sta();
        }
		sys_cb.first_connect_ruer = 0;
 		sys_cb.outear_pause = 0;
        break;

    case EVT_A2DP_MUSIC_STOP:
#if NO_CONNECT_MP3_EN		
	if (sys_cb.delay_connect_flag) {
        sys_cb.delay_connect_flag = 1;
        f_bt.warning_status |= BT_WARN_CON;
		printf("EVT_A2DP_MUSIC_STOP     connected -----------------1\n");
    }
#endif
	
		if(CHARGE_INBOX())
		{

			//printf("EVT_A2DP_MUSIC_STOP1111111111111111111\n");
			break;
		}
        if (!sbc_is_bypass() && !bsp_tws_sync_is_play()) {
			//printf("EVT_A2DP_MUSIC_STOP 				 3333\n");
            dac_fade_out();
        }
        break;

    case EVT_KEY_2_UNMUTE:
        bsp_clr_mute_sta();
        break;

#if BT_REC_EN
    case KU_REC:
        if ((!dev_is_online(DEV_SDCARD)) && (!dev_is_online(DEV_UDISK) && (!dev_is_online(DEV_SDCARD1)))) {
            break;
        }
        if (bt_is_connected()) {
            sfunc_record();
    #if REC_AUTO_PLAY
            if (rec_cb.flag_play) {
                rec_cb.flag_play = 0;
                bt_audio_bypass();
                sfunc_record_play();
                bt_audio_enable();
                dac_fade_in();
            }
    #endif
        }
        break;
#endif // BT_REC_EN

    default:
        func_message(msg);
        break;
    }
}

AT(.text.func.btring.msg)
void sfunc_bt_ring_message(u16 msg)
{
    switch (msg) {
    case KU_HSF:                //接听
    case KU_PLAY_USER_DEF:
    case KU_PLAY_PWR_USER_DEF:
#if USER_INEAR_DETECT_EN			//出耳 触摸失效
		if(sys_cb.ruer_close_touch_flag){
				break;
		}
#endif		
		//printf("KU_PLAY_PWR_USER_DEF\n");
        //bsp_tws_res_music_play(TWS_RES_CALL_HANGUP);
        //bt_call_answer_incoming();
        break;

    //case KL_PLAY_PWR_USER_DEF:
    case EVT_LONGPRESS_1_5S:
        //PWRKEY松开前不产生KHL_PLAY_PWR消息。按键松开自动清此标志。
        sys_cb.poweron_flag = 0;
    case KL_PLAY_USER_DEF:
//        if (user_def_key_msg(xcfg_cb.user_def_kl_sel)) {
//            break;
//        }
    case KL_HSF:
#if USER_INEAR_DETECT_EN			//出耳 触摸失效
		if(sys_cb.ruer_close_touch_flag){
				break;
		}
#endif		
#if OUTBOX_NO_SIRI
		if(sys_cb.outbox_siri_cnt > 0)
		{
			break;
		}
#endif

		//printf("KL_PLAY_PWR_USER_DEF\n");
        //bsp_tws_res_music_play(TWS_RES_CALL_REJECT);
        bt_call_terminate();    //拒接
        break;

    case KLH_PLAY_PWR_USER_DEF:
        //ring不响应关机消息，解决关机时间1.5时长按拒接偶尔触发关机的问题。
        break;

    case KD_PLAY_USER_DEF:
    case KD_PLAY_PWR_USER_DEF:
#if USER_INEAR_DETECT_EN			//出耳 触摸失效
		if(sys_cb.ruer_close_touch_flag){
			break;
		}
#endif
#if OUTBOX_NO_SIRI
		if(sys_cb.outbox_siri_cnt > 0)
		{
			break;
		}
#endif

	//printf("KD_PLAY_PWR_USER_DEF\n");
#if UI_FUNCTION_EN
		bt_call_answer_incoming();		//双击来电来电接听
#else

        if (user_def_key_msg(xcfg_cb.user_def_kd_sel)) {
            break;
        }
        bsp_tws_res_music_play(TWS_RES_CALL_REJECT);
        bt_call_terminate();    //拒接
#endif
        break;

    ///三击按键处理
    case KTH_PLAY_USER_DEF:
    case KTH_PLAY_PWR_USER_DEF:
        user_def_key_msg(xcfg_cb.user_def_kt_sel);
        break;

	    ///四击按键处理
    case KFO_PLAY_USER_DEF:
    case KFO_PLAY_PWR_USER_DEF:
        user_def_key_msg(xcfg_cb.user_def_kfour_sel);
        break;

    ///五击按键处理
    case KFI_PLAY_USER_DEF:
    case KFI_PLAY_PWR_USER_DEF:
        user_def_key_msg(xcfg_cb.user_def_kfive_sel);
        break;

    case MSG_SYS_1S:
        bt_hfp_report_bat();
		
#if 0//USER_INEAR_DETECT_EN				//用户自动校准通话和听筒状态，风险在于手动从手机切换，会自动切回来
#if FACTROY_TEST_EN
		if(sys_cb.ruer_cmd_open)
		{
			break;
		}
#endif
		if(sys_cb.first_connect_ruer)
		{
			break;
		}

		if(sys_cb.loc_ear_sta && sys_cb.rem_ear_sta)
		{
			if(sco_is_connected())
			{
				if(!bt_tws_is_slave())
					bt_call_private_switch();
			}
		}

		if(!sys_cb.loc_ear_sta || !sys_cb.rem_ear_sta)
		{
			if(!sco_is_connected())
			{
				if(!bt_tws_is_slave())
					bt_call_private_switch();
			}
		}
#endif
        break;

        //屏蔽来电响铃调音量
    case KL_PREV_VOL_UP:
    case KH_PREV_VOL_UP:
    case KL_NEXT_VOL_UP:
    case KH_NEXT_VOL_UP:
    case KLU_NEXT_VOL_UP:
    case KU_VOL_UP_PREV:
    case KU_VOL_UP_NEXT:
    case KU_VOL_UP:
    case KL_VOL_UP:
    case KH_VOL_UP:
    case KLU_VOL_UP:

    case KL_PREV_VOL_DOWN:
    case KH_PREV_VOL_DOWN:
    case KL_NEXT_VOL_DOWN:
    case KH_NEXT_VOL_DOWN:
    case KU_VOL_DOWN_PREV:
    case KU_VOL_DOWN_NEXT:
    case KU_VOL_DOWN:
    case KL_VOL_DOWN:
    case KH_VOL_DOWN:

    case KU_VOL_UP_DOWN:
    case KL_VOL_UP_DOWN:
    case KH_VOL_UP_DOWN:
        break;

    default:
        func_message(msg);
        break;
    }
}

void sfunc_bt_call_message(u16 msg)
{
    u8 call_status;

#if ENTER_RING_STOP_EN
        if(sys_cb.dac_fade_sta && tick_check_expire(sys_cb.dac_fade_tick,1000)){
            sys_cb.dac_fade_sta = 0;
            //dac_fade_in();
			bsp_sys_unmute();
        }
#endif

    switch (msg) {
    case KU_HOME:
    case KL_HOME:
        if (bt_get_siri_status()) {
            bt_call_terminate();                        //结束SIRI
        }
        break;

    ///挂断当前通话，或者结束当前通话并接听第2路通话
    case KU_HSF:
    case KU_PLAY_USER_DEF:
    case KU_PLAY_PWR_USER_DEF:
#if USER_INEAR_DETECT_EN			//出耳 触摸失效
		if(sys_cb.ruer_close_touch_flag){
				break;
		}
#endif		
        call_status = bt_get_call_indicate();
        break;

    ///拒接第2路通话, 或私密接听切换
	case EVT_LONGPRESS_1_5S:
		
		printf("EVT_LONGPRESS_1_5S \n");
    //case KL_PLAY_PWR_USER_DEF:
        sys_cb.poweron_flag = 0;                        //PWRKEY松开前不产生KHL_PLAY_PWR消息。按键松开自动清此标志。
    case KL_PLAY_USER_DEF:
//        if (user_def_key_msg(xcfg_cb.user_def_kl_sel)) {
//            break;
//        }
    case KL_HSF:
#if OUTBOX_NO_SIRI
		if(sys_cb.outbox_siri_cnt > 0)
		{
			break;
		}
#endif		
        call_status = bt_get_call_indicate();
#if USER_INEAR_DETECT_EN			//出耳 触摸失效
			if(sys_cb.ruer_close_touch_flag){
					break;
			}
#endif

	
#if CALL_3WAY_SLAVER_NO_END				//三方通话从机不能挂断
//		if(call_status == BT_CALL_3WAY_CALL && bt_tws_is_slave())
//		{
//			call_status = BT_CALL_INCOMING;
//		}
#endif
        if(call_status == BT_CALL_INCOMING) {
            bt_call_terminate();                        //拒接第2路通话
        }
        break;

    ///保持当前通话并接通第2路通话，或者2路通话切换
    case KD_PLAY_PWR_USER_DEF:
    case KD_PLAY_USER_DEF:
		printf("KD_PLAY_PWR_USER_DEF \n");
//        if (user_def_key_msg(xcfg_cb.user_def_kd_sel)) {
//            break;
//        }
    case KD_HSF:
#if OUTBOX_NO_SIRI
		if(sys_cb.outbox_siri_cnt > 0)
		{
			break;
		}
#endif		
        call_status = bt_get_call_indicate();
#if USER_INEAR_DETECT_EN			//出耳 触摸失效
			if(sys_cb.ruer_close_touch_flag){
					break;
			}
#endif

	
#if UI_FUNCTION_EN
		if(call_status == BT_CALL_INCOMING) {
			bt_call_answer_incoming();					//接听第2路通话
		}
		else
		{
			bt_call_terminate();						//挂断通话
			f_bt.disp_status = 0xff;
		}
#else
        if(call_status == BT_CALL_INCOMING) {
            bt_call_answer_incoming();                  //接听第2路通话
        } else if(call_status == BT_CALL_3WAY_CALL) {
            bt_call_swap();                             //切换两路通话
        }
#endif
        break;

    ///三击按键处理
    case KTH_PLAY_USER_DEF:
    case KTH_PLAY_PWR_USER_DEF:
        //user_def_key_msg(xcfg_cb.user_def_kt_sel);
        break;

    ///四击按键处理
    case KFO_PLAY_USER_DEF:
    case KFO_PLAY_PWR_USER_DEF:
        user_def_key_msg(xcfg_cb.user_def_kfour_sel);
        break;

    ///五击按键处理
    case KFI_PLAY_USER_DEF:
    case KFI_PLAY_PWR_USER_DEF:
        user_def_key_msg(xcfg_cb.user_def_kfive_sel);
        break;

    case EVT_HFP_SET_VOL:
        if(sys_cb.incall_flag & INCALL_FLAG_SCO) {
            bsp_change_volume(bsp_bt_get_hfp_vol(sys_cb.hfp_vol));
            param_hfp_vol_write();
            sys_cb.cm_times = 0;
            sys_cb.cm_vol_change = 1;
        }
        break;

    case EVT_A2DP_MUSIC_PLAY:
        dac_fade_in();
        break;
	case MSG_SYS_500MS:
		
#if USER_INEAR_DETECT_EN
			if(sys_cb.ruer_call_cnt>0)
			{
				sys_cb.ruer_call_cnt--;
					if(sys_cb.ruer_call_cnt ==1)
					{
						if (!sys_cb.loc_ear_sta && sys_cb.rem_ear_sta && bt_tws_is_slave()) //如果两个耳机都出耳，本地耳机入耳
			            {
			                //sys_cb.private_call_switch_time = 15;
			                printf("ruer  bt_tws_switch     1\n");
			                bt_tws_switch();
			            }
					}
					if(sco_is_connected())
					{
						sys_cb.ruer_call_cnt = 3;
					}
					if(sys_cb.ruer_call_cnt ==11)
					{
						if(!sys_cb.loc_ear_sta && !sys_cb.rem_ear_sta &&(sys_cb.tws_left_channel == 1 ))
						{
							if(!sco_is_connected())
							{
								
								printf("bt_call_private_switch	3\n");
								bt_call_private_switch();
								sys_cb.ruer_call_cnt = 3;
							}
						}
					}else if(sys_cb.ruer_call_cnt>3 && sys_cb.ruer_call_cnt <11)
					{
						if((!sys_cb.loc_ear_sta))
						{
							if(!sco_is_connected())
							{
								
								printf("bt_call_private_switch	5\n");
								bt_call_private_switch();
								sys_cb.ruer_call_cnt = 3;
							}
						}

					}
					
			}
			else if(sys_cb.outear_call_cnt>0)
				{
					sys_cb.outear_call_cnt --;
					if(sys_cb.outear_call_cnt ==1)
					{
						if (sys_cb.loc_ear_sta && !sys_cb.rem_ear_sta && !bt_tws_is_slave()) //如果两个耳机都入耳，本地耳机入耳
			            {
			                printf("outer  bt_tws_switch     2\n");
			                bt_tws_switch();
			            }
					}
					
					if(!sco_is_connected())
					{
						sys_cb.outear_call_cnt = 3;
					}
					

					
					if(sys_cb.outear_call_cnt ==11){
						if(sys_cb.loc_ear_sta && sys_cb.rem_ear_sta)
						{
							if(sco_is_connected() &&bt_tws_is_connected()&&sys_cb.tws_left_channel)
							{
								//printf("out ear 3  cnt	bt_call_private_switch \n ");
								
								printf("bt_call_private_switch	2\n");
								bt_call_private_switch();
								sys_cb.outear_call_cnt = 3;
							}
						}	
					}
					else if(sys_cb.outear_call_cnt>3 && sys_cb.outear_call_cnt <=10)
					{
						if(sys_cb.loc_ear_sta && sys_cb.rem_ear_sta)
						{
							if(sco_is_connected() &&bt_tws_is_connected())
							{
								//printf("out ear 3  cnt	bt_call_private_switch \n ");
								printf("bt_call_private_switch	4\n");
								bt_call_private_switch();
								sys_cb.outear_call_cnt = 3;
							}
						}
					}
				}
	
#endif
		break;
		

    case MSG_SYS_1S:
        bt_hfp_report_bat();
		
	//printf("status: %d  mater: %d  vbat:%d conn_phone:%d	pwroff_time : %d",bt_get_disp_status(),!bt_tws_is_slave(),sys_cb.vbat,bt_nor_is_connected(),sys_cb.pwr_off_timeout_cnt);
//	printf("\n");
	//printf("loc ear ---- %d  rem_ear ---- %d  ruer_touch -----  %d  ruer_cnt ---- %d	outear cnt ---- %d sco_status ---- %d\n",sys_cb.loc_ear_sta,sys_cb.rem_ear_sta,sys_cb.ruer_close_touch_flag,sys_cb.ruer_call_cnt,sys_cb.outear_call_cnt,sco_is_connected());
//	printf("\n");
#if 0//USER_INEAR_DETECT_EN
		if(sys_cb.ruer_call_cnt>0)
		{
			sys_cb.ruer_call_cnt--;
				if(sco_is_connected())
				{
					sys_cb.ruer_call_cnt = 0;
				}
				if(sys_cb.ruer_call_cnt >4)
				{
					if((!sys_cb.loc_ear_sta && !sys_cb.rem_ear_sta) &&!bt_tws_is_slave() )
					{
						bt_call_private_switch();
					}
				}
		

				
				//if(!sys_cb.loc_ear_sta || !sys_cb.rem_ear_sta)
				
				if((!sys_cb.loc_ear_sta || !sys_cb.rem_ear_sta) && bt_tws_is_slave())
				{
					if(!sco_is_connected())
					{
							printf(" ruer -----------------bt_call_private_switch \n");
							bt_call_private_switch();
					}
				}

		}
		else if(sys_cb.outear_call_cnt>0)
			{
				sys_cb.outear_call_cnt --;
				if(!sco_is_connected())
				{
					sys_cb.outear_call_cnt = 0;
				}
				if(sys_cb.loc_ear_sta && sys_cb.rem_ear_sta)
				{
					if(sco_is_connected() &&bt_tws_is_connected()&& !bt_tws_is_slave())
					{
						printf("out ear 3  cnt  bt_call_private_switch \n ");
						bt_call_private_switch();
					}
				}
			}
		
#endif
#if 0//USER_INEAR_DETECT_EN				//用户自动校准通话和听筒状态，风险在于手动从手机切换，会自动切回来
#if FACTROY_TEST_EN
		if(sys_cb.ruer_cmd_open)
		{
			break;
		}
#endif

		if(sys_cb.ruer_ctl_flag == 1)
		{
			sys_cb.ruer_ctl_flag = 2;
			if(sys_cb.loc_ear_sta && sys_cb.rem_ear_sta)
			{
				if(sco_is_connected())
				{
					if(!bt_tws_is_slave())
						bt_call_private_switch();
				}
			}

			if(!sys_cb.loc_ear_sta || !sys_cb.rem_ear_sta)
			{
				if(!sco_is_connected())
				{
					if(!bt_tws_is_slave())
						bt_call_private_switch();
				}
			}
			break;
		}
#endif


        break;

    default:
        func_message(msg);
        break;
    }
}

AT(.text.func.bt.msg)
void func_bthid_message(u16 msg)
{
    switch (msg) {
    case KU_PLAY:
    case KU_PLAY_USER_DEF:
    case KU_PLAY_PWR_USER_DEF:
        if (bt_get_status() < BT_STA_DISCONNECTING) {
            bt_connect();
        } else {
            bsp_bt_hid_photo(HID_KEY_VOL_UP);
            mp3_res_play(RES_BUF_TAKE_PHOTO_MP3,RES_LEN_TAKE_PHOTO_MP3);
        }
        break;

    case KL_PLAY:
        bt_disconnect(0);
        break;

    default:
        func_message(msg);
        break;
    }
}
