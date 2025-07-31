/*!
    \file       rgblcd.c
    \brief      RGB LCD driver for TLI interface with graphics operations
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
*/

#include "gd32h7xx_libopt.h"
#include "./SCREEN/RGBLCD/rgblcd.h"
#include "TIMER/timer.h"
#include "./DELAY/delay.h"
#include "./USART/usart.h"
#include <math.h>

#if RGBLCD_USING_TOUCH
#include "./SCREEN/RGBLCD/rgblcd_touch.h"
#endif

rgblcd_status_struct rgblcd_status;              /*!< RGB LCD status structure instance */
#define RGBLCD_LAYER_FB_ADDR    ((uint32_t)0xC0000000)  /*!< Frame buffer base address */

/*!
    \brief      configure TLI GPIO pins
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void rgblcd_tli_gpio_config(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOE);
    rcu_periph_clock_enable(RCU_GPIOF);
    rcu_periph_clock_enable(RCU_GPIOG);
    rcu_periph_clock_enable(RCU_GPIOH);
    
    /* CLK(PG7), DE(PF10), HS(PC6), VS(PA7) */
    gpio_af_set(GPIOA, GPIO_AF_14, GPIO_PIN_7);
    gpio_af_set(GPIOC, GPIO_AF_14, GPIO_PIN_6);
    gpio_af_set(GPIOF, GPIO_AF_14, GPIO_PIN_10);
    gpio_af_set(GPIOG, GPIO_AF_14, GPIO_PIN_7);
    
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_7);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_7);
    
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_6);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_6);
    
    gpio_mode_set(GPIOF, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_10);
    gpio_output_options_set(GPIOF, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_10);
    
    gpio_mode_set(GPIOG, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_7);
    gpio_output_options_set(GPIOG, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_7);
 
    /* R0(PH2), R1(PH3), R2(PH8), R3(PH9), R4(PH10), R5(PH11), R6(PH12), R7(PG6) */
    gpio_af_set(GPIOG, GPIO_AF_14, GPIO_PIN_6);
    gpio_af_set(GPIOH, GPIO_AF_14, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_8 | GPIO_PIN_9 | \
                                   GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);

    gpio_mode_set(GPIOG, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_6);
    gpio_output_options_set(GPIOG, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_6);
    
    gpio_mode_set(GPIOH, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_8 | \
                                       GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);
    gpio_output_options_set(GPIOH, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_2 | GPIO_PIN_3 | \
                                   GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);
                                   
    /* G0(PE5), G1(PE6), G2(PH13), G3(PH14), G4(PH15), G5(PH4), G6(PC7), G7(PD3) */
    gpio_af_set(GPIOC, GPIO_AF_14, GPIO_PIN_7);
    gpio_af_set(GPIOD, GPIO_AF_14, GPIO_PIN_3);
    gpio_af_set(GPIOE, GPIO_AF_14, GPIO_PIN_5 | GPIO_PIN_6);
    gpio_af_set(GPIOH, GPIO_AF_9, GPIO_PIN_4);
    gpio_af_set(GPIOH, GPIO_AF_14, GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_7);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_7);
    
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_3);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_3);
    
    gpio_mode_set(GPIOE, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_5 | GPIO_PIN_6);
    gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_5 | GPIO_PIN_6);
    
    gpio_mode_set(GPIOH, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_4 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_output_options_set(GPIOH, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_4 | GPIO_PIN_13 | \
                                                                          GPIO_PIN_14 | GPIO_PIN_15);
    
    /* B0(PE4), B1(PC10), B2(PC9), B3(PA8), B4(PC11), B5(PA3), B6(PA15), B7(PD2) */
    gpio_af_set(GPIOA, GPIO_AF_13, GPIO_PIN_8);
    gpio_af_set(GPIOA, GPIO_AF_14, GPIO_PIN_3 | GPIO_PIN_15);
    gpio_af_set(GPIOC, GPIO_AF_10, GPIO_PIN_10);
    gpio_af_set(GPIOC, GPIO_AF_14, GPIO_PIN_9 | GPIO_PIN_11);
    gpio_af_set(GPIOD, GPIO_AF_9, GPIO_PIN_2);
    gpio_af_set(GPIOE, GPIO_AF_14, GPIO_PIN_4);
    
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_3 | GPIO_PIN_8 | GPIO_PIN_15);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_3 | GPIO_PIN_8 | GPIO_PIN_15);
    
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11);
    
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_2);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_2);
    
    gpio_mode_set(GPIOE, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_4);
    gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_4);
}

