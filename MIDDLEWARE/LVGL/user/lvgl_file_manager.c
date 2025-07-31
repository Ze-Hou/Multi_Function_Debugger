#include "lvgl_file_manager.h"
#include "lvgl_main.h"
#include "./USART/usart.h"
#include "./MALLOC/malloc.h"
#include "lvgl_debugger.h"
#include <ctype.h>

lvgl_file_manager_struct lvgl_file_manager;

/* static function declarations */
static void lvgl_file_manager_scan_files(const char *path);
static void lvgl_file_manager_files_info(const char *path);
static void lvgl_file_manager_select_file(const char *file_ext);
static void lvgl_file_manager_delete_list(void);
static void lvgl_file_manager_delete_info_obj(void);

/**************************************************************
函数名称 ： closebtn_event_handler
功    能 ： 关闭按钮回调
参    数 ： e
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void closebtn_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    
    switch(code)
    {
        case LV_EVENT_CLICKED:
            lvgl_file_manager_delete();
            break;
        
        default: break;
    }
}

/**************************************************************
函数名称 ： backbtn_event_handler
功    能 ： 返回按钮回调
参    数 ： e
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void backbtn_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    char *temp;
    
    switch(code)
    {
        case LV_EVENT_CLICKED:
            temp = strrchr(lvgl_file_manager.fpath , '/');  /* 定位到上一级目录位置 */
        
            if(temp == NULL)    /* 当前目录为根目录 */
            {
                lvgl_file_manager_delete();
            }
            else
            {
                *temp = '\0';
                
                lvgl_file_manager_delete_list();
                lvgl_file_manager_delete_info_obj();
                lvgl_file_manager_scan_files(lvgl_file_manager.fpath);
                lvgl_file_manager_files_info(lvgl_file_manager.fpath);
            }
            break;
            
        default: break;
    }
}

/**************************************************************
函数名称 ： listbtn_event_handler
功    能 ： 列表按钮回调
参    数 ： e
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void listbtn_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    char path_temp[FF_LFN_BUF + 1];
    const char *path;
    char *temp;
    
    memcpy(path_temp, "/", strlen("/") + 1);
    path = lv_list_get_button_text(lvgl_file_manager.list, obj);
    strncat(path_temp, path, FF_LFN_BUF - strlen(path_temp));
    strncat(lvgl_file_manager.fpath, path_temp, FF_LFN_BUF - strlen(lvgl_file_manager.fpath));  /* 追加完整路径 */
    temp = strrchr(path_temp , '.');    /* 查找路径是否存在 '.'，若存在，则视当前路径为文件 */
    
    switch(code)
    {
        case LV_EVENT_SHORT_CLICKED:  /* 短按 */
            if(temp == NULL)                /* 路径不存在 '.' */
            {
                lvgl_file_manager_delete_list();
                lvgl_file_manager_scan_files(lvgl_file_manager.fpath);
            }
            else                            /* 路径存在 '.' */
            {
                if(lvgl_file_manager.file_selector == 0) /* 未进行文件选择 */
                {
                    /* 等待实现进一步功能 */
                    temp = strrchr(lvgl_file_manager.fpath , '/');
                    *temp = '\0';
                }
                else
                {
                    lvgl_file_manager_select_file(temp + 1);
                }
            }
            break;
            
        case LV_EVENT_LONG_PRESSED:
            lvgl_file_manager_delete_info_obj();
            lvgl_file_manager_files_info(lvgl_file_manager.fpath);
            temp = strrchr(lvgl_file_manager.fpath , '/');  /* 定位到上一级目录位置 */
            *temp = '\0';
            break;
        
        default: break;       
    }
}

