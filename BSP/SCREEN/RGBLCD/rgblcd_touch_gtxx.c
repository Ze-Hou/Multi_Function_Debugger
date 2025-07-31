/*!
    \file       rgblcd_touch_gtxx.c
    \brief      RGB LCD touch screen driver for Goodix GT series
    \version    1.0
    \date       2025-07-25
    \author     Ze-Hou
*/

#include "./SCREEN/RGBLCD/rgblcd_touch.h"
#include "./IIC/iic.h"
#include "./DELAY/delay.h"
#include <string.h>

#if (RGBLCD_USING_TOUCH != 0)

/* RGB LCD touch screen data register array */
static const uint16_t rgblcd_touch_tp_reg[RGBLCD_TOUCH_TP_MAX] = {
    RGBLCD_TOUCH_REG_TP1,
    RGBLCD_TOUCH_REG_TP2,
    RGBLCD_TOUCH_REG_TP3,
    RGBLCD_TOUCH_REG_TP4,
    RGBLCD_TOUCH_REG_TP5,
};

/* RGB LCD touch screen status structure */
static struct
{
    rgblcd_touch_iic_addr_enum iic_addr;
}rgblcd_touch_status_struct;

/*!
    \brief      initialize RGB LCD touch screen hardware
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void rgblcd_touch_hw_init(void)
{
    /* enable GPIO peripheral clocks */
    rcu_periph_clock_enable(RGBLCD_TOUCH_INT_GPIO_RCU);
    rcu_periph_clock_enable(RGBLCD_TOUCH_RST_GPIO_RCU);
    /* initialize INT pin as input with pull-up */
    gpio_mode_set(RGBLCD_TOUCH_INT_GPIO_PORT,GPIO_MODE_INPUT,GPIO_PUPD_PULLUP,RGBLCD_TOUCH_INT_GPIO_PIN);

    /* initialize RST pin as output with pull-down */
    gpio_mode_set(RGBLCD_TOUCH_RST_GPIO_PORT,GPIO_MODE_OUTPUT,GPIO_PUPD_PULLDOWN, RGBLCD_TOUCH_RST_GPIO_PIN);
    gpio_output_options_set(RGBLCD_TOUCH_RST_GPIO_PORT,GPIO_OTYPE_PP,GPIO_OSPEED_12MHZ, RGBLCD_TOUCH_RST_GPIO_PIN);
}

/*!
    \brief      reset RGB LCD touch screen hardware
    \param[in]  addr: I2C communication address to use after reset
    \param[out] none
    \retval     none
*/
static void rgblcd_touch_hw_reset(rgblcd_touch_iic_addr_enum addr)
{
    gpio_bit_reset(RGBLCD_TOUCH_RST_GPIO_PORT, RGBLCD_TOUCH_RST_GPIO_PIN);
    delay_xms(10);
    gpio_bit_set(RGBLCD_TOUCH_RST_GPIO_PORT, RGBLCD_TOUCH_RST_GPIO_PIN);
    rgblcd_touch_status_struct.iic_addr = addr;
}

/*!
    \brief      write data to RGB LCD touch screen register
    \param[in]  reg: register address to write
    \param[in]  buf: data buffer to write
    \param[in]  len: length of data to write
    \param[out] none
    \retval     rgblcd_touch_error_enum
*/
static rgblcd_touch_error_enum rgblcd_touch_write_reg(uint16_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t buf_index;
    uint8_t ret;

    iic_start(&iicDevice_2);
    iic_send_byte((rgblcd_touch_status_struct.iic_addr << 1) & 0xFE, &iicDevice_2);
    iic_wait_ack(&iicDevice_2);
    iic_send_byte((uint8_t)(reg >> 8) & 0xFF, &iicDevice_2);
    iic_wait_ack(&iicDevice_2);
    iic_send_byte((uint8_t)reg & 0xFF, &iicDevice_2);
    iic_wait_ack(&iicDevice_2);
  
    for(buf_index=0; buf_index<len; buf_index++)
    {
        iic_send_byte(buf[buf_index], &iicDevice_2);
        ret = iic_wait_ack(&iicDevice_2);
        if(ret != 0)
        {
            break;
        }
    }
    
    iic_stop(&iicDevice_2);
  
    if(ret != 0)
    {
        return RGBLCD_TOUCH_ERROR;
    }
  
    return RGBLCD_TOUCH_OK;
}

/*!
    \brief      read data from RGB LCD touch screen register
    \param[in]  reg: register address to read
    \param[in]  buf: data buffer to store read data
    \param[in]  len: length of data to read
    \param[out] none
    \retval     none
*/
static void rgblcd_touch_read_reg(uint16_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t buf_index;
  
    iic_start(&iicDevice_2);
    iic_send_byte((rgblcd_touch_status_struct.iic_addr << 1) & 0xFE, &iicDevice_2);
    iic_wait_ack(&iicDevice_2);
    iic_send_byte((uint8_t)(reg >> 8) & 0xFF, &iicDevice_2);
    iic_wait_ack(&iicDevice_2);
    iic_send_byte((uint8_t)reg & 0xFF, &iicDevice_2);
    iic_wait_ack(&iicDevice_2);
    iic_start(&iicDevice_2);
    iic_send_byte((rgblcd_touch_status_struct.iic_addr << 1) | 1, &iicDevice_2);
    iic_wait_ack(&iicDevice_2);

    for(buf_index=0; buf_index<len - 1; buf_index++)
    {
        buf[buf_index] = iic_read_byte(1, &iicDevice_2);
    }

    buf[buf_index] = iic_read_byte(0, &iicDevice_2);

    iic_stop(&iicDevice_2);
}

