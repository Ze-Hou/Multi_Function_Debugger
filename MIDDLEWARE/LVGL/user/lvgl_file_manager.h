/*!
    \file       lvgl_file_manager.h
    \brief      LVGL file manager interface header file
    \version    1.0
    \date       2025-07-26
    \author     Ze-Hou
*/

#ifndef __LVGL_FILE_MANAGER_H
#define __LVGL_FILE_MANAGER_H
#include "./FATFS/fatfs_config.h"
#include "lvgl.h"

/*!
    \brief      File selector mode enumeration
*/
typedef enum
{
    FILE_SELECTOR_OFF = 0,                              /*!< (0) View files mode */
    FILE_SELECTOR_FLM,                                  /*!< (1) Select FLM files mode */
    FILE_SELECTOR_BIN,                                  /*!< (2) Select BIN files mode */
}file_selector_enum;

/*!
    \brief      LVGL file manager structure
*/
typedef struct
{
    lv_obj_t *main_obj;                     /*!< Main object container */
    lv_obj_t *info_obj;                     /*!< Information object */
    lv_obj_t *list;                         /*!< File list object */
    char *fpath;                            /*!< File path pointer */
    DIR *dir;                               /*!< Directory pointer */
    FIL *file;                              /*!< File pointer */
    FILINFO *fileinfo;                      /*!< File information pointer */
    FRESULT fresult;                        /*!< File system result */
    file_selector_enum file_selector;      /*!< File selector mode */
    char file_ext[5];                       /*!< File extension, max 4 characters */
}lvgl_file_manager_struct;
extern lvgl_file_manager_struct lvgl_file_manager;

/*!
    \brief      File selector extension array
*/
static const char *const file_selector_file_ext[] = {
    "",                                     /*!< No filter */
    "FLM",                                  /*!< FLM file extension */
    "BIN",                                  /*!< BIN file extension */
};

/* function declarations */
void lvgl_file_manager_creat(lv_obj_t *parent);         /* create file manager interface */
void lvgl_file_manager_delete(void);                    /* delete file manager interface */
#endif /* __LVGL_FILE_MANAGER_H */
