/**
 * @file 
 * @author 
 * @version 
 * @date 
 * 
 * @copyright (c)
 * 
 */

#include "fatfs_tools.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "shell_ext.h"
#include "shell_port.h"
#include "shell_port.h"
#include "ff.h"
#include "api_flash.h"

#ifdef FATFS_ENABLE

extern FATFS g_fs; /* Work area (file system object) for logical drives */
extern FIL g_fnew; /* file objects */

int ls(int argc, char *argv[])
{
	TCHAR path[50];
	FILINFO fno;
	DIR dir;
	FRESULT res;
	char* fn;

	f_getcwd(path, sizeof(path));
	res = f_opendir(&dir, path);
	if(res == FR_OK)
	{
		while(1)
		{
			res = f_readdir(&dir, &fno);
			if(res != FR_OK || fno.fname[0]==0) break;
			if(fno.fname[0]=='.') continue;
			fn = fno.fname;

			LOG_RAW("%s\t\t%ld\r\n", fn, fno.fsize);
		}
	}
	return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, ls, ls, list files);

int fdisk(int argc, char *argv[])
{
	u32 total, free;
	exf_getfree("0:", &total, &free);
	if (argc == 1)
	{
		LOG_RAW("SPI Flash Total Size: %d KB\r\n", total);
		LOG_RAW("SPI Flash Free Size:  %d KB\r\n", free);
	}
	else if (argc == 2)
	{
		if (strcmp(argv[1], "-h") == 0)
		{
			LOG_RAW("SPI Flash Total Size: %0.3f MB\r\n", total / 1024.0f);
			LOG_RAW("SPI Flash Free Size:  %0.3f MB\r\n", free / 1024.0f);
		}
	}
	else
	{
		LOG_RAW("cmd parameter error\r\n");
	}
	return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, fdisk, fdisk, display flash capacity info);

int pwd(int argc, char *argv[])
{
	if (argc == 1)
	{
		TCHAR path[50];
		memset(path, 0, sizeof(path));
		if(FR_OK == f_getcwd(path, sizeof(path)))
		{
			LOG_RAW("%s\r\n", path);
		}
		else
		{
			LOG_RAW("f_getcwd error\r\n");
		}
	}
	return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, pwd, pwd, current dir);

int cd(int argc, char *argv[])
{
	TCHAR dir[50];
	if (argc == 2)
	{
		memset(dir, 0, sizeof(dir));
		if(f_getcwd(dir,sizeof(dir)) != FR_OK)
		{
			LOG_RAW("f_getcwd error\r\n");
		}

		if (argv[1][0] == '.')
		{
			if(argv[1][1] == '.')
			{
				for(int i=strlen(dir); i>0; i--)
				{
					if(dir[i-1]=='/')
					{
						break;
					}
					else
					{
						dir[i-1] = 0;
					}					
				}
			}
			else if(argv[1][1] == '/')
			{
				strcat(dir, &argv[1][1]);
			}
			else
			{
				return 0;
			}								
		}
		else if (argv[1][0] == '/')
		{
			memset(dir, 0, sizeof(dir));
			memcpy(dir, "0:", 2);
			strcat(dir, argv[1]);
		}
		else if ((argv[1][0] >= 'a' && argv[1][0] <= 'z') || (argv[1][0] >= 'A' && argv[1][0] <= 'Z') || argv[1][0] == '_')
		{
			strcat(dir, "/");
			strcat(dir, argv[1]);
		}
		else
		{
			LOG_RAW("no valid dir, dir start with . or / character.\r\n");
			return 0;
		}
		// LOG_RAW("dir: %s\r\n", dir);
		f_chdir(dir);
	}
	return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, cd, cd, change dir);

int mkdir(int argc, char *argv[])
{
	if (argc == 2)
	{
		f_mkdir(argv[1]);
	}
	return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, mkdir, mkdir, create dir);

int touch(int argc, char *argv[])
{
	FRESULT res;
	TCHAR dir[50];

	if (argc == 2)
	{
		memset(dir, 0, sizeof(dir));
		f_getcwd(dir,sizeof(dir));
		strcat(dir, "/");
		strcat(dir, argv[1]);
		res = f_open(&g_fnew, dir, FA_CREATE_ALWAYS | FA_WRITE);
		if (res == FR_OK)
		{
			LOG_RAW("create a new file %s", dir);
			f_close(&g_fnew);
		}
	}
	return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, touch, touch, create a file);

int rm(int argc, char *argv[])
{
	TCHAR dir[100];

	if (argc == 2)
	{
		memset(dir, 0, sizeof(dir));
		f_getcwd(dir,sizeof(dir));
		strcat(dir, "/");
		strcat(dir, argv[1]);
		f_open(&g_fnew, dir, FA_CREATE_ALWAYS | FA_WRITE);
		f_close(&g_fnew);
		f_unlink(dir);
	}
	return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, rm, rm, Delete an file or directory);

int cat(int argc, char *argv[])
{
	// FIL fnew;	/* file objects */
	TCHAR buff[10] = {0}; /* file copy buffer */
	UINT br = 0;			/* File R/W count */
	FRESULT res;
	TCHAR dir[100];

	if (argc == 2)
	{
		memset(dir, 0, sizeof(dir));
		f_getcwd(dir,sizeof(dir));
		strcat(dir, "/");
		strcat(dir, argv[1]);
		res = f_open(&g_fnew, dir, FA_OPEN_EXISTING | FA_READ);
		if (res == FR_OK)
		{
			do
			{
				memset(buff, 0, sizeof(buff));
				res = f_read(&g_fnew, buff, sizeof(buff), &br);
				if(res == FR_OK)
				{
					LOG_RAW("%s", buff);
				}
			}while(br == sizeof(buff));

			f_close(&g_fnew);
			LOG_RAW("\r\n");
		}
	}
	return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, cat, cat, show file in console);

int echo(int argc, char *argv[])
{
	TCHAR dir[100];

	if (argc == 4 && strcmp(argv[2], ">")==0)
	{
		memset(dir, 0, sizeof(dir));
		f_getcwd(dir, sizeof(dir));
		strcat(dir, "/");
		strcat(dir, argv[3]);
		f_open(&g_fnew, dir, FA_CREATE_ALWAYS | FA_WRITE);
		f_puts(argv[1], &g_fnew);
		f_puts("\r\n", &g_fnew);
		f_close(&g_fnew);
		LOG_RAW("\r\n");
	}
	else if(argc == 5 && strcmp(argv[1], "-a")==0 && strcmp(argv[3], ">")==0)
	{
		memset(dir, 0, sizeof(dir));
		f_getcwd(dir, sizeof(dir));
		strcat(dir, "/");
		strcat(dir, argv[4]);
		f_open(&g_fnew, dir, FA_OPEN_EXISTING | FA_WRITE);
		f_lseek(&g_fnew, g_fnew.fsize);
		f_puts(argv[2], &g_fnew);
		f_puts("\r\n", &g_fnew);
		f_close(&g_fnew);
		LOG_RAW("\r\n");
	}
	else
	{
		LOG_RAW("usage: \r\n");
		LOG_RAW("echo string > xxx.txt (clear and write)\r\n");
		LOG_RAW("echo -a string > xxx.txt (write at the end)\r\n");
	}
	
	return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, echo, echo, wrtie string to file);

int flashclear(int argc, char *argv[])
{
	f_mkfs("0:", 0, 4096);
	return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN, flashclear, flashclear, erase spi flash);

#endif
