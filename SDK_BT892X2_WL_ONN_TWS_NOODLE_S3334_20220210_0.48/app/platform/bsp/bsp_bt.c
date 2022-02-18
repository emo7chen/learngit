#include "include.h"
#include "api.h"


void bt_new_name_init(void);
void lock_code_charge(void);
void unlock_code_charge(void);

/********************************************************/
extern void bt_tws_set_operation(uint8_t *cmd);

void bt_set_sys_clk(uint8_t level)
{
    if (level) {
        sys_clk_req(2, SYS_120M);
    } else {
        sys_clk_free(2);
    }
}

void bsp_bt_init(void)
{
    //更新配置工具的设置
    cfg_bt_rf_def_txpwr = xcfg_cb.bt_rf_pwrdec;
    cfg_bt_page_txpwr = xcfg_cb.bt_rf_page_pwrdec;
    cfg_ble_page_txpwr = xcfg_cb.ble_rf_page_pwrdec;
    cfg_ble_page_rssi_thr = xcfg_cb.ble_page_rssi_thr;

    cfg_bt_support_profile = (PROF_A2DP*BT_A2DP_EN*xcfg_cb.bt_a2dp_en) | (PROF_HFP*BT_HFP_EN*xcfg_cb.bt_sco_en)\
                            | (PROF_SPP*BT_SPP_EN*(xcfg_cb.bt_spp_en||xcfg_cb.eq_dgb_spp_en)) | (PROF_HID*BT_HID_EN*xcfg_cb.bt_hid_en) | (PROF_HSP*BT_HSP_EN*xcfg_cb.bt_sco_en);
#if BT_A2DP_VOL_CTRL_EN
    if(!xcfg_cb.bt_a2dp_vol_ctrl_en) {
        cfg_bt_a2dp_feature &= ~A2DP_AVRCP_VOL_CTRL;
    } else {
        cfg_bt_a2dp_feature |= A2DP_AVRCP_VOL_CTRL;
    }
#endif

#if BT_A2DP_AVRCP_PLAY_STATUS_EN
    cfg_bt_a2dp_feature |= A2DP_AVRCP_PLAY_STATUS;
#endif

    cfg_bt_dual_mode = BT_DUAL_MODE_EN * xcfg_cb.ble_en;
    cfg_bt_max_acl_link = BT_2ACL_EN * xcfg_cb.bt_2acl_en + 1;
#if BT_TWS_EN
    cfg_bt_tws_mode = BT_TWS_EN * xcfg_cb.bt_tws_en;
    if(xcfg_cb.bt_tws_en == 0) {
        cfg_bt_tws_feat      = 0;
        cfg_bt_tws_pair_mode = 0;
    } else {
        cfg_bt_tws_pair_mode &= ~TWS_PAIR_OP_MASK;
        cfg_bt_tws_pair_mode |= xcfg_cb.bt_tws_pair_mode & TWS_PAIR_OP_MASK;
#if BT_TWS_PAIR_BONDING_EN
        if(xcfg_cb.bt_tws_pair_bonding_en) {
            cfg_bt_tws_pair_mode |= TWS_PAIR_MS_BONDING;
            cfg_bt_tws_feat |= TWS_FEAT_MS_BONDING;
        } else {
            cfg_bt_tws_pair_mode &= ~TWS_PAIR_MS_BONDING;
        }
#endif

#if FUNC_SINGLE_TEST_MODE
		if(func_cb.sta == FUNC_SINGLE ||func_cb.sta == FUNC_BT_DUT ){
			 sys_cb.into_pair_flag = 1;
       		 cfg_bt_tws_pair_mode = 5;
 	  	 }
#endif
#if BT_TWS_MS_SWITCH_EN
        if(xcfg_cb.bt_tswi_en) {
            cfg_bt_tws_feat |= TWS_FEAT_MS_SWITCH;
        } else {
            cfg_bt_tws_feat &= ~TWS_FEAT_MS_SWITCH;
        }
#endif
		cfg_bt_tws_feat |= TWS_FEAT_PAIR_NOR_LINKINFO;
        if(xcfg_cb.bt_tws_lr_mode > 8) {//开机时PWRKEY可能按住，先不检测
            tws_lr_xcfg_sel();
        }
    }
#endif // BT_TWS_EN

#if BT_HFP_EN
    if(!xcfg_cb.bt_hfp_ring_number_en) {
        cfg_bt_hfp_feature &= ~HFP_RING_NUMBER_EN;
    }
#endif

#if USER_INEAR_DETECT_EN
    sys_cb.loc_ear_sta = 0x1;           //有入耳检测时，改为实际状态（0=戴入，1=取下）
    sys_cb.rem_ear_sta = 0x1;           //有入耳检测时，改为实际状态（0=戴入，1=取下）
#endif // USER_INEAR_DETECT_EN

    sys_cb.discon_reason = 0xff;

#if BT_FCC_TEST_EN
    bt_fcc_init();
#endif

#if FUNC_BTHID_EN
    if (is_bthid_mode()) {
        cfg_bt_support_profile = PROF_HID;
        cfg_bt_max_acl_link = 1;
        cfg_bt_dual_mode = 0;
        cfg_bt_tws_mode = 0;
    }
#endif // FUNC_BTHID_EN

    memset(&f_bt, 0, sizeof(func_bt_t));
    f_bt.disp_status = 0xfe;
    f_bt.need_pairing = 1;  //开机若回连不成功，需要播报pairing
    if (!is_bthid_mode()) {
        f_bt.hid_menu_flag = 1;
    }

    bt_setup();
}

void bsp_bt_close(void)
{
}

