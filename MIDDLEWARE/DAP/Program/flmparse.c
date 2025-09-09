/*
 * flmparse.c
 *
 *  Created on: 2021年4月10日
 *      Author: hello
 */
/* 基于原作者hello的flmparse.c修改 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include "elf.h"
#include "flash_blob.h"
#include "FlashOS.h"
#include "./MALLOC/malloc.h"
#include "./USART/usart.h"
#include "./FATFS/fatfs_config.h"

FlashDeviceStruct flash_device; /* 目标flash信息 */
program_target_t flash_algo;    /* 目标flash算法信息 */
static uint8_t *buffer;

#define FLM_PRASE_INFO_PRINT    1                /* 控制是否打印解析信息 */ 
#define MCU_RAM_BASE_ADDR       0x20000000      /* 单片机默认内存基地址 */

#define LOAD_FUN_NUM 5

const char* StrFunNameTable[LOAD_FUN_NUM] = {
	"Init",
	"UnInit",
	"EraseChip",
	"EraseSector",
	"ProgramPage"
};

/*
FLM算法4K空间分布：
+------------------------------------------------------------------------+
| Algo Blob  | Program Buffer              | Static Data | Stack Pointer |
+------------------------------------------------------------------------+
| < 1K       | 1K             |      1K    | 1K          | 1K            |
+------------------------------------------------------------------------+
| 0x20000000 | 0x20000400     | 0x20000800 | 0x20000C00  | 0x20001000    |
+------------------------------------------------------------------------+

FLM算法2K空间分布：
+-----------------------------------------------------------+
| Algo Blob  | Program Buffer | Static Data | Stack Pointer |
+-----------------------------------------------------------+
| < 1K       | 512 Bytes      | 256 Bytes   | 256 Bytes     |
+-----------------------------------------------------------+
| 0x20000000 | 0x20000400     | 0x20000600  | 0x20000800    |
+-----------------------------------------------------------+
*/

static int ReadDataFromFile(const char* fpath, uint32_t offset, void* buf, uint32_t size)
{
    FRESULT fresult;
    FIL *file;
    uint32_t read_bytes;
    
    file = (FIL *)mymalloc(SRAMIN, sizeof(FIL));
    if(file == NULL)return -1;
    
    fresult = f_open(file, fpath, FA_READ);
    if(fresult != FR_OK)
    {
        goto __exit;
    }
    fresult = f_lseek(file, offset);
    if(fresult != FR_OK)
    {
        goto __exit;
    }
    fresult = f_read(file, buf, size, &read_bytes);
    if(fresult != FR_OK)
    {
        goto __exit;
    }
    
__exit:
    f_close(file);
    myfree(SRAMIN, file);
    return fresult;
}