/*!
    \brief      configure RGB LCD power control GPIO
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void rgblcd_pwr_gpio_config(void)
{
    rcu_periph_clock_enable(RCU_GPIOD);
    
    gpio_mode_set(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_4);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_12MHZ, GPIO_PIN_4);
    gpio_bit_write(GPIOD, GPIO_PIN_4, SET);
}

/*!
    \brief      configure RGB LCD backlight control
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void rgblcd_blk_config(void)
{
    rcu_periph_clock_enable(RCU_GPIOC);

    gpio_af_set(GPIOC, GPIO_AF_2, GPIO_PIN_8); 
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_8);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_8);
    timer_general2_config();
}

/*!
    \brief      configure TLI (LCD-TFT Interface) peripheral
    \param[in]  width: display width in pixels
    \param[in]  height: display height in pixels  
    \param[in]  timing: pointer to timing configuration structure
    \param[out] none
    \retval     rgblcd_error_enum
*/
static rgblcd_error_enum rgblcd_tli_config(uint16_t width, uint16_t height, const rgblcd_timing_struct *timing)
{
    tli_parameter_struct               tli_init_struct;
    tli_layer_parameter_struct         tli_layer_init_struct;
    
    rcu_periph_clock_enable(RCU_TLI);
    rcu_tli_clock_div_config(RCU_PLL2R_DIV2);   /* TLI clock = 48 / 2 = 24MHz */
    
    tli_deinit();
    /* configure TLI parameter struct */
    tli_init_struct.signalpolarity_hs      = TLI_HSYN_ACTLIVE_LOW;
    tli_init_struct.signalpolarity_vs      = TLI_VSYN_ACTLIVE_LOW;
    tli_init_struct.signalpolarity_de      = TLI_DE_ACTLIVE_LOW;
    tli_init_struct.signalpolarity_pixelck = (timing->pixel_clock_polarity != 0) ? TLI_PIXEL_CLOCK_INVERTEDTLI : TLI_PIXEL_CLOCK_TLI;
    /* LCD display timing configuration */
    tli_init_struct.synpsz_hpsz   = timing->hsync_len - 1;
    tli_init_struct.synpsz_vpsz   = timing->vsync_len - 1;
    tli_init_struct.backpsz_hbpsz = timing->hsync_len + timing->hback_porch - 1;
    tli_init_struct.backpsz_vbpsz = timing->vsync_len + timing->vback_porch - 1;
    tli_init_struct.activesz_hasz = timing->hsync_len + timing->hback_porch + timing->hactive - 1;
    tli_init_struct.activesz_vasz = timing->vsync_len + timing->vback_porch + timing->vactive - 1;
    tli_init_struct.totalsz_htsz  = timing->hsync_len + timing->hback_porch + timing->hactive + timing->hfront_porch - 1;
    tli_init_struct.totalsz_vtsz  = timing->vsync_len + timing->vback_porch + timing->vactive + timing->vfront_porch - 1;
    /* configure LCD background R,G,B values */
    tli_init_struct.backcolor_red   = 0x00;
    tli_init_struct.backcolor_green = 0x00;
    tli_init_struct.backcolor_blue  = 0x00;
    tli_init(&tli_init_struct);
    
    /* TLI layer0 configuration */
    /* TLI window size configuration */
    tli_layer_init_struct.layer_window_leftpos   = timing->hsync_len + timing->hback_porch;
    tli_layer_init_struct.layer_window_rightpos  = timing->hsync_len + timing->hback_porch + width - 1;
    tli_layer_init_struct.layer_window_toppos    = timing->vsync_len + timing->vback_porch;
    tli_layer_init_struct.layer_window_bottompos = timing->vsync_len + timing->vback_porch + height - 1;
    /* TLI window pixel format configuration */
    tli_layer_init_struct.layer_ppf = LAYER_PPF_RGB565;
    /* TLI window specified alpha configuration */
    tli_layer_init_struct.layer_sa = 255;                   /*!< Alpha value: 255 = fully opaque */
    /* TLI layer default alpha R,G,B value configuration */
    tli_layer_init_struct.layer_default_alpha = 0xFF;
    tli_layer_init_struct.layer_default_red   = 0xFF;
    tli_layer_init_struct.layer_default_green = 0xFF;
    tli_layer_init_struct.layer_default_blue  = 0xFF;
    /* TLI window blend configuration */
    tli_layer_init_struct.layer_acf1 = LAYER_ACF2_SA;
    tli_layer_init_struct.layer_acf2 = LAYER_ACF2_SA;
    /* TLI layer frame buffer base address configuration */
    tli_layer_init_struct.layer_frame_bufaddr     = (uint32_t)RGBLCD_LAYER_FB_ADDR;
    tli_layer_init_struct.layer_frame_line_length = ((width * 2) + 3);
    tli_layer_init_struct.layer_frame_buf_stride_offset = (width * 2);
    tli_layer_init_struct.layer_frame_total_line_number = height;
    tli_layer_init(LAYER0, &tli_layer_init_struct);
    
    tli_dither_config(TLI_DITHER_DISABLE);              /*!< disable dithering */
    /* enable layer0 */
    tli_layer_enable(LAYER0); 
    tli_reload_config(TLI_FRAME_BLANK_RELOAD_EN);       /*!< reload layer0 and layer1 configuration */
    tli_enable();
    
    return RGBLCD_OK;
}