/**************************************************************
函数名称 ： lvgl_file_manager_scan_files
功    能 ： 扫描磁盘文件
参    数 ： path: 扫描路径
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void lvgl_file_manager_scan_files(const char *path)
{
    lv_obj_t * listbtn;
    
    lvgl_file_manager.dir = (DIR *)mymalloc(SRAMIN, sizeof(DIR));
    lvgl_file_manager.fileinfo = (FILINFO *)mymalloc(SRAMIN, sizeof(FILINFO));
    if((!lvgl_file_manager.dir) || (!lvgl_file_manager.fileinfo))   /* 如果有内存申请失败，则退出 */
    {
        myfree(SRAMIN, lvgl_file_manager.dir);
        myfree(SRAMIN, lvgl_file_manager.fileinfo);
        
        return;
    }
    
    /*Create a list*/
    lvgl_file_manager.list = lv_list_create(lvgl_file_manager.main_obj);    /* 创建一个列表，用于显示当前路径下的文件 */
    lv_obj_set_size(lvgl_file_manager.list, lv_pct(60), lv_pct(90));
    lv_obj_set_style_pad_row(lvgl_file_manager.list, 0, 0);
    lv_obj_align(lvgl_file_manager.list, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    
    lv_list_add_text(lvgl_file_manager.list, path);                         /* 向列表头添加当前路径 */
    lv_obj_set_style_text_font(lvgl_file_manager.list, &lv_font_fzst_24, 0);/* 设置字体 */
    
    lvgl_file_manager.fresult = f_opendir(lvgl_file_manager.dir, path);     /* 打开当前目录 */
    if(lvgl_file_manager.fresult == FR_OK)
    {
        while(1)                                                            /* 打开成功，则轮询当前目录下的文件，并在列表添加 */
        {
            lvgl_file_manager.fresult = f_readdir(lvgl_file_manager.dir, lvgl_file_manager.fileinfo);
            if((lvgl_file_manager.fresult != FR_OK) || (lvgl_file_manager.fileinfo->fname[0] == 0)) break;  /* 轮询完，退出循环 */
            if(lvgl_file_manager.fileinfo->fattrib & AM_DIR)
            {
                listbtn = lv_list_add_button(lvgl_file_manager.list, LV_SYMBOL_DIRECTORY, lvgl_file_manager.fileinfo->fname);
            }
            else
            {
                listbtn = lv_list_add_button(lvgl_file_manager.list, LV_SYMBOL_FILE, lvgl_file_manager.fileinfo->fname);
            }
            lv_obj_set_style_text_font(listbtn, &lv_font_fzst_24, 0);
            lv_obj_add_event_cb(listbtn, listbtn_event_handler, LV_EVENT_SHORT_CLICKED, NULL);  /* 添加短按事件 */
            lv_obj_add_event_cb(listbtn, listbtn_event_handler, LV_EVENT_LONG_PRESSED, NULL);   /* 添加长按事件 */
        }
        f_closedir(lvgl_file_manager.dir);
    }

    myfree(SRAMIN, lvgl_file_manager.dir);                                  /* 释放内存 */
    myfree(SRAMIN, lvgl_file_manager.fileinfo);
}