/**************************************************************
函数名称 ： FLM_Prase
功    能 ： FLM下载算法文件解析
参    数 ： fpath: 文件路径  
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
int FLM_Prase(const void *fpath)
{
	int i = 0, j = 0;
	int found = 0;
	const Elf32_Phdr* pPhdr;
	const Elf32_Shdr* pShdr;
	const Elf32_Sym* pSymbol;
	Elf32_Ehdr ehdr = {0};      // ELF文件信息头
	Elf32_Shdr ShdrSym = {0};   // 符号表头
	Elf32_Shdr ShdrStr = {0};   // 字符串表头
	int StrFunIndexTable[LOAD_FUN_NUM] = {0};

	for(i = 0; i < LOAD_FUN_NUM; i++)
	{
		StrFunIndexTable[i] = -1;
	}

	/* 读取ELF文件头信息（ELF Header） */
	ReadDataFromFile(fpath, 0, &ehdr, sizeof(Elf32_Ehdr));

	/* 不是ELF格式文件 */
	if (strstr((const char *)ehdr.e_ident, "ELF") == NULL)
	{
		return -1;
	}

	/* 读取程序头信息（Program Header） */
    buffer = mymalloc(SRAMIN, sizeof(Elf32_Phdr) * ehdr.e_phnum);
    if(buffer == NULL)
    {
        return -2;
    }
    pPhdr = (const Elf32_Phdr *)buffer;
	ReadDataFromFile(fpath, ehdr.e_phoff, buffer, sizeof(Elf32_Phdr) * ehdr.e_phnum);

	for (i = 0; i < ehdr.e_phnum; i++)
	{
		if (pPhdr[i].p_type == PT_LOAD && (pPhdr[i].p_flags & (PF_X | PF_W | PF_R)) == (PF_X | PF_W | PF_R))
		{
            flash_algo.algo_blob = (uint32_t *)mymalloc(SRAMIN, pPhdr[i].p_filesz + 32); /* 还需要加上8个中断halt程序 */
            if(flash_algo.algo_blob == NULL)
            {
                return -2;
            }
			if(ReadDataFromFile(fpath, pPhdr[i].p_offset, (uint32_t *)(flash_algo.algo_blob + 8), pPhdr[i].p_filesz) != 0)  // 提取需要下载到RAM的程序代码
			{
				return -3;
			}
			flash_algo.algo_size = pPhdr[i].p_filesz + 32;
		}
		else if ((pPhdr[i].p_type == PT_LOAD)&&(i == 1))
        {
            if(ReadDataFromFile(fpath, pPhdr[i].p_offset, &flash_device, offsetof(FlashDeviceStruct, sectors)) != 0)
            {
                return -3;
            }
            flash_device.sectors = (FlashSectorStruct *)mymalloc(SRAMIN, SECTOR_NUM * sizeof(FlashSectorStruct));
            if(flash_device.sectors == NULL)
            {
                return -2;
            }
            if(ReadDataFromFile(fpath, pPhdr[i].p_offset + offsetof(FlashDeviceStruct, sectors), flash_device.sectors, SECTOR_NUM * sizeof(FlashSectorStruct)) != 0)
            {
                return -3;
            }
        }
	}
    
	/* 读取节区头部（Sections Header） */
    myfree(SRAMIN, buffer);
    buffer = mymalloc(SRAMIN, sizeof(Elf32_Shdr) * ehdr.e_shnum);
    if(buffer == NULL)
    {
        return -2;
    }
    pShdr = (const Elf32_Shdr *) buffer;
	ReadDataFromFile(fpath, ehdr.e_shoff, buffer, sizeof(Elf32_Shdr) * ehdr.e_shnum);
	// 查找符号表头并拷贝出来备用
	for (i = 0; i < ehdr.e_shnum; i++)
	{
		if (pShdr[i].sh_type == SHT_SYMTAB)
		{
			memcpy(&ShdrSym, &pShdr[i], sizeof(Elf32_Shdr));

			// 查找字符串表头并拷贝出来备用
			if (pShdr[ShdrSym.sh_link].sh_type == SHT_STRTAB)
			{
				memcpy(&ShdrStr, &pShdr[ShdrSym.sh_link], sizeof(Elf32_Shdr));
				found = 1;
				break;
			}
		}
	}

	if(!found)
	{
		return -4;
	}

	/* 根据字符串表头读取所有字符串表 */
    myfree(SRAMIN, buffer);
    buffer = mymalloc(SRAMIN, ShdrStr.sh_size);
    if(buffer == NULL)
    {
        return -2;
    }
	ReadDataFromFile(fpath, ShdrStr.sh_offset, buffer, ShdrStr.sh_size);
	for (i = 0; i < ShdrStr.sh_size; i++)
	{
		if (buffer[i] == '\0')
		{
			buffer[i] = '\n';
		}
	}

	buffer[ShdrStr.sh_size] = 0;
#if 0
	PRINT_INFO("------------------<< Symbols >> -----------------\r\n");
	PRINT_INFO("%s\r\n", buffer);
	PRINT_INFO("-------------------------------------------------\r\n");
#endif
	for (i = 0; i < LOAD_FUN_NUM; i++)
	{
		char* p = NULL;

		if(StrFunNameTable[i] == NULL)
			continue;

		if((p = strstr((const char *) buffer, StrFunNameTable[i])) == NULL)
			continue;

		StrFunIndexTable[i] = (uint32_t) p - (uint32_t) buffer;
	}


	/* 读取符号表 */
    myfree(SRAMIN, buffer);
    buffer = mymalloc(SRAMIN, ShdrSym.sh_size);
    if(buffer == NULL)
    {
        return -2;
    }
    pSymbol = (const Elf32_Sym *) buffer;
	ReadDataFromFile(fpath, ShdrSym.sh_offset, buffer, ShdrSym.sh_size);

	// 遍历查询我们用到的函数符号
	for (i = 0; i < ShdrSym.sh_size / sizeof(Elf32_Sym); i++, pSymbol++)
	{
		for (j = 0; j < LOAD_FUN_NUM; j++)
		{
			if (StrFunIndexTable[j] >= 0 && StrFunIndexTable[j] == pSymbol->st_name)  // symbol.st_name的值就是偏移地址
			{
				switch (j)
				{
					case 0:
						flash_algo.init = pSymbol->st_value;
						break;
					case 1:
						flash_algo.uninit = pSymbol->st_value;
						break;
					case 2:
						flash_algo.erase_chip = pSymbol->st_value;
						break;
					case 3:
						flash_algo.erase_sector = pSymbol->st_value;
						break;
					case 4:
						flash_algo.program_page = pSymbol->st_value;
						break;
					default:
						break;
				}
			}
		}
	}
    
    myfree(SRAMIN, buffer);
	return 0;
}