/*!
    \brief      get RGB LCD module ID
    \param[in]  none
    \param[out] none
    \retval     uint8_t: LCD module ID value
*/
static uint8_t rgblcd_get_id(void)
{
    uint8_t id;
  
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOG);

    gpio_mode_set(GPIOD, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO_PIN_2);  /*B7*/
    gpio_mode_set(GPIOD, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO_PIN_3);  /*G7*/
    gpio_mode_set(GPIOG, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO_PIN_6);  /*R7*/

    id =  gpio_input_bit_get(GPIOD,GPIO_PIN_2);
    id |= gpio_input_bit_get(GPIOD,GPIO_PIN_3) << 1;
    id |= gpio_input_bit_get(GPIOG,GPIO_PIN_6) << 2;
    
    return id;
}

/*!
    \brief      setup LCD parameters by module ID
    \param[in]  id: LCD module hardware ID
    \param[out] none
    \retval     rgblcd_error_enum
*/
static rgblcd_error_enum rgblcd_setup_param_by_id(uint8_t id)
{
    uint8_t index;

    for (index=0; index < (sizeof(rgblcd_param) / sizeof(rgblcd_param[0])); index++)
    {
        if (id == rgblcd_param[index].id)
        {
          rgblcd_status.param = &rgblcd_param[index];
          return RGBLCD_OK;
        }
    }

    return RGBLCD_PARAMETER_INVALID;
}

/*!
    \brief      transform coordinates based on display direction
    \param[in]  x: pointer to X coordinate value (modified in place)
    \param[in]  y: pointer to Y coordinate value (modified in place)
    \param[out] none
    \retval     none
*/
static void rgblcd_pos_transform( uint16_t *x, uint16_t *y)
{
    uint16_t x_target;
    uint16_t y_target;

    switch (rgblcd_status.disp_dir)
    {
        case RGBLCD_DISP_DIR_0:
            x_target = *x;
            y_target = *y;
            break;
        case RGBLCD_DISP_DIR_90:
            x_target = rgblcd_status.param->width - *y - 1;
            y_target = *x;
            break;
        case RGBLCD_DISP_DIR_180:
            x_target = rgblcd_status.param->width - *x - 1;
            y_target = rgblcd_status.param->height - *y - 1;
            break;
        case RGBLCD_DISP_DIR_270:
            x_target = *y;
            y_target = rgblcd_status.param->height - *x - 1;
            break;
    }
  
    *x = x_target;
    *y = y_target;
}

/*!
    \brief      initialize IPA (Image Processing Accelerator) for memory operations
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void rgblcd_ipa_init(void)
{
    rcu_periph_clock_enable(RCU_IPA);
    ipa_deinit();
    
    IPA_CTL |= IPA_CTL_TST;             /* transfer stop */
    IPA_CTL &= ~(IPA_CTL_PFCM);         /*!< memory-to-memory pixel format conversion mode */
    IPA_FPCTL &= ~IPA_FPCTL_FPF;
    IPA_FPCTL |= FOREGROUND_PPF_RGB565; /*!< foreground pixel format RGB565 */
    IPA_DPCTL &= ~IPA_DPCTL_DPF;
    IPA_DPCTL |= IPA_DPF_RGB565;        /*!< destination pixel format RGB565 */
    IPA_ITCTL |= IPA_ITCTL_ITEN;
    IPA_ITCTL |= ((uint32_t)0xFF << 8U);
}