void tws_user_key_process(uint32_t *opcode)
{
//*opcode
	switch (*opcode)
	{
#if USER_INEAR_DETECT_EN && FIRST_RUER_NO_RESPON
		case TWS_USER_TELL_OUTER_STA:
			printf("TWS_USER_TELL_OUTER_STA \n");
			//sys_cb.first_connect_ruer = 0;
			break;
		case TWS_USER_TELL_RUER_STA:
			printf("TWS_USER_TELL_RUER_STA \n");
			//sys_cb.first_connect_ruer = 1;
			break;
#endif
#if CONNECT_TWS_1S_LED
		case TWS_USER_KEY_TWS_CONNECT_1S:
			{
				/*主机接受到从机的准备完成的消息，开始亮1s灯*/
				/*ps由于存在先后开机，不同步开机的情况，耳机在func_run之前就已经tws连接成功
				导致亮1s灯丢失(进入func_bt的时候LED会初始化),此消息等待从机进入func_bt之后再亮灯*/
				printf("TWS_CONNECT_START\n");
				led_connect_1s();
				sys_cb.close_led = 1;
				sys_cb.tws_conn_1s_cnt = 1;
			}
			break;
#endif
#if CONNECT_PHONE_1S_LED
		case TWS_USER_KEY_CONNECT_PHONE_1S:
			sys_cb.close_led = 1;
			sys_cb.connect_flush_cnt = 1;
			break;
#endif
#if CHARGE_INBOX_CHECK
		case TWS_USER_FLUSH_STA:
			if(!CHARGE_INBOX()){		//如果主机在仓里，刷新状态让副机刷新灯
				f_bt.disp_status = 0xff;	//刷新显示
				break;
			}
			if(sys_cb.led_master_sta == 0x21){
				bt_tws_user_key(0x21);
			}
			if(sys_cb.led_master_sta == 0x22){
				bt_tws_user_key(0x22);
			}
			if(sys_cb.led_master_sta == 0x23){
				bt_tws_user_key(0x23);
			}
			if(sys_cb.led_master_sta == 0x24){
				bt_tws_user_key(0x24);
			}
			if(sys_cb.led_master_sta == 0x25){
				bt_tws_user_key(0x25);
			}
			if(sys_cb.led_master_sta == 0x26){
				bt_tws_user_key(0x26);
			}
			else
			{
#if CLEAR_INFO_SLAVER_LED_ON
				//耳机在仓里清除配对后，重新白灯亮，拿出从机，从机灯闪烁异常
				if(!bt_tws_is_slave()&&CHARGE_DC_IN()) //如果主机在仓里，进行TWS连接需要告诉从机当前状态
				{
					bt_tws_user_key(0x900|bt_get_disp_status());
				}
#endif
			}
			break;
		case 0x21:
			//printf("slaver outbox  led_bt_idle \n");
			sys_cb.slaver_flush_led_flag = 1;
 			led_bt_idle();
			break;
		case 0x22:
			//printf("slaver outbox  led_bt_reconnect \n");
			sys_cb.slaver_flush_led_flag = 1;
 			led_bt_reconnect();
			break;
		case 0x23:
			sys_cb.slaver_flush_led_flag = 1;
			//printf("slaver outbox  led_bt_connected \n");
			led_bt_connected();
			break;
		case 0x24:
			sys_cb.slaver_flush_led_flag = 1;
			//printf("slaver outbox  led_bt_ring \n");
 			led_bt_ring();
			break;
		case 0x25:
			sys_cb.slaver_flush_led_flag = 1;
			//printf("slaver outbox  led_bt_play \n");
 			led_bt_play();
			break;
		case 0x26:
			sys_cb.slaver_flush_led_flag = 1;
			//printf("slaver outbox  led_bt_call \n");
			led_bt_call();
			break;
		case 0x27:
			printf("receive 0x27\n");
			msg_enqueue(EVT_ENTER_SLEEP_MODE);
			break;
		case 0x28:
			break;
#if USER_INEAR_DETECT_EN
		case 0x29:
			sys_cb.first_connect_ruer = 1;
			break;
		case 0x30:
			sys_cb.outear_pause = 2;
			break;
#endif
#if SCO_WAIT_ON_EN
        case TWS_USER_KEY_TWS_SCO_WAIT:
             msg_enqueue(EVT_SYNC_SCO_WAIT);
            break;
#endif
#if CLEAR_INFO_SLAVER_LED_ON
//   清除配对记录后，在仓里先拿从机出来，不知道不知道主机得指示灯状态，所以需要先获取主机的当前状态
		case 0x902:
			led_bt_idle();
			break;
		case 0x905:
			led_bt_reconnect();
			break;
		case 0x906:
			led_bt_connected();
			break;
		case 0x907:
			led_bt_play();
			break;
		case 0x908:
			led_bt_ring();
			break;
		case 0x909:
			led_bt_call();
			break;
#endif
#endif
		default:
#if GT_TEST_EN
			if(*opcode == TWS_USER_GET_BAT)
			{
			    uint hfp_get_bat_level_ex(void);
				sys_cb.vbat_local = (hfp_get_bat_level_ex());
				bt_tws_user_key(0xC0|(sys_cb.vbat_local/10));
			}
			else if((*opcode >= 0xC0) && (*opcode <= 0xCA))
			{
				*opcode &= 0x0f;

				gt_test_tws_process((*opcode)*10);
			}
			else if(*opcode == TWS_USER_GET_FW)
			{

				bt_tws_user_key(0x700 | sys_cb.fw_local);
			}
			else if(*opcode >= 0x700 && (*opcode <= 0x799))
			{
				*opcode &= 0xff;
				gt_fw_tws_process(*opcode);
			}
			else if(*opcode == TWS_USER_GET_HW)
			{
				bt_tws_user_key(0x800 | sys_cb.hw_local);
			}
			else if(*opcode >= 0x800 && (*opcode <= 0x899))
			{
				*opcode &= 0xff;
				gt_hw_tws_process(*opcode);
			}
			else if(*opcode == TWS_USER_GET_TOUCH)
			{
				bt_tws_user_key(0x900|sys_cb.touch_flag);
			}
			else if(*opcode >= 0x900 && (*opcode <= 0x999))
			{
				*opcode &= 0xff;
				gt_touch_tws_process(*opcode);
			}
#if CHUANYUN_MODE
			else if(*opcode == TWS_USER_GET_SHIP)
			{
				sys_cb.spp_ship_mode_flag = 1;
				bt_tws_user_key(0xA01);			//从机收到消息，给主机回消息
				msg_enqueue(EVT_SHIPMODE);	//进入船运模式;
			}
			else if(*opcode >= 0XA01)		//主机收到从机船运消息，给从机返回
			{
				*opcode &= 0xff;
				sys_cb.spp_ship_mode_flag = 1;
				gt_ship_tws_process(*opcode);
				msg_enqueue(EVT_SHIPMODE);	//进入船运模式
			}
#endif
#endif
            break;
   }

}