/**************************************************************
函数名称 ： flm_prase
功    能 ： flm下载算法文件解析
参    数 ： fpath: 文件路径  
返 回 值 ： 无
作    者 ： ZeHou
**************************************************************/
int flm_prase(const char* fpath)
{
    int i = 0;

	if(FLM_Prase(fpath) < 0)
	{
        myfree(SRAMIN, flash_algo.algo_blob);
        myfree(SRAMIN, flash_device.sectors);
        myfree(SRAMIN, buffer);
		PRINT_INFO("错误：解析FLM格式文件失败，请检查FLM文件是否存在或格式正确性！\r\n");
		return -1;
	}

    /* 这8个数是中断halt程序，让函数执行完后返回到这里来执行从而让CPU自动halt住 */
	flash_algo.algo_blob[0] = 0xE00ABE00;
	flash_algo.algo_blob[1] = 0x062D780D;
	flash_algo.algo_blob[2] = 0x24084068;
	flash_algo.algo_blob[3] = 0xD3000040;
	flash_algo.algo_blob[4] = 0x1E644058;
	flash_algo.algo_blob[5] = 0x1C49D1FA;
	flash_algo.algo_blob[6] = 0x2A001E52;
	flash_algo.algo_blob[7] = 0x4770D1F2;
    
    flash_algo.init += (MCU_RAM_BASE_ADDR + 32);
	flash_algo.uninit += (MCU_RAM_BASE_ADDR + 32);
	flash_algo.erase_chip += (MCU_RAM_BASE_ADDR + 32);
	flash_algo.erase_sector += (MCU_RAM_BASE_ADDR + 32);
	flash_algo.program_page += (MCU_RAM_BASE_ADDR + 32);
    
    flash_algo.program_buffer = MCU_RAM_BASE_ADDR + ((flash_algo.algo_size % 0x400) ? ((flash_algo.algo_size / 0x400 + 1) * 0x400) : flash_algo.algo_size);
    flash_algo.algo_start = MCU_RAM_BASE_ADDR;
    flash_algo.program_buffer_size = flash_device.szPage;
    
    flash_algo.sys_call_s.breakpoint = MCU_RAM_BASE_ADDR + 1;
    flash_algo.sys_call_s.static_base = flash_algo.program_buffer + flash_algo.program_buffer_size;
    flash_algo.sys_call_s.stack_pointer = flash_algo.sys_call_s.static_base + 0x400;
    
    #if FLM_PRASE_INFO_PRINT
    PRINT_INFO("print flash algorithm information>>\r\n");
    PRINT_INFO("/*********************************************************************/\r\n");
    PRINT_INFO("algo_start: 0x%08X\r\n", flash_algo.algo_start);
	PRINT_INFO("init: 0X%08X\r\n",        flash_algo.init);
	PRINT_INFO("uninit: 0X%08X\r\n",      flash_algo.uninit);
	PRINT_INFO("erase_chip: 0X%08X\r\n",   flash_algo.erase_chip);
	PRINT_INFO("erase_sector: 0X%08X\r\n", flash_algo.erase_sector);
	PRINT_INFO("program_page:0X%08X\r\n", flash_algo.program_page);
	PRINT_INFO("breakpoint: 0x%08X\r\n", flash_algo.sys_call_s.breakpoint);
	PRINT_INFO("stack_base: 0x%08X\r\n", flash_algo.sys_call_s.static_base);
	PRINT_INFO("stack_pointer: 0x%08X\r\n", flash_algo.sys_call_s.stack_pointer);
	PRINT_INFO("program_buffer: 0x%08X\r\n", flash_algo.program_buffer);
	PRINT_INFO("program_buffer_size: 0x%08X\r\n", flash_algo.program_buffer_size);
    PRINT_INFO("vers: %d\r\n", flash_device.vers);
    PRINT_INFO("devName: %s\r\n", flash_device.devName);
    PRINT_INFO("devType: %d\r\n", flash_device.devType);
    PRINT_INFO("devAdr: 0x%08X\r\n", flash_device.devAdr);
    PRINT_INFO("szDev: %d\r\n", flash_device.szDev);
    PRINT_INFO("szPage: %d\r\n", flash_device.szPage);
    PRINT_INFO("/*********************************************************************/\r\n");
    #endif

	return 0;
}