/*!
    \brief      initialize RGB LCD display
    \param[in]  none
    \param[out] none
    \retval     rgblcd_error_enum
*/
rgblcd_error_enum rgblcd_init(void)
{
    uint8_t id;
    rgblcd_error_enum status = RGBLCD_OK;

    rgblcd_pwr_gpio_config();                                   /* configure RGB LCD power GPIO */
    delay_xms(100);
    
    id = rgblcd_get_id();                                       /* get LCD module hardware ID */
    status = rgblcd_setup_param_by_id(id);                      /* setup TLI parameters by ID */
    
    if(status)
    {
        PRINT_ERROR("get rgblcd parameter unsuccessfully\r\n");
        return status;
    }
    
    rgblcd_blk_config();                                        /* configure backlight PWM */
    rgblcd_tli_gpio_config();                                   /* configure RGB LCD GPIO pins */
    
    rgblcd_tli_config(rgblcd_status.param->width, \
                      rgblcd_status.param->height, \
                      &rgblcd_status.param->timing);     /* configure TLI peripheral */
    
    rgblcd_status.fb = (void *)RGBLCD_LAYER_FB_ADDR;            /* set frame buffer base address */
    rgblcd_status.back_color = WHITE;                           /* initialize background color */
    rgblcd_set_disp_dir(RGBLCD_DISP_DIR_0);           /* set display direction */
    rgblcd_blk_set(50);                                  /* set LCD backlight, default 50% brightness */
    rgblcd_ipa_init();
    rgblcd_clear(WHITE);
    
    #if RGBLCD_USING_TOUCH
    status = rgblcd_touch_init(rgblcd_status.param->touch_type);
    
    if(status)
    {
        PRINT_ERROR("initialize rgblcd touch unsuccessfully\r\n");
        return status;
    }
    #endif
    
    rgblcd_status.rgblcd_state = true;                          /* mark RGB LCD as initialized */
    PRINT_INFO("initialize rgblcd successfully\r\n");
    rgblcd_info_print();
    
    return status;
}

/*!
    \brief      print RGB LCD module information
    \param[in]  none
    \param[out] none
    \retval     none
*/
void rgblcd_info_print(void)
{
    PRINT_INFO("print rgblcd information>>\r\n");
    PRINT_INFO("/*********************************************************************/\r\n");
    PRINT_INFO("rgblcd pid: 0x%04X\r\n", rgblcd_status.param->pid);
    PRINT_INFO("rgblcd width: %u\r\n", rgblcd_status.param->width);
    PRINT_INFO("rgblcd height: %u\r\n", rgblcd_status.param->height);
    PRINT_INFO("rgblcd clock frequency: %u MHz\r\n", (uint32_t)(rgblcd_status.param->timing.clock_freq * 1e-6));
    PRINT_INFO("rgblcd touch type: 0x%02X\r\n", rgblcd_status.param->touch_type);
    PRINT_INFO("/*********************************************************************/\r\n");
}

/*!
    \brief      turn on RGB LCD display
    \param[in]  none
    \param[out] none
    \retval     none
*/
void rgblcd_display_on(void)
{
    rgblcd_pwr_set(SET);
    rgblcd_blk_set(60);
    tli_enable();
}

/*!
    \brief      turn off RGB LCD display
    \param[in]  none
    \param[out] none
    \retval     none
*/
void rgblcd_display_off(void)
{
    tli_disable();
    rgblcd_blk_set(0);
    rgblcd_pwr_set(RESET);
}

/*!
    \brief      set RGB LCD power state
    \param[in]  value: power state (SET or RESET)
    \param[out] none
    \retval     none
*/
void rgblcd_pwr_set(bit_status value)
{
    gpio_bit_write(GPIOD, GPIO_PIN_4, value);
}

/*!
    \brief      set RGB LCD backlight brightness
    \param[in]  value: brightness level (0-100)
    \param[out] none
    \retval     none
*/
void rgblcd_blk_set(uint8_t value)
{
    if(value <= 100)
    {
       timer_channel_output_pulse_value_config(TIMER2, TIMER_CH_2, value*10);
    }
}