/**************************************************************
函数名称 ： lvgl_file_manager_files_info
功    能 ： 获取磁盘文件信息
参    数 ： path: 文件路径
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void lvgl_file_manager_files_info(const char *path)
{
    lv_obj_t * titlelabel;
    lv_obj_t * table;
    uint16_t buffer_len;
    char * file_info_buffer;
    FATFS *fs;
    uint32_t free_clust = 0, total_sector = 0, free_sector;

    /* Create a info obj */
    lvgl_file_manager.info_obj = lv_obj_create(lvgl_file_manager.main_obj);    /* 创建一个基础对象，用于容纳文件属性信息 */
    lv_obj_set_size(lvgl_file_manager.info_obj, lv_pct(40), lv_pct(90));
    lv_obj_align(lvgl_file_manager.info_obj, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_pad_top(lvgl_file_manager.info_obj, 5, 0);
    lv_obj_set_style_pad_bottom(lvgl_file_manager.info_obj, 5, 0);
    lv_obj_set_style_pad_left(lvgl_file_manager.info_obj, 5, 0);
    lv_obj_set_style_pad_right(lvgl_file_manager.info_obj, 5, 0);
    
    titlelabel = lv_label_create(lvgl_file_manager.info_obj);                   /* 创建一个标签，显示当前文件路径 */
    lv_obj_set_size(titlelabel, lv_pct(100), lv_pct(10));
    lv_label_set_text(titlelabel, path);
    lv_label_set_long_mode(titlelabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_font(titlelabel, &lv_font_fzst_24, 0);
    lv_obj_align(titlelabel, LV_ALIGN_TOP_MID, 0, 0);
    
    table = lv_table_create(lvgl_file_manager.info_obj);                        /* 创建一个表格，用于显示文件属性信息 */
    lv_obj_set_size(table, lv_pct(100), lv_pct(90));
    lv_obj_set_style_text_font(table, &lv_font_fzst_24, 0);
    lv_obj_align(table, LV_ALIGN_BOTTOM_MID, 0, 0);
    
    if(strcmp(path, "C:") == 0)
    {
        lvgl_file_manager.fresult = f_getfree((const TCHAR *)path, (DWORD *)&free_clust, &fs);
        
        if(lvgl_file_manager.fresult == FR_OK)
        {
            total_sector = (fs->n_fatent - 2) * fs->csize;      /* 得到总扇区数 */
            free_sector = free_clust * fs->csize;               /* 得到空闲扇区数 */
            
            lv_table_set_cell_value(table, 0, 0, "total");
            buffer_len = snprintf(NULL, 0, "%.2fGB", ((double)total_sector / 2 / 1024 / 1024)) + 1;
            file_info_buffer = (char *)mymalloc(SRAMIN, buffer_len);
            if(file_info_buffer)
            {
                snprintf(file_info_buffer, buffer_len, "%.2fGB", ((double)total_sector / 2 / 1024 / 1024));
                lv_table_set_cell_value(table, 0, 1, file_info_buffer);
                myfree(SRAMIN, file_info_buffer);
            }
            
            lv_table_set_cell_value(table, 1, 0, "free");
            buffer_len = snprintf(NULL, 0, "%.2fGB", ((double)free_sector / 2 / 1024 / 1024)) + 1;
            file_info_buffer = (char *)mymalloc(SRAMIN, buffer_len);
            if(file_info_buffer)
            {
                snprintf(file_info_buffer, buffer_len, "%.2fGB", ((double)free_sector / 2 / 1024 / 1024));
                lv_table_set_cell_value(table, 1, 1, file_info_buffer);
                myfree(SRAMIN, file_info_buffer);
            }
        }
    }
    else
    {
        lvgl_file_manager.fileinfo = (FILINFO *)mymalloc(SRAMIN, sizeof(FILINFO));

        if(!lvgl_file_manager.fileinfo)                                             /* 如果有内存申请失败，则退出 */
        {      
            return;
        }
        
        lvgl_file_manager.fresult = f_stat(path, lvgl_file_manager.fileinfo);
    
        if(lvgl_file_manager.fresult == FR_OK)
        {
            lv_table_set_cell_value(table, 0, 0, "date");
            /* 格式化fatfs时间戳（日期） */
            buffer_len = snprintf(NULL, 0, "%04d.%02d.%02d", ((lvgl_file_manager.fileinfo->fdate >> 9 & 0x7F) + 1980), (lvgl_file_manager.fileinfo->fdate >> 5 & 0x0F), (lvgl_file_manager.fileinfo->fdate & 0x1F)) + 1;
            file_info_buffer = (char *)mymalloc(SRAMIN, buffer_len);
            if(file_info_buffer)
            {
                snprintf(file_info_buffer, buffer_len, "%04d:%02d:%02d", ((lvgl_file_manager.fileinfo->fdate >> 9 & 0x7F) + 1980), (lvgl_file_manager.fileinfo->fdate >> 5 & 0x0F), (lvgl_file_manager.fileinfo->fdate & 0x1F));
                lv_table_set_cell_value(table, 0, 1, file_info_buffer);
                myfree(SRAMIN, file_info_buffer);
            }
            
            lv_table_set_cell_value(table, 1, 0, "time");
            /* 格式化fatfs时间戳（时间） */
            buffer_len = snprintf(NULL, 0, "%02d:%02d:%02d", (lvgl_file_manager.fileinfo->ftime >> 11 & 0x1F), (lvgl_file_manager.fileinfo->ftime >> 5 & 0x3F), ((lvgl_file_manager.fileinfo->ftime & 0x1F) * 2)) + 1;
            file_info_buffer = (char *)mymalloc(SRAMIN, buffer_len);
            if(file_info_buffer)
            {
                snprintf(file_info_buffer, buffer_len, "%02d:%02d:%02d", (lvgl_file_manager.fileinfo->ftime >> 11 & 0x1F), (lvgl_file_manager.fileinfo->ftime >> 5 & 0x3F), ((lvgl_file_manager.fileinfo->ftime & 0x1F) * 2));
                lv_table_set_cell_value(table, 1, 1, file_info_buffer);
                myfree(SRAMIN, file_info_buffer);
            }
            
            lv_table_set_cell_value(table, 2, 0, "R/W");
            if(lvgl_file_manager.fileinfo->fattrib & AM_RDO)
            {
                lv_table_set_cell_value(table, 2, 1, "RO");
            }
            else
            {
                lv_table_set_cell_value(table, 2, 1, "RW");
            }
            
            if(!(lvgl_file_manager.fileinfo->fattrib & AM_DIR)) /*如果是文件，显示文件大小 */
            {
                lv_table_set_cell_value(table, 3, 0, "size");
                buffer_len = snprintf(NULL, 0, "%fKb", ((double)lvgl_file_manager.fileinfo->fsize / 1024)) + 1;
                file_info_buffer = (char *)mymalloc(SRAMIN, buffer_len);
                if(file_info_buffer)
                {
                    snprintf(file_info_buffer, buffer_len, "%.2fKb", ((double)lvgl_file_manager.fileinfo->fsize / 1024));
                    lv_table_set_cell_value(table, 3, 1, file_info_buffer);
                    myfree(SRAMIN, file_info_buffer);
                }
            }
        }
        
        myfree(SRAMIN, lvgl_file_manager.fileinfo); /* 释放内存 */
    }
}

/**************************************************************
函数名称 ： lvgl_file_manager_select_file
功    能 ： 选择相应的文件
参    数 ： file_ext: 文件扩展名
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void lvgl_file_manager_select_file(const char *file_ext)
{
    uint8_t i = 0;
    char path_temp[FF_LFN_BUF + 1];
    char *temp;
    char file_ext_temp[5];
    file_selector_enum file_selector_temp = lvgl_file_manager.file_selector;
    
    /* 暂存选择路径，并将路径恢复到选择之前 */
    memcpy(path_temp, lvgl_file_manager.fpath, strlen(lvgl_file_manager.fpath) + 1);
    temp = strrchr(lvgl_file_manager.fpath , '/');
    *temp = '\0';
    
    memset(file_ext_temp, 0x00, sizeof(file_ext_temp));
    for(i = 0; i < strlen(file_ext); i++)
    {
        file_ext_temp[i] = toupper(file_ext[i]);
    }
    
    if(strcmp(lvgl_file_manager.file_ext, file_ext_temp) == 0)
    {
        lvgl_file_manager_delete();
        
        switch(file_selector_temp)
        {
            /* 是FLM算法文件 */
            case FILE_SELECTOR_FLM:
                lvgl_flm_prase(path_temp);
                break;
                
            case FILE_SELECTOR_BIN:
                bin_file_select_callback(path_temp);
                break;
                
            default: break;
        }
    }
    

}