/*!
    \brief      software reset RGB LCD touch screen
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void rgblcd_touch_sw_reset(void)
{
    uint8_t dat;

    dat = 0x02;
    rgblcd_touch_write_reg(RGBLCD_TOUCH_REG_CTRL, &dat, 1);
    delay_xms(10);

    dat = 0x00;
    rgblcd_touch_write_reg(RGBLCD_TOUCH_REG_CTRL, &dat, 1);
}

/*!
    \brief      get RGB LCD touch screen product ID
    \param[in]  pid: buffer to store product ID (ASCII format)
    \param[out] none
    \retval     none
*/
static void rgblcd_touch_get_pid(char *pid)
{
    rgblcd_touch_read_reg(RGBLCD_TOUCH_REG_PID, (uint8_t *)pid, 4);
    pid[4] = '\0';
}

/*!
    \brief      initialize RGB LCD touch screen
    \param[in]  type: touch screen controller type
    \param[out] none
    \retval     rgblcd_touch_error_enum
*/
rgblcd_touch_error_enum rgblcd_touch_init(rgblcd_touch_enum type)
{
    char pid[5];

    if(type != TOUCH_GTXX)
    {
        return RGBLCD_TOUCH_ERROR;
    }
  
    rgblcd_touch_hw_init();
    rgblcd_touch_hw_reset(RGBLCD_TOUCH_IIC_ADDR_14);
    
    if(iicDevice_2.iic_state != true)
    {
        iic_init(&iicDevice_2);      /* initialize I2C interface */
    }
    
    rgblcd_touch_get_pid(pid);
    if((strcmp(pid, RGBLCD_TOUCH_PID0) != 0))
    {
        return RGBLCD_TOUCH_ERROR;
    }
    
    rgblcd_touch_sw_reset();
  
    return RGBLCD_TOUCH_OK;
}

/*!
    \brief      scan RGB LCD touch screen for touch points
    \param[in]  point: pointer to touch point information structure
    \param[in]  cnt: number of touch points to scan (1~RGBLCD_TOUCH_TP_MAX)
    \param[out] none
    \retval     0: no touch points detected
    \retval     other: actual number of touch points detected
    \note       this function should be called at intervals of 10ms or more
*/
uint8_t rgblcd_touch_scan(rgblcd_touch_point_struct *point, uint8_t cnt)
{
    uint8_t tp_info;
    uint8_t tp_cnt;
    uint8_t point_index;
    rgblcd_disp_dir_enum dir;
    uint8_t tpn_info[6];
    rgblcd_touch_point_struct point_raw;

    if ((cnt == 0) || (cnt > RGBLCD_TOUCH_TP_MAX))
    {
        return 0;
    }
  
    for (point_index=0; point_index<cnt; point_index++)
    {
        if (&point[point_index] == NULL)
        {
            return 0;
        }
    }
  
    rgblcd_touch_read_reg(RGBLCD_TOUCH_REG_TPINFO, &tp_info, sizeof(tp_info));
    if ((tp_info & RGBLCD_TOUCH_TPINFO_MASK_STA) == RGBLCD_TOUCH_TPINFO_MASK_STA)
    {
        tp_cnt = tp_info & RGBLCD_TOUCH_TPINFO_MASK_CNT;
        tp_cnt = (cnt < tp_cnt) ? cnt : tp_cnt;

        for (point_index=0; point_index < tp_cnt; point_index++)
        {
            rgblcd_touch_read_reg(rgblcd_touch_tp_reg[point_index], tpn_info, sizeof(tpn_info));
            point_raw.x = (uint16_t)(tpn_info[1] << 8) | tpn_info[0];
            point_raw.y = (uint16_t)(tpn_info[3] << 8) | tpn_info[2];
            point_raw.size = (uint16_t)(tpn_info[5] << 8) | tpn_info[4];

            dir = rgblcd_status.disp_dir;
            switch (dir)
            {
                case RGBLCD_DISP_DIR_0:
                    point[point_index].x = point_raw.x;
                    point[point_index].y = point_raw.y;
                    break;
                case RGBLCD_DISP_DIR_90:
                    point[point_index].x = point_raw.y;
                    point[point_index].y = rgblcd_status.height - point_raw.x;
                    break;
                case RGBLCD_DISP_DIR_180:
                    point[point_index].x = rgblcd_status.width - point_raw.x;
                    point[point_index].y = rgblcd_status.height - point_raw.y;
                    break;

                case RGBLCD_DISP_DIR_270:
                    point[point_index].x = rgblcd_status.width - point_raw.y;
                    point[point_index].y = point_raw.x;
                    break;
                
                default: break;
            }

            point[point_index].size = point_raw.size;
        }

        tp_info = 0;
        rgblcd_touch_write_reg(RGBLCD_TOUCH_REG_TPINFO, &tp_info, sizeof(tp_info));

        return tp_cnt;
    }

    return 0;
}
#endif