/*!
    \brief      set RGB LCD display direction
    \param[in]  disp_dir: display direction (rgblcd_disp_dir_enum)
    \param[out] none
    \retval     rgblcd_error_enum
*/
rgblcd_error_enum rgblcd_set_disp_dir(rgblcd_disp_dir_enum disp_dir)
{
    switch (disp_dir)
    {
        case RGBLCD_DISP_DIR_0:
        case RGBLCD_DISP_DIR_180:     
            rgblcd_status.width  = rgblcd_status.param->width;
            rgblcd_status.height = rgblcd_status.param->height;
            break;
        case RGBLCD_DISP_DIR_90:      
        case RGBLCD_DISP_DIR_270:
            rgblcd_status.width  = rgblcd_status.param->height;
            rgblcd_status.height = rgblcd_status.param->width;
            break;
        default: return RGBLCD_PARAMETER_INVALID;
    }

    rgblcd_status.disp_dir = disp_dir;

    return RGBLCD_OK;
}

/*!
    \brief      fill rectangular area with solid color using IPA
    \param[in]  xs: start X coordinate
    \param[in]  ys: start Y coordinate  
    \param[in]  xe: end X coordinate
    \param[in]  ye: end Y coordinate
    \param[in]  color: fill color
    \param[out] none
    \retval     none
*/
void rgblcd_ipa_fill(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye, uint16_t color)
{
    uint32_t timeout = 6000000U;
    
    if(xe >= rgblcd_status.width)xe = rgblcd_status.width - 1;
    if(ye >= rgblcd_status.height)ye = rgblcd_status.height - 1;
    
    rgblcd_pos_transform(&xs, &ys); /* transform display direction coordinates */
    rgblcd_pos_transform(&xe, &ye);
    
    if (xs > xe)
    {
        xs = xs ^ xe;
        xe = xs ^ xe;
        xs = xs ^ xe;
    }
    
    if (ys > ye)
    {
        ys = ys ^ ye;
        ye = ys ^ ye;
        ys = ys ^ ye;
    }
    
    IPA_CTL |= IPA_CTL_TST;             /* transfer stop */
    IPA_CTL &= ~(IPA_CTL_PFCM);
    IPA_CTL |= IPA_FILL_UP_DE;          /*!< register to destination pixel format conversion */
    IPA_DLOFF = rgblcd_status.param->width - (xe - xs + 1);    /* destination line offset */
    IPA_DMADDR = (uint32_t)((uint16_t *)rgblcd_status.fb + (ys * rgblcd_status.param->width + xs));
    IPA_IMS = (((xe - xs + 1) << 16U) | (ye - ys + 1));
    IPA_DPV = (uint16_t)color;
    while(!(TLI_STAT & TLI_STAT_VS));
    IPA_CTL |= IPA_CTL_TEN; 
    while((IPA_INTF & IPA_FLAG_FTF) != IPA_FLAG_FTF)
    {
        timeout--;
        if(timeout == 0)break;
    }
    IPA_INTC |= IPA_FLAG_FTF;   /* clear transfer complete flag */
}

/*!
    \brief      fill rectangular area with color buffer using IPA
    \param[in]  xs: start X coordinate
    \param[in]  ys: start Y coordinate
    \param[in]  xe: end X coordinate  
    \param[in]  ye: end Y coordinate
    \param[in]  color_buffer: pointer to color data buffer
    \param[out] none
    \retval     none
*/
void rgblcd_ipa_color_fill(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye, void *color_buffer)
{
    uint32_t timeout = 6000000U;
    
    if(xe >= rgblcd_status.width)xe = rgblcd_status.width - 1;
    if(ye >= rgblcd_status.height)ye = rgblcd_status.height - 1;
    
    rgblcd_pos_transform(&xs, &ys); /* transform display direction coordinates */
    rgblcd_pos_transform(&xe, &ye);
    
    if (xs > xe)
    {
        xs = xs ^ xe;
        xe = xs ^ xe;
        xs = xs ^ xe;
    }
    
    if (ys > ye)
    {
        ys = ys ^ ye;
        ye = ys ^ ye;
        ys = ys ^ ye;
    }
    
    IPA_CTL |= IPA_CTL_TST;             /* transfer stop */
    IPA_CTL &= ~(IPA_CTL_PFCM);
    IPA_CTL |= IPA_FGTODE;              /*!< foreground memory to destination memory transfer with pixel format conversion */
    IPA_DLOFF = rgblcd_status.param->width - (xe - xs + 1);    /* destination line offset */
    IPA_DMADDR = (uint32_t)((uint16_t *)rgblcd_status.fb + (ys * rgblcd_status.param->width + xs));
    IPA_IMS = (((xe - xs + 1) << 16U) | (ye - ys + 1));
    IPA_FLOFF = 0;                                              /*!< foreground line offset */
    IPA_FMADDR = (uint32_t)color_buffer;                        /*!< foreground memory address */
    while(!(TLI_STAT & TLI_STAT_VS));
    IPA_CTL |= IPA_CTL_TEN; 
    while((IPA_INTF & IPA_FLAG_FTF) != IPA_FLAG_FTF)
    {
        timeout--;
        if(timeout == 0)break;
    }
    IPA_INTC |= IPA_FLAG_FTF;
}

