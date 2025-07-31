/*!
    \file       lvgl_debugger.h
    \brief      LVGL debugger interface header file
    \version    1.0
    \date       2025-07-26
    \author     Ze-Hou
*/

#ifndef __LVGL_DEBUGGER_H
#define __LVGL_DEBUGGER_H
#include "lvgl_file_manager.h"
#include "flash_blob.h"
#include "FlashOS.h"
#include "SWD_host.h"
#include "SWD_flash.h"

extern FlashDeviceStruct flash_device;         /* target flash information */
extern program_target_t flash_algo;            /* target flash algorithm information */
extern int flm_prase(const char* fpath);       /* FLM file parser function */
extern volatile uint32_t download_file_size;   /* download file size */

/*!
    \brief      LVGL debugger interface structure
*/
typedef struct
{
    lv_obj_t *msgbox;                       /*!< Message box object */
    lv_obj_t *main_obj;                     /*!< Main object container */
    lv_obj_t *flm_file_label;               /*!< FLM file label */
    lv_obj_t *id_label;                     /*!< Target ID label */
    lv_obj_t *textarea;                     /*!< Text area for information */
    lv_obj_t *connect_btn;                  /*!< Connect button */
    lv_obj_t *connect_label;                /*!< Connect status label */
    lv_obj_t *address_textarea;             /*!< Address input textarea */
    lv_obj_t *size_textarea;                /*!< Size input textarea */
    lv_obj_t *read_btn;                     /*!< Read button */
    lv_obj_t *download_led;                 /*!< Download status LED */
    lv_obj_t *download_update_label;        /*!< Download update label */
    lv_obj_t *download_label;               /*!< Download progress label */
    lv_obj_t *download_bin_path_label;      /*!< Download BIN file path label */
    lv_obj_t *download_btn;                 /*!< Download button */
    
    lv_timer_t *timer;                      /*!< Update timer */
}lvgl_debugger_struct;
extern lvgl_debugger_struct lvgl_debugger;

/*!
    \brief      LVGL debugger download status structure
*/
typedef struct
{
    uint16_t run_count;                     /*!< Run count */
    uint8_t status;                         /*!< Download status */
    uint8_t error;                          /*!< Error code */
}lvgl_debugger_download_struct;

/* function declarations */
int debugger_read_bin_file(uint32_t offset, void* buf, uint32_t size, uint32_t *read_bytes);    /* read binary file */
void lvgl_debugger_off_line_creat(lv_obj_t *parent);                                           /* create debugger offline interface */
void lvgl_flm_prase(const char *fpath);                                                        /* parse FLM file */
void bin_file_select_callback(const char *fpath);                                              /* BIN file selection callback */
void lvgl_debugger_off_line_delete(void);                                                      /* delete debugger offline interface */

#endif /* __LVGL_DEBUGGER_H */