/**************************************************************
函数名称 ： lvgl_file_manager_creat
功    能 ： 创建文件管理器
参    数 ： parent
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
void lvgl_file_manager_creat(lv_obj_t *parent)
{
    lv_obj_t *obj_title;
    lv_obj_t *titlelabel;
    lv_obj_t *obj_btn;
    lv_obj_t *btnlabel;
    lv_obj_t *closebtn;
    lv_obj_t *backbtn;
    lv_obj_t *listbtn;

    lvgl_file_manager.fpath = (char *)mymalloc(SRAMIN, FF_LFN_BUF + 1); /* 为文件管理器查询文件路径申请内存 */
    if(lvgl_file_manager.fpath == NULL)return;
    memcpy(lvgl_file_manager.fpath, "C:", strlen("C:") + 1);

    lvgl_file_manager.main_obj = lv_obj_create(parent);                 /* 创建文件管理器容器 */
    lv_obj_set_size(lvgl_file_manager.main_obj, lv_pct(100), lv_pct(100));
    lv_obj_center(lvgl_file_manager.main_obj);
    lv_obj_set_style_pad_top(lvgl_file_manager.main_obj, 5, 0);
    lv_obj_set_style_pad_bottom(lvgl_file_manager.main_obj, 5, 0);
    lv_obj_set_style_pad_left(lvgl_file_manager.main_obj, 5, 0);
    lv_obj_set_style_pad_right(lvgl_file_manager.main_obj, 5, 0);
    lv_obj_remove_flag(lvgl_file_manager.main_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_clip_corner(lvgl_file_manager.main_obj, true, 0);
    
    obj_title = lv_obj_create(lvgl_file_manager.main_obj);              /* 创建文件管理器标题容器 */
    lv_obj_set_size(obj_title, lv_pct(80), lv_pct(10));
    lv_obj_align(obj_title, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_pad_top(obj_title, 0, 0);
    lv_obj_set_style_pad_bottom(obj_title, 0, 0);
    lv_obj_set_style_pad_left(obj_title, 5, 0);
    lv_obj_set_style_pad_right(obj_title, 5, 0);
    lv_obj_remove_flag(obj_title, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_side(obj_title, LV_BORDER_SIDE_NONE, 0);

    titlelabel = lv_label_create(obj_title);                            /* 创建文件管理器标题 */
    lv_label_set_text(titlelabel, "文件管理器");
    lv_obj_set_style_text_font(titlelabel, &lv_font_fzst_32, 0);
    lv_obj_align(titlelabel, LV_ALIGN_CENTER, 0, 0);
    
    obj_btn = lv_obj_create(lvgl_file_manager.main_obj);                /* 创建文件管理器按钮容器 */
    lv_obj_set_size(obj_btn, lv_pct(20), lv_pct(10));
    lv_obj_align(obj_btn, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_pad_top(obj_btn, 0, 0);
    lv_obj_set_style_pad_bottom(obj_btn, 0, 0);
    lv_obj_set_style_pad_left(obj_btn, 5, 0);
    lv_obj_set_style_pad_right(obj_btn, 5, 0);
    lv_obj_remove_flag(obj_btn, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_side(obj_btn, LV_BORDER_SIDE_NONE, 0);
     
    closebtn = lv_button_create(obj_btn);                               /* 创建文件管理器关闭按钮 */
    lv_obj_add_event_cb(closebtn, closebtn_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_size(closebtn, lv_pct(50), lv_pct(100));
    lv_obj_align(closebtn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_margin_all(closebtn, 5, 0);
    
    btnlabel = lv_label_create(closebtn);
    lv_label_set_text(btnlabel, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_font(btnlabel, &lv_font_fzst_24, 0);
    lv_obj_center(btnlabel);
    
    backbtn = lv_button_create(obj_btn);                                /* 创建文件管理器返回按钮 */
    lv_obj_add_event_cb(backbtn, backbtn_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_size(backbtn, lv_pct(50), lv_pct(100));
    lv_obj_align(backbtn, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_margin_all(backbtn, 5, 0);
    
    btnlabel = lv_label_create(backbtn);
    lv_label_set_text(btnlabel, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_font(btnlabel, &lv_font_fzst_24, 0);
    lv_obj_center(btnlabel);
    
    lvgl_file_manager_scan_files(lvgl_file_manager.fpath);
    lvgl_file_manager_files_info(lvgl_file_manager.fpath);
    
    main_menu_page_flag |= 0x01;
}

/**************************************************************
函数名称 ： lvgl_file_manager_delete
功    能 ： 删除文件管理器
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
void lvgl_file_manager_delete(void)
{
    myfree(SRAMIN, lvgl_file_manager.fpath);
    lv_obj_delete(lvgl_file_manager.main_obj);
    lvgl_file_manager.main_obj = NULL;
    lvgl_file_manager.list = NULL;
    lvgl_file_manager.info_obj = NULL;
    lvgl_file_manager.file_selector = FILE_SELECTOR_OFF;
    memset(lvgl_file_manager.file_ext, '\0', sizeof(lvgl_file_manager.file_ext));
    main_menu_page_flag &= ~0x01;
    if(main_menu_page_flag == 0)
    {
        lvgl_show_main_menu();
    }
}

/**************************************************************
函数名称 ： lvgl_file_manager_delete
功    能 ： 删除文件列表
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void lvgl_file_manager_delete_list(void)
{
    lv_obj_delete(lvgl_file_manager.list);
    lvgl_file_manager.list = NULL;
}

/**************************************************************
函数名称 ： lvgl_file_manager_delete
功    能 ： 删除文件属性栏
参    数 ： 无
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
static void lvgl_file_manager_delete_info_obj(void)
{
    lv_obj_delete(lvgl_file_manager.info_obj);
    lvgl_file_manager.info_obj = NULL;    
}