void tws_warning_process(uint32_t *opcode)
{
	uint8_t type = (*opcode & 0xff00) >> 8;
	uint8_t res_num = (*opcode) & 0xff;
	if (type != RES_TYPE_INVALID) {
		bsp_tws_res_slave_music_play(type, res_num);
	}
}

#if BT_PWRKEY_5S_DISCOVER_EN
bool bsp_bt_w4_connect(void)
{
    if (xcfg_cb.bt_pwrkey_nsec_discover) {
        while (sys_cb.pwrkey_5s_check) {            //等待检测结束
            WDT_CLR();
            delay_5ms(2);
        }

        //已检测到长按5S，需要直接进入配对状态。播放PAIRING提示音。
        if (sys_cb.pwrkey_5s_flag) {
			printf("close tws pairig longpress 8s\n");
#if LONGPRESS_8S_SINGLE_MODE
			cfg_bt_tws_pair_mode = 5;
			sys_cb.pwr_off_timeout_cnt = POWEROFF_TIME_VALUE;
#endif
            return false;
        }
    }
    return true;
}

bool bsp_bt_pwrkey5s_check(void)
{
    bool res = !bsp_bt_w4_connect();
    delay_5ms(2);
    return res;
}

void bsp_bt_pwrkey5s_clr(void)
{
    if (!xcfg_cb.bt_pwrkey_nsec_discover) {
        return;
    }
    sys_cb.pwrkey_5s_flag = 0;
}
#endif // BT_PWRKEY_5S_DISCOVER_EN

void bsp_bt_vol_change(void)
{
#if BT_A2DP_VOL_CTRL_EN || BT_TWS_EN
    if((xcfg_cb.bt_a2dp_vol_ctrl_en && (bt_get_status() >= BT_STA_CONNECTED)) || bt_tws_is_connected()) {
#if SDK_VERSION > 0x0100
        bt_tws_vol_change();        //通知TWS音量已调整
#endif
        bt_music_vol_change();      //通知手机音量已调整
    }
#endif
}

uint bsp_bt_get_hfp_vol(uint hfp_vol)
{
    uint vol;
    vol = (hfp_vol + 1) * sys_cb.hfp2sys_mul;
    if (vol > VOL_MAX) {
        vol = VOL_MAX;
    }
    return vol;
}

void bsp_bt_call_volume_msg(u16 msg)
{
    if ((msg == KU_VOL_UP) && (sys_cb.hfp_vol < 15)) {
        sys_cb.hfp_vol++;
    } else if ((msg == KU_VOL_DOWN) && (sys_cb.hfp_vol > 0)) {
        sys_cb.hfp_vol--;
    } else {
        return;
    }
    bt_hfp_set_spk_gain();
    if(sys_cb.incall_flag & INCALL_FLAG_SCO) {
        bsp_change_volume(bsp_bt_get_hfp_vol(sys_cb.hfp_vol));
    }
//    printf("call vol: %d\n", sys_cb.hfp_vol);
}

