#include "config.h"

ENTRY(_start)

//Define the flash max size
__max_flash_size = FLASH_CODE_SIZE;

__base = 0x10000000;

// Define the area size
__cache_stack_vma   = 0x10600;  //for loader
__cache_stack_size  = 512;      //for loader
__stack_vma         = 0x10800;
__stack_ram_size    = 1k;
__heap_vma          = 0x10c00;
__heap_ram_size     = 12k;
__data_vma          = 0x13c00;
__data_ram_size     = 13k;
__comm_vma          = 0x17000;
__comm_ram_size     = 36k;
__cram_vma          = 0x20000;
__cram_ram_size     = 16k;
__bram_vma          = 0x24000;
__bram_ram_size     = 48k;

__mram_vma          = 0x30000;
__mram_vma_size     = 31k;
__sndp_comm_vma     = 0x37c00;
__sndp_comm_ram_size = 19k;
__sndp1_comm_vma     = 0x30000;
__sndp1_comm_ram_size = 40k;
__sram_vma          = 0x3c800;
__sram_ram_size     = 14k;


//Define the area
__aram_vma = 0x50000;
__eram_vma = 0x54000;
__dram_vma = 0x58000;
__fram_vma = 0x3A000;
__rram_vma = 0x55800;  //for rnn input/output

MEMORY
{
    init            : org = __base,             len = 512
    flash(rx)       : org = __base + 512,       len = __max_flash_size
    comm(rx)        : org = __comm_vma,         len = __comm_ram_size
    sndp_comm(rx)   : org = __sndp_comm_vma,    len = __sndp_comm_ram_size
    sndp1_comm(rx)  : org = __sndp1_comm_vma,   len = __sndp1_comm_ram_size

    stack           : org = __stack_vma,        len = __stack_ram_size
    heap            : org = __heap_vma,         len = __heap_ram_size
    data            : org = __data_vma,         len = __data_ram_size
    bram            : org = __bram_vma,         len = __bram_ram_size
    cram            : org = __cram_vma,         len = __cram_ram_size
    aram            : org = __aram_vma,         len = 16k
    dram            : org = __dram_vma,         len = 0x4500
    eram            : org = __eram_vma,         len = 0xa00
    fram            : org = __fram_vma,         len = 24k
    mram            : org = __mram_vma,         len = __mram_vma_size
    sram            : org = __sram_vma,         len = __sram_ram_size
    rram            : org = __rram_vma,         len = 608
}