/*!
    \brief      clear RGB LCD screen
    \param[in]  color: clear color
    \param[out] none
    \retval     none
*/
void rgblcd_clear(uint16_t color)
{
    rgblcd_ipa_fill(0, 0, rgblcd_status.width - 1, rgblcd_status.height - 1, color);
}

/*!
    \brief      draw a pixel point on RGB LCD
    \param[in]  x: X coordinate of the point
    \param[in]  y: Y coordinate of the point
    \param[in]  color: pixel color
    \param[out] none
    \retval     none
*/
void rgblcd_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    rgblcd_pos_transform(&x, &y);
    *(uint16_t *)((uint16_t *)rgblcd_status.fb + (y * rgblcd_status.param->width + x)) = (uint16_t)color;
}

/*!
    \brief      read pixel color at specified coordinates on RGB LCD
    \param[in]  x: X coordinate of the point
    \param[in]  y: Y coordinate of the point
    \param[out] none
    \retval     pixel color at specified coordinates
*/
uint16_t rgblcd_read_point(uint16_t x, uint16_t y)
{
    rgblcd_pos_transform(&x, &y);
    return *(uint16_t *)((uint16_t *)rgblcd_status.fb + (y * rgblcd_status.param->width + x));
}

/*!
    \brief      draw a line on RGB LCD
    \param[in]  xs: start X coordinate
    \param[in]  ys: start Y coordinate  
    \param[in]  xe: end X coordinate
    \param[in]  ye: end Y coordinate
    \param[in]  color: line color
    \param[out] none
    \retval     none
*/
void rgblcd_draw_line(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye, uint16_t color)
{
    uint16_t x_delta, y_delta;
    int16_t x_sign, y_sign;
    int16_t error1, error2;
    
    x_delta = (xs < xe) ? (xe - xs) : (xs - xe);
    y_delta = (ys < ye) ? (ye - ys) : (ys - ye);
    x_sign = (xs < xe) ? 1 : -1;
    y_sign = (ys < ye) ? 1 : -1;
    error1 = x_delta - y_delta;
  
    rgblcd_draw_point(xe, ye, color);

    while((xs != xe) || (ys != ye))
    {
        rgblcd_draw_point(xs, ys, color);

        error2 = error1 << 1;
        
        if(error2 > -y_delta)
        {
            error1 -= y_delta;
            xs += x_sign;
        }

        if(error2 < x_delta)
        {
            error1 += x_delta;
            ys += y_sign;
        }
    }
}

/*!
    \brief      draw a rectangle outline on RGB LCD
    \param[in]  xs: start X coordinate
    \param[in]  ys: start Y coordinate
    \param[in]  xe: end X coordinate  
    \param[in]  ye: end Y coordinate
    \param[in]  color: rectangle outline color
    \param[out] none
    \retval     none
*/
void rgblcd_draw_rect(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye, uint16_t color)
{
    rgblcd_draw_line(xs, ys, xe, ys, color);
    rgblcd_draw_line(xs, ye, xe, ye, color);
    rgblcd_draw_line(xs, ys, xs, ye, color);
    rgblcd_draw_line(xe, ys, xe, ye, color);
}

/*!
    \brief      draw a circle outline on RGB LCD
    \param[in]  x: center X coordinate
    \param[in]  y: center Y coordinate
    \param[in]  r: circle radius
    \param[in]  color: circle outline color
    \param[out] none
    \retval     none
*/
void rgblcd_draw_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color)
{
    int32_t x_t, y_t;
    int32_t error1, error2;

    x_t = -r;
    y_t = 0;
    error1 = 2 - 2 * r;
    
    do{
        rgblcd_draw_point(x - x_t, y + y_t, color);
        rgblcd_draw_point(x + x_t, y + y_t, color);
        rgblcd_draw_point(x + x_t, y - y_t, color);
        rgblcd_draw_point(x - x_t, y - y_t, color);
    
        error2 = error1;
        if(error2 <= y_t)
        {
            y_t++;
            error1 = error1 + (y_t * 2 + 1);
            if((-x_t == y_t) && (error2 <= x_t))error2 = 0;
        }
    
        if(error2 > x_t)
        {
            x_t++;
            error1 = error1 + (x_t * 2 + 1);
        }
    }while(x_t <= 0);
}