void bt_emit_notice(uint evt, void *params)
{
    u32 tmp;
    u8 *packet = params;
    u8 opcode = 0;
    u8 scan_status = 0x03;
#if WL_SINGLE_PWR_ON			//单耳开机，回连手机失败后，一直播报pairing问题
	//static u8 tws_connect_fail_flag = 0;
#endif //WL_TIME_OUT_PWROFF
    switch(evt) {
    case BT_NOTICE_INIT_FINISH:
#if BT_TWS_MS_SWITCH_EN
        if(CHARGE_DC_RESET || bt_tws_is_ms_switch()) {
            sys_cb.dc_rst_flag = 1;
            RTCCON &= ~BIT(6);
        }
#endif
#if BT_TWS_EN
        if(xcfg_cb.bt_tws_pair_mode > 1) {
            bt_tws_set_scan(0x03);
        }
#endif
#if USER_TWS_PAIR_MODE
		if(!bt_tws_get_link_info(NULL)&&func_cb.sta !=FUNC_BT_DUT && func_cb.sta!=FUNC_SINGLE){		//如果TWS记录为空，就发起TWS搜索

			if(sys_cb.pwrkey_5s_flag != 1){ 	//检测到长按8S ，不进入TWS配对状态
				sys_cb.tws_pair_led_flag = 1;
				msg_enqueue(EVT_TWS_PAIR);
			}
		}
#endif

        if(cfg_bt_work_mode == MODE_BQB_RF_BREDR) {
            opcode = 1;                     //测试模式，不回连，打开可被发现可被连接
#if BT_PWRKEY_5S_DISCOVER_EN
        } else if(!bsp_bt_w4_connect()) {

            opcode = 1;                     //长按5S开机，不回连，打开可被发现可被连接
#endif
        } else {
            if(bt_nor_get_link_info(NULL)) {
                scan_status = 0x02;         //有回连信息，不开可被发现
            }
        }
#if VUSB_TBOX_QTEST_EN
        qtest_create_env();
#endif
        bt_start_work(opcode, scan_status);
#if LE_WIN10_POPUP
        ble_adv0_set_ctrl(1);				//打开LE广播，可被win10发现
#endif
        break;

    case BT_NOTICE_DISCONNECT:
		printf("BT_NOTICE_DISCONNECT\n");
#if WL_TIME_OUT_PWROFF				//手动与手机断链，5分钟后关机
		sys_cb.pwr_off_timeout_cnt = POWEROFF_TIME_VALUE;
#endif
#if DISCONN_PAIR_CLOSE_MP3					//手动断链已自定义播报
	 	sys_cb.is_need_pairing = 0;
#endif
#if VUSB_TBOX_QTEST_EN
        if(qtest_get_mode()){
            qtest_exit();
             //断开蓝牙连接，默认复位
            if(!qtest_get_pickup_sta()){
                sw_reset_kick(SW_RST_FLAG);
            }else  if (qtest_get_pickup_sta()==3){
                msg_enqueue(EVT_QTEST_PICKUP_PWROFF);
            }
        }
#endif
#if WL_LINKLOSS_EN			//超距时先出发DISSCONNECT  再出发LINKLOSS
		sys_cb.wait_linkloss_cnt = 1;
#else
        f_bt.warning_status |= BT_WARN_DISCON;
#endif
#if LE_WIN10_POPUP
        ble_adv0_set_ctrl(1);				//打开LE广播，可被win10发现
#endif
#if  BT_A2DP_RECORD_DEVICE_VOL
        msg_enqueue(EVT_A2DP_SAVE_DEV_VOL);
#endif
        msg_enqueue(EVT_AUTO_PWFOFF_EN);
#if WL_SYSN_VOL				//连接不同步的手机，音量恢复默认
		sys_cb.vol = SYS_INIT_VOLUME;
		bsp_change_volume(sys_cb.vol);
		bsp_bt_vol_change();
#endif
        pwroff_autoset_flag(AUTOSET_EN);
        delay_5ms(5);
        break;
    case BT_NOTICE_CONNECTED:{
#if TWS_PAIRING_BLEM
		sys_cb.tws_disccon_flag = 0;
		sys_cb.single_linkloss_flag = 0;
#endif
#if WL_SINGLE_PWR_ON
		sys_cb.tws_connect_fail_close_flush = 0;
#endif
#if WL_TIME_OUT_PWROFF				//连接手机成功，清0
		sys_cb.pwr_off_timeout_cnt = 300;
#endif
		printf("BT_NOTICE_CONNECTED\n");
#if DISCONN_PAIR_CLOSE_MP3
		sys_cb.is_need_pairing = 1;
#endif
        f_bt.warning_status |= BT_WARN_CON;
        bt_reset_redial_number(packet[0] & 0x01);
#if LE_WIN10_POPUP
        ble_adv0_set_ctrl(0);				//关闭LE广播
#endif
#if BT_PWRKEY_5S_DISCOVER_EN
        bsp_bt_pwrkey5s_clr();
#endif // BT_PWRKEY_5S_DISCOVER_EN
#if USER_INEAR_DETECT_EN && FIRST_RUER_NO_RESPON
		sys_cb.first_connect_ruer = 1;		//第一次连接标志，用于控制第一次入耳不触发播放音乐
		//bt_tws_user_key(TWS_USER_TELL_RUER_STA);
#endif

#if CONNECT_PHONE_1S_LED
		sys_cb.connect_phone_flag = 1;
#endif
#if WL_LINKLOSS_EN
		sys_cb.link_loss_flag = 0;
		sys_cb.link_loss_cnt = 0;
#endif

        delay_5ms(5);
        pwroff_autoset_flag(AUTOSET_OFF);
		msg_enqueue(EVT_AUTO_PWFOFF_DIS);
    } break;
	  case BT_NOTICE_CONNECT_START:

		  printf("BT_NOTICE_CONNECT_START\n");
	  	break;


   	  case BT_NOTICE_CONNECT_FAIL:
	  		printf("BT_NOTICE_CONNECT_FAIL\n");
#if WL_TIME_OUT_PWROFF				//主机回连手机失败，在过5分钟关机
		sys_cb.pwr_off_timeout_cnt = POWEROFF_TIME_VALUE;
#endif
#if DISCONN_PAIR_CLOSE_MP3
		   sys_cb.is_need_pairing = 1;
#endif
#if MP3_PWRON_FIX_EN
		  sys_cb.tws_conn_flg = 4;
#endif
	        if(bt_is_scan_ctrl()) {
				printf("set can 0x03\n");
	            bt_set_scan(0x03);      //回连失败，打开可被发现可被连接
	        }
#if WL_TIME_OUT_PWROFF				//主机连接手机失败，超时自动关机
			sys_cb.pwr_off_timeout_cnt = POWEROFF_TIME_VALUE;
#endif
#if SINGE_DC_OUT
			if(!bt_tws_is_connected()){
				sys_cb.tws_connect_fail_close_flush = 0;
			}
#endif
	        break;
	  case BT_NOTICE_LOSTCONNECT:
	  		printf("BT_NOTICE_LOSTCONNECT \n");
#if WL_TIME_OUT_PWROFF				//主机超距自动关机，在过5分钟关机
		sys_cb.pwr_off_timeout_cnt = POWEROFF_TIME_VALUE;
#endif
#if WL_LINKLOSS_EN
			sys_cb.link_loss_flag = 1;
			sys_cb.link_loss_cnt = 1;
#endif
#if WL_TIME_OUT_PWROFF				//单耳超距时，从机超时自动关机
			sys_cb.pwr_off_timeout_cnt = POWEROFF_TIME_VALUE;
#endif
	        break;
//    case BT_NOTICE_INCOMING:
//    case BT_NOTICE_RING:
//    case BT_NOTICE_OUTGOING:
//    case BT_NOTICE_CALL:
//        break;

    case BT_NOTICE_SET_SPK_GAIN:
        if(packet[0] != sys_cb.hfp_vol) {
            sys_cb.hfp_vol = packet[0];
            msg_enqueue(EVT_HFP_SET_VOL);
        }
        break;

    case BT_NOTICE_MUSIC_PLAY:
		printf("BT_NOTICE_MUSIC_PLAY\n");
		msg_enqueue(EVT_A2DP_MUSIC_PLAY);
		break;

    case BT_NOTICE_MUSIC_STOP:

		printf("BT_NOTICE_MUSIC_STOP\n");
        if (bt_get_disp_status() > BT_STA_PLAYING) {
            break;
        }

		msg_enqueue(EVT_A2DP_MUSIC_STOP);
        break;

    case BT_NOTICE_MUSIC_CHANGE_VOL:
        if(packet[0] == 0) {
            msg_enqueue(KU_VOL_DOWN);
        } else {
            msg_enqueue(KU_VOL_UP);
        }
        break;
    case BT_NOTICE_MUSIC_CHANGE_DEV:
#if BT_A2DP_VOL_CTRL_EN && BT_A2DP_VOL_CTRL_WITHOUT_KEY
        if( (cfg_bt_a2dp_feature & A2DP_AVRCP_VOL_CTRL)
           && ((packet[1]&BIT(7)) != 0 || (packet[1]&BIT(0)) == 0) ) {      //BIT(7)=是否复位音量, BIT(0)=是否支持音量同步
            if(sys_cb.vol != VOL_MAX) {                                     //手机恢复VOL_MAX
                msg_enqueue(EVT_A2DP_SET_VOL);
                sys_cb.vol = VOL_MAX;
                bt_tws_vol_change();
            }
            printf("max_vol\n");
        }
#endif
#if  BT_A2DP_RECORD_DEVICE_VOL
        if ((packet[1]&BIT(1)) == 0) { // bit(1) ：初次连接配对不用恢复音量
            uint8_t a2dp_get_cur_dev_vol(void);
            uint8_t cur_dev_vol = a2dp_get_cur_dev_vol();
            uint8_t update = false;
            if (cur_dev_vol != 0xff) {
                if(sys_cb.vol != cur_dev_vol) {                                     //恢复音量
                    sys_cb.vol = cur_dev_vol;
                    update = true;
                }
            } else { //没有记录音量
#if  BT_A2DP_FIRST_CON_RESTORE_VOL
                if(sys_cb.vol != SYS_INIT_VOLUME) {                                     //恢复默认音量
                    sys_cb.vol = SYS_INIT_VOLUME;
                    printf("init_vol\n");
                }
#endif
                update = true;
            }
            if (update) {
#if BT_2ACL_EN
                if (xcfg_cb.bt_2acl_en && (bt_get_connected_num() > 1)) {            // 两个设备互相切换，播放时才设置音量
                    sys_cb.restore_dev_vol = BIT(7) | cur_dev_vol;
                } else
#endif
                {
                    msg_enqueue(EVT_A2DP_SET_VOL);
                    bt_tws_vol_change();
                }
            }
            printf("dev_vol %d\n", sys_cb.vol);
        } else {
            msg_enqueue(EVT_A2DP_SET_VOL);
            bt_tws_vol_change();
        }
        msg_enqueue(EVT_A2DP_SAVE_DEV_VOL);
#endif
#if  BT_A2DP_FIRST_CON_RESTORE_VOL
        if (packet[1]&BIT(1)) { // bit(1) ：初次连接配对
    #if BT_A2DP_VOL_CTRL_EN
            if((cfg_bt_a2dp_feature & A2DP_AVRCP_VOL_CTRL) && (packet[1] & BIT(0))) { // 支持音量同步的手机不处理
               break;
            }
    #endif
    #if BT_2ACL_EN && !BT_A2DP_RECORD_DEVICE_VOL
            if (xcfg_cb.bt_2acl_en && (bt_get_connected_num() > 1)) {            // 已经有设备在使用，不调音量
                break;
            }
    #endif
            if(sys_cb.vol != SYS_INIT_VOLUME) {                                     //恢复默认音量
                sys_cb.vol = SYS_INIT_VOLUME;
                msg_enqueue(EVT_A2DP_SET_VOL);
                bt_tws_vol_change();
                printf("init_vol\n");
            }
        }
#endif
        break;

    case BT_NOTICE_HID_CONN_EVT:
#if BT_HID_MANU_EN
        if (xcfg_cb.bt_hid_manu_en) {
            if (f_bt.hid_menu_flag == 2) {
                //按键连接/断开HID Profile完成
                if (packet[0]) {
                    f_bt.warning_status |= BT_WARN_HID_CON;
                } else {
                    f_bt.warning_status |= BT_WARN_HID_DISCON;
                }
                f_bt.hid_menu_flag = 1;
            }
    #if BT_HID_DISCON_DEFAULT_EN
            else if (f_bt.hid_menu_flag == 1) {
                if ((packet[0]) & (xcfg_cb.bt_hid_discon_default_en)) {
                    f_bt.hid_discon_flag = 1;
                }
            }
    #endif // BT_HID_DISCON_DEFAULT_EN
        }
#endif // BT_HID_MANU_EN
        break;

#if BT_TWS_EN
    case BT_NOTICE_TWS_SEARCH_FAIL:
#if WL_TIME_OUT_PWROFF				//TWS配对超时，5分钟后关机
		sys_cb.pwr_off_timeout_cnt = POWEROFF_TIME_VALUE;
#endif
		printf("BT_NOTICE_TWS_SEARCH_FAIL\n");
#if USER_TWS_PAIR_MODE
		sys_cb.tws_pair_led_flag = 0;
		msg_enqueue(EVT_BT_UPDATE_STA);
#endif
		break;
    case BT_NOTICE_TWS_CONNECT_START:
		printf("BT_NOTICE_TWS_CONNECT_START\n");
        break;
    case BT_NOTICE_TWS_DISCONNECT:
#if WL_LINKLOSS_EN				//控制单耳超距时，主机灯变成2S1闪
		sys_cb.tws_pair_led_flag = 0;
#endif
#if TWS_PAIRING_BLEM
		sys_cb.tws_disccon_flag = 1;
#endif
		printf("BT_NOTICE_TWS_DISCONNECT\n");
        f_bt.tws_status = 0;
        f_bt.warning_status |= BT_WARN_TWS_DISCON;      //TWS断线不播报提示音，仅更改声道配置
        msg_enqueue(EVT_BLE_ADV0_BAT);
#if FOT_EN
        fot_tws_disconnect_callback();
#endif
        msg_enqueue(EVT_BT_UPDATE_STA);                 //刷新显示
#if  BT_A2DP_RECORD_DEVICE_VOL
        msg_enqueue(EVT_A2DP_SAVE_DEV_VOL);
#endif
		if(!bt_is_connected()){
        	pwroff_autoset_flag(AUTOSET_EN);
		}
#if WL_SINGLE_PWR_ON
		if(!bt_nor_is_connected()){
			sys_cb.tws_connect_fail_close_flush = 2;		//优化TWS连接的时候，一只耳机关机，另一只耳机走回连灯
		}
#endif
        break;
    case BT_NOTICE_TWS_CONNECTED:
#if TWS_PAIRING_BLEM
		sys_cb.tws_disccon_flag = 0;
		sys_cb.is_need_pairing = 1;
		sys_cb.single_linkloss_flag = 0;
#endif
#if WL_SINGLE_PWR_ON
		sys_cb.tws_connect_fail_close_flush = 0;
#endif
#if WL_TIME_OUT_PWROFF				//TWS连接成功后进入配对状，在过5分钟关机
		sys_cb.pwr_off_timeout_cnt = POWEROFF_TIME_VALUE;
#endif
		printf("BT_NOTICE_TWS_CONNECTED \n");
        if(bt_tws_is_slave()){
            ble_adv_dis();  //副机关闭BLE广播
        }

#if VUSB_SMART_VBAT_HOUSE_EN
        bsp_vhouse_update_sta();
#endif
        f_bt.tws_status = packet[0];
        if(f_bt.tws_status & FEAT_TWS_MUTE_FLAG) {
            f_bt.warning_status |= BT_WARN_TWS_CON;     //无连接提示音，仅更改声道配置
        } else if(f_bt.tws_status & FEAT_TWS_ROLE) {
            f_bt.warning_status |= BT_WARN_TWS_SCON;    //TWS连接提示音
        } else {
            f_bt.warning_status |= BT_WARN_TWS_MCON;    //TWS连接提示音
        }
#if FOT_EN
        fot_tws_connect_callback();
#endif
        bsp_tws_sync_conn_callback();
#if CONNECT_TWS_1S_LED
		sys_cb.tws_conn_1s_flag = 1;					//tws连接1s
#endif
        msg_enqueue(EVT_BT_UPDATE_STA);                 //刷新显示

        if(bt_tws_is_slave()){
            pwroff_autoset_flag(AUTOSET_OFF);
        }else{
            if(!bt_is_connected()){
                pwroff_autoset_flag(AUTOSET_EN);
            }
        }
        break;
    case BT_NOTICE_TWS_CONNECT_FAIL:
#if WL_SINGLE_PWR_ON
		if(!sys_cb.tws_connect_fail_flag)					//优化有TWS记录时，单耳开机，一直走这个消息，然后一直播报pairing
		{
#endif //WL_SINGLE_PWR_ON
#if WL_TIME_OUT_PWROFF				//TWS连接超时自动关机，10S后 ，在过5分钟关机
		if(!sys_cb.tws_disccon_flag){
			sys_cb.pwr_off_timeout_cnt = POWEROFF_TIME_VALUE;
		}
#endif //WL_TIME_OUT_PWROFF

#if DISCONN_PAIR_CLOSE_MP3
		sys_cb.is_need_pairing = 1;
#endif //DISCONN_PAIR_CLOSE_MP3
#if WL_SINGLE_PWR_ON
		sys_cb.tws_connect_fail_flag = 1;
		printf("BT_NOTICE_TWS_CONNECT_FAIL ----------break\n");
		break;
		}
		sys_cb.tws_connect_fail_close_flush++;;
#endif //WL_SINGLE_PWR_ON

		printf("BT_NOTICE_TWS_CONNECT_FAIL\n");
        break;
    case BT_NOTICE_TWS_LOSTCONNECT:
		printf("BT_NOTICE_TWS_LOSTCONNECT\n");
#if WL_TIME_OUT_PWROFF				//TWS从机超距在过5分钟关机
		sys_cb.pwr_off_timeout_cnt = POWEROFF_TIME_VALUE;
#endif
#if WL_LINKLOSS_EN
		if(!bt_nor_is_connected())		//单耳超距的时候，与手机连接的手机也会播放提示音
		{
#if SLNGLE_LINKLOSS
			if(!sys_cb.link_loss_flag)
			{
				f_bt.warning_status|= BT_WARN_DISCON;

			}
#else
			f_bt.warning_status|= BT_WARN_DISCON;
#endif
		}
		led_bt_reconnect();
		sys_cb.single_linkloss_flag =1;
#endif
        break;

    case BT_NOTICE_TWS_SET_VOL:
        a2dp_vol_check(&packet[0]);
        tmp = (packet[0]+1) * VOL_MAX / 128;
        sys_cb.vol = tmp;
        if (sys_cb.vol > VOL_MAX) {
            sys_cb.vol = VOL_MAX;
        }
        msg_enqueue(EVT_TWS_SET_VOL);
        break;

	case BT_NOTICE_TWS_USER_KEY:
		tws_user_key_process(params);
		break;
	case BT_NOTICE_TWS_WARNING:
        tws_warning_process(params);
        break;
    case BT_NOTICE_TWS_STATUS_CHANGE:
        msg_enqueue(EVT_BT_UPDATE_STA);                 //刷新显示
        break;
#endif
	case BT_NOTICE_MUSIC_SET_VOL:
	    a2dp_vol_check(&packet[0]);
        tmp = (packet[0]+1) * VOL_MAX / 128;
        if(tmp != sys_cb.vol) {
            sys_cb.vol = tmp;
            if (sys_cb.vol > VOL_MAX) {
                sys_cb.vol = VOL_MAX;
            }
            msg_enqueue(EVT_A2DP_SET_VOL);
        }
        break;
//    case BT_NOTICE_RECON_FINISH:
//        printf("RECON_FAIL, reason=%d\n", packet[1]);
//        break;
//    case BT_NOTICE_ABORT_STATUS:
//        if(packet[0] != 0) {
//            printf("ABORT_START\n");
//        } else {
//            if(packet[1] == 0 || packet[1] == 0x13) {
//                printf("ABORT_OK, reason=%d, %d\n", packet[1], bt_nor_is_connected());
//            } else {
//                printf("ABORT_FAIL, reason=%d, %d\n", packet[1], bt_nor_is_connected());
//            }
//        }
//        break;
    case BT_NOTICE_LOW_LATENCY_STA:
        if (packet[0] == 1) {
            msg_enqueue(EVT_A2DP_LLTY_EN);
        } else {
            msg_enqueue(EVT_A2DP_LLTY_DIS);
        }
        break;
    }
}


