/**
 * @file lv_port_disp_templ.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include <stdbool.h>
#include "./SCREEN/RGBLCD/rgblcd.h"
#include "gd32h7xx_libopt.h"

/*********************
 *      DEFINES
 *********************/
 #define MY_DISP_HOR_RES    800
#define MY_DISP_VER_RES    480

#ifndef MY_DISP_HOR_RES
    #warning Please define or replace the macro MY_DISP_HOR_RES with the actual screen width, default value 320 is used for now.
    #define MY_DISP_HOR_RES    320
#endif

#ifndef MY_DISP_VER_RES
    #warning Please define or replace the macro MY_DISP_VER_RES with the actual screen height, default value 240 is used for now.
    #define MY_DISP_VER_RES    240
#endif

#define BYTE_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565)) /*will be 2 for RGB565 */

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    /*------------------------------------
     * Create a display and set a flush_cb
     * -----------------------------------*/
    lv_display_t * disp = lv_display_create(MY_DISP_HOR_RES, MY_DISP_VER_RES);
    lv_display_set_flush_cb(disp, disp_flush);

    /* Example 1
     * One buffer for partial rendering*/
//    LV_ATTRIBUTE_MEM_ALIGN
//    static uint8_t buf_1_1[MY_DISP_HOR_RES * 10 * BYTE_PER_PIXEL];            /*A buffer for 10 rows*/
//    lv_display_set_buffers(disp, buf_1_1, NULL, sizeof(buf_1_1), LV_DISPLAY_RENDER_MODE_PARTIAL);
    

    /* Example 2
     * Two buffers for partial rendering
     * In flush_cb DMA or similar hardware should be used to update the display in the background.*/
    LV_ATTRIBUTE_MEM_ALIGN
    static uint8_t buf_2_1[MY_DISP_HOR_RES * 120 * BYTE_PER_PIXEL];

    LV_ATTRIBUTE_MEM_ALIGN
    static uint8_t buf_2_2[MY_DISP_HOR_RES * 120 * BYTE_PER_PIXEL];
    lv_display_set_buffers(disp, buf_2_1, buf_2_2, sizeof(buf_2_1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* Example 3
     * Two buffers screen sized buffer for double buffering.
     * Both LV_DISPLAY_RENDER_MODE_DIRECT and LV_DISPLAY_RENDER_MODE_FULL works, see their comments*/
//    LV_ATTRIBUTE_MEM_ALIGN
//    static uint8_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES * BYTE_PER_PIXEL];

//    LV_ATTRIBUTE_MEM_ALIGN
//    static uint8_t buf_3_2[MY_DISP_HOR_RES * MY_DISP_VER_RES * BYTE_PER_PIXEL];
//    lv_display_set_buffers(disp, buf_3_1, buf_3_2, sizeof(buf_3_1), LV_DISPLAY_RENDER_MODE_DIRECT);

//    lv_display_set_buffers(disp, (void *)(0xC0000000), (void *)(0xC0100000), 800 * 480 * 2, LV_DISPLAY_RENDER_MODE_DIRECT);

}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
    /*You code here*/
    if(rgblcd_status.rgblcd_state != true)
    {
        rgblcd_init();
    }
    rgblcd_clear(BLACK);    /* ���ñ���Ϊ��ɫ */
    /* ����ipa�ж����ȼ� */
    nvic_irq_enable(IPA_IRQn, 3, 0);
    IPA_CTL &= ~(IPA_CTL_TAEIE | IPA_CTL_FTFIE | IPA_CTL_TLMIE | IPA_CTL_LACIE | IPA_CTL_LLFIE | IPA_CTL_WCFIE); /* ʧ����Ӧ�ж� */
}

volatile bool disp_flush_enabled = true;
volatile static uint8_t lv_ipa_flag = 0;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

/*Flush the content of the internal buffer the specific area on the display.
 *`px_map` contains the rendered image as raw pixel map and it should be copied to `area` on the display.
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_display_flush_ready()' has to be called when it's finished.*/
static void disp_flush(lv_display_t * disp_drv, const lv_area_t * area, uint8_t * px_map)
{
    if(disp_flush_enabled)
    {
        lv_ipa_flag = 1;
        IPA_CTL |= IPA_CTL_TST;             /* ֹͣ���� */
        IPA_CTL |= (IPA_CTL_TAEIE | IPA_CTL_FTFIE); /* ʹ����Ӧ�ж� */
        IPA_CTL &= ~(IPA_CTL_PFCM);
        IPA_CTL |= IPA_FGTODE;              /* ǰ����洢����Ŀ��洢�������ظ�ʽת�� */
        IPA_DLOFF = rgblcd_status.param->width - (area->x2 - area->x1 + 1);    /* Ŀ����ƫ�� */
        IPA_DMADDR = (uint32_t)((uint16_t *)rgblcd_status.fb + (area->y1 * rgblcd_status.param->width + area->x1));
        IPA_IMS = (((area->x2 - area->x1 + 1) << 16U) | (area->y2 - area->y1 + 1));
        IPA_FLOFF = 0;                                              /* ǰ������ƫ�� */
        IPA_FMADDR = (uint32_t)px_map;                        /* ǰ����洢������ַ */
        while(!(TLI_STAT & TLI_STAT_VS));
        IPA_CTL |= IPA_CTL_TEN;
    }

    /*IMPORTANT!!!
     *Inform the graphics library that you are ready with the flushing*/
    /* lv_display_flush_ready(disp_drv) in  IPA_IRQHandler */
}

void IPA_IRQHandler(void)
{
    if(IPA_INTF & IPA_FLAG_TAE) /* �����ж� */
    {
        IPA_INTC |= IPA_FLAG_TAE;
    }
    if(IPA_INTF & IPA_FLAG_FTF) /* ��������ж� */
    {
        IPA_INTC |= IPA_FLAG_FTF;
        if(lv_ipa_flag)
        {
            lv_ipa_flag = 0;
            lv_display_flush_ready(lv_display_get_default());
        }
    }
    IPA_INTC |= 0x3F;
    IPA_CTL &= ~(IPA_CTL_TAEIE | IPA_CTL_FTFIE); /* ʧ����Ӧ�ж� */
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