/*!
    \brief      display a single character on RGB LCD
    \param[in]  x: X coordinate for character display
    \param[in]  y: Y coordinate for character display
    \param[in]  ch: character to display
    \param[in]  font: character font size (rgblcd_font_enum)
    \param[in]  color: character color
    \param[out] none
    \retval     none
*/
void rgblcd_show_char(uint16_t x, uint16_t y, char ch, rgblcd_font_enum font, uint16_t color)
{
    const uint8_t *ch_code;
    uint8_t ch_width, ch_height, ch_size, ch_offset;
    uint8_t byte_index, byte_code, bit_index;
    uint8_t width_index = 0, height_index = 0;
    
    ch_offset = ch - ' ';
    
    switch(font)
    {
        #if(LCD_FONT_12 != 0)
        case RGBLCD_FONT_12:
            ch_code = lcd_font_1206[ch_offset];
            ch_width = LCD_FONT_12_CHAR_WIDTH;
            ch_height = LCD_FONT_12_CHAR_HEIGHT;
            ch_size = LCD_FONT_12_CHAR_SIZE;
            break;
        #endif
        #if(LCD_FONT_16 != 0)
        case RGBLCD_FONT_16:
            ch_code = lcd_font_1608[ch_offset];
            ch_width = LCD_FONT_16_CHAR_WIDTH;
            ch_height = LCD_FONT_16_CHAR_HEIGHT;
            ch_size = LCD_FONT_16_CHAR_SIZE;
            break;
        #endif
        #if(LCD_FONT_24 != 0)
        case RGBLCD_FONT_24:
            ch_code = lcd_font_2412[ch_offset];
            ch_width = LCD_FONT_24_CHAR_WIDTH;
            ch_height = LCD_FONT_24_CHAR_HEIGHT;
            ch_size = LCD_FONT_24_CHAR_SIZE;
            break;
        #endif
        #if(LCD_FONT_32 != 0)
        case RGBLCD_FONT_32:
            ch_code = lcd_font_3216[ch_offset];
            ch_width = LCD_FONT_32_CHAR_WIDTH;
            ch_height = LCD_FONT_32_CHAR_HEIGHT;
            ch_size = LCD_FONT_32_CHAR_SIZE;
            break;
        #endif
        default: return;
    }
    
    if((x + ch_width > rgblcd_status.width ) || (y + ch_height > rgblcd_status.height))
    {
        return;
    }
    
    for(byte_index = 0; byte_index < ch_size; byte_index++)
    {
        byte_code = ch_code[byte_index];
        for(bit_index = 0; bit_index < 8; bit_index++)
        {
            if((byte_code & 0x80) != 0)
            {
                rgblcd_draw_point(x + width_index, y + height_index, color);
            }
            else
            {
                rgblcd_draw_point(x + width_index, y + height_index, rgblcd_status.back_color);
            }
            
            width_index++;
            
            if(width_index == ch_width)
            {
                width_index = 0;
                height_index++;
                break;
            }
            
            byte_code <<= 1;
        }
  }
}