#if BT_TWS_EN
//TWS蓝牙名称是否增加_L或_R后缀
void bt_name_suffix_set(void)
{
    sys_cb.name_suffix_en = 0;
    if (xcfg_cb.bt_tws_name_suffix_en) {
        if ( (xcfg_cb.bt_tws_lr_mode < 9) ||
             (xcfg_cb.bt_tws_lr_mode == 9 && xcfg_cb.tws_sel_left_gpio_sel == 0) ) {
            return;
        }

        if (strlen(xcfg_cb.bt_name) < 30) {
            if (sys_cb.tws_left_channel) {
                strcat(xcfg_cb.bt_name, "L");
            } else {
                strcat(xcfg_cb.bt_name, "R");
            }
            sys_cb.name_suffix_en = 1;
            printf("bt_name_suffix: %s\n", xcfg_cb.bt_name);
        }
    }
}
#endif // BT_TWS_EN



#if BT_TWS_MS_SWITCH_EN
#if VUSB_SMART_VBAT_HOUSE_EN && VUSB_SMART_VBAT_DELAY_DISC > 0
static bool bt_tws_wait_house_open(void)
{
    u32 tout = tick_get();
    u16 msg;
    bool fade_sta = dac_is_fade_in();
#if DAC_DNR_EN
    u8 dnr_sta = dac_dnr_get_sta();
    dac_dnr_set_sta(0);
#endif
    dac_fade_out();
    while(!tick_check_expire(tout, VUSB_SMART_VBAT_DELAY_DISC)) {   //超时后才断开
        WDT_CLR();
        msg = msg_dequeue();
		sys_cb.dc_rst_tout = 0;
        switch(msg) {
        case EVT_A2DP_MUSIC_PLAY:
            fade_sta = true;
            break;
        case EVT_A2DP_MUSIC_STOP:
            fade_sta = false;
            break;
        case EVT_A2DP_SET_VOL:
        case EVT_ONLINE_SET_EQ:
        case EVT_ONLINE_SET_ANC:
        case EVT_TWS_SET_VOL:
        case EVT_BT_SET_LANG_ID:
        case EVT_BT_SET_EQ:
        case EVT_NR_STA_CHANGE:
        case EVT_QTEST_PICKUP_PWROFF:
            func_message(msg);
            break;
        case KU_PLAY_USER_DEF:
            break;
        }
        bsp_smart_vhouse_process(0);
        if(sys_cb.loc_house_state & BIT(0)) {
            if(fade_sta) {
                dac_fade_in();
            }
#if DAC_DNR_EN
            dac_dnr_set_sta(dnr_sta);
#endif
            return true;
        }
    }

    return false;
}
#endif