SECTIONS
{
    .init : {
        *(.reset)
    } > init

    .comm : {
        *(.vector)
        *(.plt)
        *(.com_text*)
        *(.bcom_text*)
        *(.bcom_rodata*)
        *(.com_text.stack.handler)
        *(.com_rodata*)
        *debug.o(.rodata*)
        *(.data*)
        *(.sdata*)
        *(.load_text)
        *(.load_rodata)
        . = ALIGN(512);
    } > comm AT > flash

    .sndp_comm : {
        __code_start_sndp = .;
        *(.sndp_text*)
        *(.sndp_rodata*)
        __code_end_sndp = .;

        __code_start_sndp_dm = .;
        *(.sndp_dm_text*)
        *(.sndp_dm_rodata*)
        __code_end_sndp_dm = .;

        __code_start_dmns = .;
        *(.dmns_text*)
        *(.dmns_rodata*)
        __code_end_dmns = .;
        . = ALIGN(512);
    } > sndp_comm AT > flash

    .sndp1_comm : {
        __code_start_sndp1 = .;
        *(.sndp1_text*)
        *(.sndp1_rodata*)
        . = ALIGN(512);
        __code_end_sndp1 = .;

        __code_start_dnn = .;
        *(.dnn_text*)
        *(.dnn_rodata*)
         . = ALIGN(512);
        __code_end_dnn = .;
    } > sndp1_comm AT > flash

    .flash : {
        __code_start_stream = .;
        *(.com_stream.text*)
        . = ALIGN(512);
        __code_end_stream = .;

        __code_start_sniff = .;
        *(.sniff*)
        . = ALIGN(512);
        __code_end_sniff = .;

        __code_start_btdec_com = .;
        *(.btdec*)
        . = ALIGN(512);
        __code_end_btdec_com = .;

        __code_start_mp3dec = .;
        *(.mp3dec*)
        . = ALIGN(512);
        __code_end_mp3dec = .;

        __code_start_sbcdec = .;
        *(.sbcdec*)
        *(.sbc_com*)
        . = ALIGN(512);
        __code_end_sbcdec = .;

        __code_start_aacdec = .;
        *(.aacdec*)
        . = ALIGN(512);
        __code_end_aacdec = .;

        __code_start_mp3enc = .;
        *(.mpaenc*)
        . = ALIGN(512);
        __code_end_mp3enc = .;

        __code_start_wavdec = .;
        *(.wavdec*)
        . = ALIGN(512);
        __code_end_wavdec = .;

        __code_start_wmadec = .;
        *(.wmadec*)
        . = ALIGN(512);
        __code_end_wmadec = .;

        __code_start_apedec = .;
        *(.apedec*)
        . = ALIGN(512);
        __code_end_apedec = .;

        __code_start_flacdec = .;
        *(.flacdec*)
        . = ALIGN(512);
        __code_end_flacdec = .;

        __code_start_usbdev = .;
        *(.usbdev*)
        . = ALIGN(512);
        __code_end_usbdev = .;

        __code_start_wma_rlc44q = .;
        *(.wma_huff_rlc44q_table)
        . = ALIGN(512);
        __code_end_wma_rlc44q = .;

        __code_start_wma_rlc44o = .;
        *(.wma_huff_rlc44o_table)
        . = ALIGN(512);
        __code_end_wma_rlc44o = .;

        __code_start_wma_rlc16 = .;
        *(.wma_huff_rlc16_table)
        . = ALIGN(512);
        __code_end_wma_rlc16 = .;

        __code_start_sleep = .;
        *(.saradc_text*)
        *(.sleep_text*)
        . = ALIGN(512);
        __code_end_sleep = .;

        __code_start_charge = .;
        *(.charge_text*)
        *(.text.bsp.charge)
        *(.text.bsp.tkey)
        *(.text.vhouse)
        . = ALIGN(512);
        __code_end_charge = .;

        __code_start_bt_voice = .;
        *(.bt_voice*)
        *(.sco_mic_dnr*)
        . = ALIGN(512);
        __code_end_bt_voice = .;

        __code_start_pwrsave = .;
        *(.text.pwroff*)
        . = ALIGN(512);
        __code_end_pwrsave = .;

        __code_start_bb_test = .;
        *(.bb_test*)
        *(.bb_test_rodata*)
        . = ALIGN(512);
        __code_end_bb_test = .;

         __code_start_piano = .;
        *(.piano_com*)
        . = ALIGN(512);
        __code_end_piano = .;

        __code_start_record = .;
        *(.com_rec*)
        . = ALIGN(512);
        __code_end_record = .;

        __code_start_update = .;
        *(.text.update*)
        . = ALIGN(512);
        __code_end_update = .;

        __code_start_huart = .;
        *(.com_huart*)
        . = ALIGN(512);
        __code_end_huart = .;

        __code_start_spdif = .;
        *(.spdif_text*)
        . = ALIGN(512);
        __code_end_spdif = .;

        __code_start_fota = .;
        *(.text.fot.cache*)
        . = ALIGN(512);
        __code_end_fota = .;

        __code_start_dueros = .;
        *(.dueros_text*)
        . = ALIGN(512);
        __code_end_dueros = .;

        __code_start_opus = .;
        *(.text.opus.asm)
        *(.text.opus.proc)
        *(.text.opus.ec_tell)
        *(.text.opus.entenc)
        *(.text.opus.mathops)
        *(.rodata.opus*)
        . = ALIGN(512);
        __code_end_opus = .;

        __code_start_bts = .;
        *(.text.stk.tws.txreq*)
        *(.text.stk.l2cap.txreq*)
        . = ALIGN(512);
        *(.text.stk.tws.send*)
        . = ALIGN(512);
        *(.text.stk.tws.recv*)
        . = ALIGN(512);
        *(.text.stk.hci.tx*)
        . = ALIGN(512);
        *(.text.bts.hci.rx*)
        . = ALIGN(512);
        *(.text.stk.bt.acl.tx*)
        . = ALIGN(512);
        __code_end_bts = .;

        *(.irq_init.aligned)
        . = ALIGN(512);
        *(.text.ble.chmap*)
        . = ALIGN(512);
        *(.text*)
        *(.rodata*)
        *(.srodata*)
        *(.rela*)
        LONG(0)
        . = ALIGN(512);

    } > flash

    .stack (NOLOAD) : {
        __irq_stack_start = .;
        . = 0x400;
        __irq_stack = .;
    } > stack
    __irq_stack_size = __irq_stack - __irq_stack_start;

    .bss (NOLOAD) : {
        __bss_start = .;
        *(COMMON)
        *(.bss*)
        *(.sbss*)
        *(.buf*)
        *(.dac_obuf)
        *(.btmem.stack.sdp)
        *(.btmem.stack.spp)
        *(.btmem.stack.l2cap)
        *(.btmem.stack.hid)
        *(.qtest_buf)
        __bss_end = .;
        *(.btmem.noload_mem*)
    } > data
    __bss_size = __bss_end - __bss_start;

    .heap (NOLOAD) : {
        *(.mem_heap)
    } > heap

    .bram __bram_vma (NOLOAD) : {
        *(.btmem.bthw)
        *(.btmem*)

        *(.ble_cache*)
        *(.ble_buf*)
        *(.dueros_dma*)
    } > bram

    .aac_btmem __bram_vma (NOLOAD) : {
        *(.aac_pcm)
    } > bram

    .mp3enc_btmem __bram_vma (NOLOAD) : {
        *(.l2enc_sb_buf)
    } > bram

    .echo_btmem __bram_vma (NOLOAD) : {
        *(.echo_buf)
    } > bram

    .cram_aac __cram_vma (NOLOAD) : {
        *(.aac_buf*)
        *(.a2dp_play*)
        *(.fcc.huart_buf)
        . = 0x2000;

        . = 0x3d00;
        *(.tkey_buf.debug)
        *(.wav_res_buf)
        . = 0x4000;
    } > cram

    .cram_sbcenc __cram_vma (NOLOAD) : {
        *(.sbcenc.buf)
        *(.nr_buf*)
        *(.bt_sndp*)
        *(.sndp.bqt)
        *(.sco_eq_buf)
        *(.bt_dmns*)
        . = 0x3e00;
        *(.anc_temp_buf)
    } > cram

    .aec_ram __aram_vma (NOLOAD) : {
        *(.aec_buf*)
        . = 0x3310;
        *(.nr_cache*)
        . = 0x4000;
    } > aram

    .nlms_aec_ram __aram_vma (NOLOAD) : {
        *(.nlms_aec_buf)
        *(.td_aec.buf)
        *(.nlp_aec_buf*)
        *(.aec_data*)
        . = 0x3310;
    } > aram

    .fcc_buf __aram_vma (NOLOAD) : {
        . = 0x1000;
        *(.fcc_buf.huart)
    } > aram

    .sbc_cache __aram_vma (NOLOAD) : {
        *(.sbc_cache_buf)
        . = 0x4000;
    } > aram

    .aram_music __aram_vma (NOLOAD) : {
        *(.mp3buf.dec)
        . = 0x1000;
         *(.pff_buf*)
        . = 0x1200;
        *(.usb_buf*)
        *(.mp3_id3_buf)
        *(.lrc_buf*)
        *(.pff.scan.tempbuf)
        . = 0x3800;
        *(.avio_buf)
        *(.diskio_buf)
        *(.pff.buf)
        *(.fname.buf)
        . = 0x4000;
    } > aram

    .aram_msc_stm __aram_vma (NOLOAD) : {
        *(.stm_buf)
        . = 0x1000;
    } > aram

    .aram_fnav __aram_vma (NOLOAD) : {
        . = 0x1000;
        *(.fnav_buf)
        . = 0x2800;
    } > aram

    .aram_fmrx __aram_vma (NOLOAD) : {
        . = 0x1800;
        *(.fmrx_cb_buf)
    } > aram

    .aram_enc __aram_vma (NOLOAD) : {
        *(.l2enc.buf*)
        . = 0x1000;
        *(.voice_assistant*)
        *(.karaok.l2enc)
        . = 0x3800;
    } > aram

    .aram_wma __aram_vma (NOLOAD) : {
        . = 0x1200;
        *(.wma_coefs_buf0)
        *(.wma_ctx_buf)
        *(.wma_wincb_buf)
        *(.wma_id3_buf)
        . = 0x3800;
    } > aram

    .aram_flac __aram_vma (NOLOAD) : {
        . = 0x1200;
        *(.flac_decoded_buf)
        . = 0x3800;
    } > aram

    .dram_upd __dram_vma (NOLOAD) : {
        . = 0x3800;
        *(.upd_ota*)
        *(.upd_buf*)
        . = 0x4500;
    } > dram

    .dram_upd __dram_vma (NOLOAD) : {
        *(.ota_buf*)
    } > dram

    .dram_mp3 __dram_vma (NOLOAD) : {
        . = 0x1000;
        *(.mp3_decram_buf)
        . = 0x4500;
    } > dram

    .dram_aac __dram_vma (NOLOAD) : {
        *(.aac_decram_buf)
        *(.aac_cache*)
        . = 0x4500;
    } > dram

    .dram_ape __dram_vma (NOLOAD) : {
        *(.apebuf.cb)
        *(.apebuf.filter)
        *(.apebuf.decode)
        *(.apebuf.seektbl)
        . = 0x3000;
    } > dram

    .dram_wma __dram_vma (NOLOAD) : {
        *(.wmabuf*)
        . = 0x4500;
    } > dram

    .dram_flac __dram_vma (NOLOAD) : {
        *(.flacbuf*)
         *(.flac_decoded_buf1)
        *(.wavbuf.dec)
        . = 0x4500;
    } > dram

    .dram_sbc __dram_vma (NOLOAD) : {
        *(.sbc_decram_buf)
        . = 0x1000;
        *(.sbcbuf*)

        *(.tsco_cache)
        *(.sco_cache)
        *(.sco_data)
        *(.sco_hci)
        *(.plc_buf)
        *(.plc_data)
        . = 0x4500;
    }  > dram

    .dram_rec __dram_vma (NOLOAD) : {
        *(.karaok.rec)
        . = 0xD38;
        *(.rec.wave.header)
        *(.recenc.obuf)
        *(.rec.obuf)
        *(.l2enc_bit_buf)
        . = 0x3000;
        *(.gpdma_buf)
    } > dram

    .dram_usb __dram_vma (NOLOAD) : {
        *(.usb.device.desc.buf)
        *(.ude.aubuf)
        . = 0x3000;
    } > dram

    .dram_spf __dram_vma (NOLOAD) : {
        *(.spf_buf)
        . = 0x3000;
    } > dram

    .dram_opus __dram_vma (NOLOAD) : {
        *(.opus_buf*)
        //. = 0x4500;
    } > dram


    .dram_eq_rec __dram_vma (NOLOAD) : {
        *(.eq_rec_buf)
        . = 0x1000;
    }  > dram

//    .eram_gpdma __eram_vma (NOLOAD) : {
//        *(.ble_cache*)
//        *(.ble_buf*)
//        . = 0xa00;
//    } > eram

    .eram_fot __eram_vma (NOLOAD) : {
        *(.fot_data.*)
        . = 0xa00;
    } > eram

    .eram_tp __eram_vma (NOLOAD) : {
        *(.tp*)
        . = 0xa00;
    } > eram

    .fram_sndp1 __fram_vma (NOLOAD) : {
        *(.sndp1*)
        . = 0x6000;
    } > fram

    .fram_dnn __fram_vma (NOLOAD) : {
        *(.dnn_data*)
        . = 0x6000;
    } > fram

    .fram_wma __fram_vma (NOLOAD) : {
        . = 0x1000;
        *(.wma_coefs_buf)
        . = 0x3000;
    } > fram

    .mram_rnn __mram_vma (NOLOAD) : {
        *(.rnn_model)
        *(.sndp.denoise)
        . = 0x7c00;
    } > mram

    .mram_dm __mram_vma (NOLOAD) : {
        *(.sndp_dm*)
        . = 0x7c00;
    } > mram

    .mram_ains2 __mram_vma (NOLOAD) : {
        *(.ains2_buf*)
        . = 0x3000;
    } > mram

    .mram_a2dp __mram_vma (NOLOAD) : {
        *(.a2dp_cache1)
        *(.ttp_adc_buf)
        . = 0x7c00;
    } > mram

    .mram_kalaok __mram_vma (NOLOAD) : {
        *(.mav_cache)
        *(.mav_buf)
        *(.mav_buf1)
        *(.mav_cache1)
        *(.echo_cache)
        *(.echo_cache1)
        . = 0x5148;;
    } > mram

    .mram_scomav __mram_vma (NOLOAD) : {
        *(.mavsco_buf)
    } > mram

    .mram_dmns __mram_vma (NOLOAD) : {
        *(.dmns_buf*)
        . = 0x7c00;
    } > mram

    .sram_dmns __sram_vma (NOLOAD) : {
        *(.dmns_data*)
        . = 0x3800;
    } > sram

    .sram_sndp __sram_vma (NOLOAD) : {
        *(.sndp*)
        . = 0x3800;
    } > sram

    .rram_rnn __rram_vma (NOLOAD) : {
        . = 0x1c0;
        *(.rnn_input)
        . = 0x230;
        *(.rnn_output)
    } > rram
}

//Calc the lma
__bank_size = SIZEOF(.flash);
__comm_lma = LOADADDR(.comm);
__comm_size = SIZEOF(.comm);
__sndp_comm_lma = LOADADDR(.sndp_comm);
__sndp_comm_size = SIZEOF(.sndp_comm);
__sndp1_comm_lma = LOADADDR(.sndp1_comm);
__sndp1_comm_size = SIZEOF(.sndp1_comm);