/*!
    \brief      display a string on RGB LCD with automatic line wrapping
    \param[in]  x: start X coordinate for string display
    \param[in]  y: start Y coordinate for string display
    \param[in]  width: display area width
    \param[in]  height: display area height
    \param[in]  str: string to display
    \param[in]  font: string font size (rgblcd_font_enum)
    \param[in]  color: string color
    \param[out] none
    \retval     none
*/
void rgblcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, char *str, rgblcd_font_enum font, uint16_t color)
{
    uint8_t ch_width, ch_height;
    uint16_t x_raw, y_raw;
    uint16_t x_limit, y_limit;
  
    switch(font)
    {
        #if(LCD_FONT_12 != 0)
        case RGBLCD_FONT_12:
            ch_width = LCD_FONT_12_CHAR_WIDTH;
            ch_height = LCD_FONT_12_CHAR_HEIGHT;
            break;
        #endif
        #if(LCD_FONT_16 != 0)
        case RGBLCD_FONT_16:
            ch_width = LCD_FONT_16_CHAR_WIDTH;
            ch_height = LCD_FONT_16_CHAR_HEIGHT;
            break;
        #endif
        #if(LCD_FONT_24 != 0)
        case RGBLCD_FONT_24:
            ch_width = LCD_FONT_24_CHAR_WIDTH;
            ch_height = LCD_FONT_24_CHAR_HEIGHT;
            break;
        #endif
        #if(LCD_FONT_32 != 0)
        case RGBLCD_FONT_32:
            ch_width = LCD_FONT_32_CHAR_WIDTH;
            ch_height = LCD_FONT_32_CHAR_HEIGHT;
            break;
        #endif
        default: return;
    }
    
    x_raw = x;
    y_raw = y;
    x_limit = ((x + width + 1) > rgblcd_status.width) ? rgblcd_status.width : (x + width + 1);
    y_limit = ((y + height + 1) > rgblcd_status.height) ? rgblcd_status.height : (y + height + 1);
  
    while((*str >= ' ') && (*str <= '~'))
    {
        if(x + ch_width >= x_limit)
        {
            x = x_raw;
            y += ch_height;
        }

        if(y + ch_height >= y_limit)
        {
            y = x_raw;
            x = y_raw;
        }

        rgblcd_show_char(x, y, *str, font, color);

        x += ch_width;
        str++;
    }
}

/*!
    \brief      display a number on RGB LCD with optional leading zero control
    \param[in]  x: X coordinate for number display  
    \param[in]  y: Y coordinate for number display
    \param[in]  num: number to display
    \param[in]  len: number of digits to display
    \param[in]  mode: leading zero mode (RGBLCD_NUM_SHOW_NOZERO: no leading zeros, RGBLCD_NUM_SHOW_ZERO: with leading zeros)
    \param[in]  font: number font size (rgblcd_font_enum)
    \param[in]  color: number color
    \param[out] none
    \retval     none
*/
void rgblcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, rgblcd_num_mode_enum mode, rgblcd_font_enum font, uint16_t color)
{
  uint8_t ch_width;
  uint8_t len_index, num_index;
  uint8_t first_nozero = 0;
  char pad;
  
    switch(font)
    {
        #if(LCD_FONT_12 != 0)
        case RGBLCD_FONT_12:
            ch_width = LCD_FONT_12_CHAR_WIDTH;
            break;
        #endif
        #if(LCD_FONT_16 != 0)
        case RGBLCD_FONT_16:
            ch_width = LCD_FONT_16_CHAR_WIDTH;
            break;
        #endif
        #if(LCD_FONT_24 != 0)
        case RGBLCD_FONT_24:
            ch_width = LCD_FONT_24_CHAR_WIDTH;
            break;
        #endif
        #if(LCD_FONT_32 != 0)
        case RGBLCD_FONT_32:
            ch_width = LCD_FONT_32_CHAR_WIDTH;
            break;
        #endif
        default: return;
    }
  
    switch (mode)
    {
        case RGBLCD_NUM_SHOW_NOZERO:
            pad = ' ';
            break;

        case RGBLCD_NUM_SHOW_ZERO:
            pad = '0';
            break;
        default: return;
    }
  
    for (len_index=0; len_index<len; len_index++)
    {
        num_index = (num / (uint32_t)pow(10, len - len_index - 1)) % 10;
        if ((first_nozero == 0) && (len_index < (len - 1)))
        {
            if (num_index == 0)
            {
                rgblcd_show_char(x + ch_width * len_index, y, pad, font, color);
                continue;
            }
            else
            {
                first_nozero = 1;
            }
        }
        rgblcd_show_char(x + ch_width * len_index, y, num_index + '0', font, color);
    }
}

/*!
    \brief      display a number on RGB LCD without leading zeros
    \param[in]  x: X coordinate for number display
    \param[in]  y: Y coordinate for number display
    \param[in]  num: number to display
    \param[in]  len: number of digits to display
    \param[in]  font: number font size (rgblcd_font_enum)
    \param[in]  color: number color
    \param[out] none
    \retval     none
*/
void rgblcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, rgblcd_font_enum font, uint16_t color)
{
    rgblcd_show_xnum(x, y, num, len, RGBLCD_NUM_SHOW_NOZERO, font, color);
}