//耳机进仓充电
void bt_tws_switch_for_charge(void)
{
#if CHARGE_10S_RST
	static u32 rst_tick = 0;
#endif
#if VUSB_SMART_VBAT_HOUSE_EN && VUSB_SMART_VBAT_DELAY_DISC > 0
    if(bt_tws_wait_house_open()) {
        sys_cb.dc_rst_flag = 1;
        return;
    }
#endif
#if CHARGE_INBOX_CHECK
		sys_cb.inbox_sleep_cnt = SLEEP_MODE_CNT;
#endif
    sys_cb.discon_reason = 0;
#if CLOSE_WINDOW_WHITE_LONGON 			//修改关盖 白灯常亮一会再灭的问题
	func_bt_exit();
	rled_off();
	led_off();
#if CLOSE_WINDOW_RED_ON
	rled_off_t();
#endif
#else
	rled_off();
	led_off();
    func_bt_exit();
#endif
#if ANC_EN
    u8 anc_user_mode = sys_cb.anc_user_mode;
    bsp_anc_set_mode(0);
#endif
    bsp_tws_res_music_play_clear();

	dac_power_off();
	sys_cb.dc_rst_flag = 0;
#if USER_INEAR_DETECT_EN
	sys_cb.ruer_reset_flag = 0;
#endif
	sys_cb.loc_ear_sta = 1;
	printf("bt_tws_switch_for_charge: %d\n", CHARGE_INBOX());
    lock_code_charge();
	bsp_charge_box_enter(1);

    while(1) {
#if VUSB_TBOX_QTEST_EN
        if(qtest_get_mode()||sys_cb.qtest_sta){
            break;
        }
#endif
        WDT_CLR();
#if CHARGE_LOW_POWER_EN
        delay_us(800);
#else
        delay_5ms(1);
#endif
#if CHARGE_10S_RST
		 if (tick_check_expire(rst_tick, 1000)) {
				rst_tick = tick_get();
				if (++sys_cb.charge_rst_time_out >= 10) {
					//printf("charge rst \n");
					WDT_RST();
				}
		 }
#endif

        if (bsp_charge_box_process()) {
            break;
        }
    }
    bsp_charge_box_exit();
    unlock_code_charge();
#if POWER_UP_LED_CTL
	sys_cb.pwr_led_on_time_flag = 1;
	sys_cb.pwr_led_on_ctl = 1;
#endif
#if USB_CHARGE_NO_FULL		//修改充电仓插USB充电时耳机充不满的问题
	sys_cb.charge_time_out = 0;
#endif
#if CHARGE_10S_RST
	sys_cb.charge_rst_time_out = 0;
#endif
    printf("dc out\n");
	sys_cb.delay_connect_flag = 0;
	sys_cb.close_window_flag = 0;
#if CLOSE_WINDOW_RED_ON
	rled_on_t();
#endif
#if WL_TIME_OUT_PWROFF				//手动与手机断链，5分钟后关机
	sys_cb.pwr_off_timeout_cnt = POWEROFF_TIME_VALUE;
#endif
#if CHARGE_INBOX_CHECK				//从仓内开盖开机
	sys_cb.from_box_power_on = 1;
	sys_cb.inbox_led_flag = 0;
#endif
#if USER_INEAR_DETECT_EN
	sys_cb.ruer_reset_flag = 0;
#endif

#if SINGE_DC_OUT
	sys_cb.is_need_pairing = 1;
	sys_cb.tws_connect_fail_close_flush	=0;
	sys_cb.tws_disccon_flag = 0;
	sys_cb.single_linkloss_flag = 0;
	sys_cb.link_loss_flag = 0;
	sys_cb.link_loss_cnt = 0;
	sys_cb.tws_connect_fail_flag = 0;
	sys_cb.first_connect_ruer = 1;
	sys_cb.ruer_cmd_open = 0;
	sys_cb.double_click_query_version_flag = 0;
#endif
#if PLXC_UP_EN
	sys_cb.i2c_flag = 0;
	sys_cb.plxc_spp_flag = 0;
#endif
    sys_cb.charge_ok_led_flag = 0 ;

	sys_cb.loc_ear_sta = 1;
#if OUTBOX_NO_SIRI
	sys_cb.outbox_siri_cnt = 4;
#endif
	sys_cb.charge_led_cnt = 0;
	sys_cb.dc_out_flag = 1;
#if !POWER_UP_LED_CTL
	led_power_up();
#endif
    //led_bt_init();
    set_sys_clk(SYS_CLK_SEL);
    func_bt_init();
	//key_init();
	bsp_adc_recover_start();


    en_auto_pwroff();
    if (xcfg_cb.bt_tws_pair_mode > 1) {
        bt_tws_set_scan(0x03);
    }
    if (!bsp_dac_off_for_bt_conn()) {
        dac_restart();
    }
#if ANC_EN
    bsp_anc_set_mode(anc_user_mode);
    dac_fade_in();
#endif
#if DAC_DNR_EN
    dac_dnr_set_sta(1);
    sys_cb.dnr_sta = 1;
#endif
#if CHARGE_INBOX_CHECK
	bsp_sys_unmute();
#endif
    if (xcfg_cb.bt_outbox_voice_pwron_en) {
		if(func_cb.sta!= FUNC_PWROFF){
       		mp3_res_play(RES_BUF_POWERON_MP3, RES_LEN_POWERON_MP3);
		}
    }
#if CHARGE_INBOX_CHECK
	sys_cb.inbox_cnt = 0;
#endif
    bt_audio_enable();
}
#endif // BT_TWS_MS_SWITCH_EN

