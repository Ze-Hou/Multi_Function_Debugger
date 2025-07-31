#include "lvgl.h"
#include "./USART/usart.h"
#include "./W25Q256/w25q256.h"
#include "./MALLOC/malloc.h"
#include "./FONT/fonts.h"
#include <string.h>
#include <stdio.h>

static const uint8_t opa4_table[16] = {0,  17, 34,  51,
                                       68, 85, 102, 119,
                                       136, 153, 170, 187,
                                       204, 221, 238, 255
                                      };

typedef struct{
    uint16_t min;
    uint16_t max;
    uint8_t  bpp;
    uint8_t  reserved[3];
}x_header_t;

typedef struct{
    uint32_t pos;
}x_table_t;

typedef struct{
    uint8_t adv_w;
    uint8_t box_w;
    uint8_t box_h;
    int8_t  ofs_x;
    int8_t  ofs_y;
    uint8_t r;
}glyph_dsc_t;

static x_header_t __g_xbf_hd = {
    .min = 0x0009,
    .max = 0xffe5,
    .bpp = 4,
};

static uint8_t *lvgl_font_buffer;

/**************************************************************
函数名称 ： lvgl_font_buffer_malloc
功    能 ： 为lvgl_font_buffer申请内存
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
void lvgl_font_buffer_malloc(void)
{
    lvgl_font_buffer = (uint8_t *)mymalloc(SRAMDTCM, 2176);
    if(lvgl_font_buffer)
    {
        PRINT_INFO("apply 2176 byte memory successfully in lvgl_font_buffer_malloc, address: 0x%08X\r\n", (uint32_t)lvgl_font_buffer);
    }
    else
    {
        PRINT_ERROR("apply 2176 byte memory unsuccessfully in lvgl_font_buffer_malloc\r\n");
    }
}
                                     
static uint8_t *__user_font_getdata(int32_t line_height, int offset, int size){
    uint32_t address;
    
    switch(line_height)
    {
        case 13:
            address = font_info.lvfontfzst12addr;
            break;
        
        case 14:
            address = font_info.lvfontsimsun12addr;
            break;
        
        case 17:
            address = font_info.lvfontfzst16addr;
            break;
        
        case 18:
            address = font_info.lvfontsimsun16addr;
            break;
        
        case 25:
            address = font_info.lvfontfzst24addr;
            break;
        
        case 27:
            address = font_info.lvfontsimsun24addr;
            break;
        
        case 34:
            address = font_info.lvfontfzst32addr;
            break;
        
        case 37:
            address = font_info.lvfontsimsun32addr;
            break;
        
        case 51:
            address = font_info.lvfontfzst48addr;
            break;
        
        case 55:
            address = font_info.lvfontsimsun48addr;
            break;
        
        case 59:
            address = font_info.lvfontfzst56addr;
            break;
        
        default: break;
    }

    w25q256_read_data(address + offset, lvgl_font_buffer, size);
    
    return lvgl_font_buffer;
}

static const void * __user_font_get_bitmap(lv_font_glyph_dsc_t * g_dsc, lv_draw_buf_t * draw_buf)
{
    uint32_t unicode_letter = g_dsc->gid.index;
    uint8_t * bitmap_out = draw_buf->data;
    const lv_font_t *font = g_dsc->resolved_font;

    lv_font_fmt_txt_dsc_t * fdsc = (lv_font_fmt_txt_dsc_t *) font->dsc;

    if(unicode_letter >__g_xbf_hd.max || unicode_letter<__g_xbf_hd.min ) {
        return NULL;
    }

    uint32_t unicode_offset = sizeof(x_header_t)+(unicode_letter-__g_xbf_hd.min)*4;
    uint32_t *p_pos = (uint32_t *)__user_font_getdata(font->line_height, unicode_offset, 4);
    if( p_pos[0] != 0 ) {
        uint32_t pos = p_pos[0];
        glyph_dsc_t * gdsc = (glyph_dsc_t*)__user_font_getdata(font->line_height, pos, sizeof(glyph_dsc_t));
        glyph_dsc_t gdscPoint = {0};
        memcpy(&gdscPoint, gdsc, sizeof(glyph_dsc_t));
        gdsc = &gdscPoint;
       
        int32_t gsize = (int32_t) gdsc->box_w * gdsc->box_h;
        if(gsize == 0) return NULL;
        if(fdsc->bitmap_format == LV_FONT_FMT_TXT_PLAIN) {
            const uint8_t * bitmap_in = __user_font_getdata(font->line_height, pos+sizeof(glyph_dsc_t), gdsc->box_w*gdsc->box_h*__g_xbf_hd.bpp/8);
            uint8_t * bitmap_out_tmp = bitmap_out;
            int32_t i = 0;
            int32_t x, y;
            uint32_t stride = lv_draw_buf_width_to_stride(gdsc->box_w, LV_COLOR_FORMAT_A8);

            if(fdsc->bpp == 4) {
                for(y = 0; y < gdsc->box_h; y ++) {
                    for(x = 0; x < gdsc->box_w; x++, i++) {
                        i = i & 0x1;
                        if(i == 0) {
                            bitmap_out_tmp[x] = opa4_table[(*bitmap_in) >> 4];
                        }
                        else if(i == 1) {
                            bitmap_out_tmp[x] = opa4_table[(*bitmap_in) & 0xF];
                            bitmap_in++;
                        }
                    }
                    bitmap_out_tmp += stride;
                }
            }
            return draw_buf;
        }
    }
    return NULL;
}


static bool __user_font_get_glyph_dsc(const lv_font_t * font, lv_font_glyph_dsc_t * dsc_out, uint32_t unicode_letter, uint32_t unicode_letter_next) {
    if( unicode_letter>__g_xbf_hd.max || unicode_letter<__g_xbf_hd.min ) {
        return NULL;
    }

    uint32_t unicode_offset = sizeof(x_header_t)+(unicode_letter-__g_xbf_hd.min)*4;
    uint32_t *p_pos = (uint32_t *)__user_font_getdata(font->line_height, unicode_offset, 4);
    if( p_pos[0] != 0 ) {
        glyph_dsc_t * gdsc = (glyph_dsc_t*)__user_font_getdata(font->line_height, p_pos[0], sizeof(glyph_dsc_t));
        dsc_out->adv_w = gdsc->adv_w;
        dsc_out->box_h = gdsc->box_h;
        dsc_out->box_w = gdsc->box_w;
        dsc_out->ofs_x = gdsc->ofs_x;
        dsc_out->ofs_y = gdsc->ofs_y;
        dsc_out->format   = __g_xbf_hd.bpp;
        dsc_out->gid.index = unicode_letter; //官方工具生成的字库赋的值就是uicode的id
        dsc_out->is_placeholder = false;
        return true;
    }
    return false;
}

//SimSun,,-1
//字模高度：14
//XBF字体,外部bin文件
const lv_font_t lv_font_simsun_12 = {
    .get_glyph_dsc = __user_font_get_glyph_dsc,
    .get_glyph_bitmap = __user_font_get_bitmap,
    .line_height = 14,
    .base_line = 0,
};

//SimSun,,-1
//字模高度：18
//XBF字体,外部bin文件
const lv_font_t lv_font_simsun_16 = {
    .get_glyph_bitmap = __user_font_get_bitmap,
    .get_glyph_dsc = __user_font_get_glyph_dsc,
    .line_height = 18,
    .base_line = 0,
};

//SimSun,,-1
//字模高度：27
//XBF字体,外部bin文件
const lv_font_t lv_font_simsun_24 = {
    .get_glyph_bitmap = __user_font_get_bitmap,
    .get_glyph_dsc = __user_font_get_glyph_dsc,
    .line_height = 27,
    .base_line = 0,
};

//SimSun,,-1
//字模高度：37
//XBF字体,外部bin文件
const lv_font_t lv_font_simsun_32 = {
    .get_glyph_bitmap = __user_font_get_bitmap,
    .get_glyph_dsc = __user_font_get_glyph_dsc,
    .line_height = 37,
    .base_line = 0,
};

//SimSun,,-1
//字模高度：55
//XBF字体,外部bin文件
const lv_font_t lv_font_simsun_48 = {
    .get_glyph_bitmap = __user_font_get_bitmap,
    .get_glyph_dsc = __user_font_get_glyph_dsc,
    .line_height = 55,
    .base_line = 0,
};

//FZShuTi,,-1
//字模高度：13
//XBF字体,外部bin文件
const lv_font_t lv_font_fzst_12 = {
    .get_glyph_bitmap = __user_font_get_bitmap,
    .get_glyph_dsc = __user_font_get_glyph_dsc,
    .line_height = 13,
    .base_line = 0,
};

//FZShuTi,,-1
//字模高度：17
//XBF字体,外部bin文件
const lv_font_t lv_font_fzst_16 = {
    .get_glyph_bitmap = __user_font_get_bitmap,
    .get_glyph_dsc = __user_font_get_glyph_dsc,
    .line_height = 17,
    .base_line = 0,
};

//FZShuTi,,-1
//字模高度：25
//XBF字体,外部bin文件
const lv_font_t lv_font_fzst_24 = {
    .get_glyph_bitmap = __user_font_get_bitmap,
    .get_glyph_dsc = __user_font_get_glyph_dsc,
    .line_height = 25,
    .base_line = 0,
};

//FZShuTi,,-1
//字模高度：34
//XBF字体,外部bin文件
const lv_font_t lv_font_fzst_32 = {
    .get_glyph_bitmap = __user_font_get_bitmap,
    .get_glyph_dsc = __user_font_get_glyph_dsc,
    .line_height = 34,
    .base_line = 0,
};

//FZShuTi,,-1
//字模高度：51
//XBF字体,外部bin文件
const lv_font_t lv_font_fzst_48 = {
    .get_glyph_bitmap = __user_font_get_bitmap,
    .get_glyph_dsc = __user_font_get_glyph_dsc,
    .line_height = 51,
    .base_line = 0,
};

//FZShuTi,,-1
//字模高度：59
//XBF字体,外部bin文件
const lv_font_t lv_font_fzst_56 = {
    .get_glyph_bitmap = __user_font_get_bitmap,
    .get_glyph_dsc = __user_font_get_glyph_dsc,
    .line_height = 59,
    .base_line = 0,
};
