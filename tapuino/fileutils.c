#include <inttypes.h>
#include <string.h>
#include "ff.h"
#include "config.h"
#include "fileutils.h"

#define DEFAULT_DIR "/"
#define INVALID_FILE_ATTR   (AM_VOL)


FATFS g_fs;
DIR g_dir;
FIL g_fil;
uint8_t g_fat_buffer[FAT_BUF_SIZE];

/*---------------------------------------------------------*/
/* User Provided RTC Function called by FatFs module       */

DWORD get_fattime (void) {
  /* Returns current time packed into a DWORD variable */
  return ((DWORD)(2014 - 1980) << 25) /* Year 2014 */
            | ((DWORD)6 << 21)        /* Month 6 */
            | ((DWORD)1 << 16)        /* Mday 1 */
            | ((DWORD)0 << 11)        /* Hour 0 */
            | ((DWORD)0 << 5)         /* Min 0 */
            | ((DWORD)0 >> 1);        /* Sec 0 */
}

int is_valid_file(FILINFO* pfile_info) {
  char* file_name;
  int len;
  file_name = pfile_info->lfname[0] ? pfile_info->lfname : pfile_info->fname;
  len = strlen(file_name);
  
  if ((pfile_info->fattrib & INVALID_FILE_ATTR)) {
    return 0;
  }

  if ((pfile_info->fattrib & AM_DIR)) {
    return (file_name[0] != '.');
  }
  
  if (len < 5) {
    return 0;
  }

  return 1;
}

int get_num_files(FILINFO* pfile_info) {
  int num_files = 0;
  if (!pfile_info) {
    return 0;
  }
  // rewind directory to first file
  if (f_readdir(&g_dir, 0) != FR_OK) {
    return 0;
  }
  
  while(1) {
    pfile_info->fname[0] = 0;
    if ((f_readdir(&g_dir, pfile_info) != FR_OK) || !pfile_info->fname[0]) {
      break;
    }

    if (!is_valid_file(pfile_info)) {
      continue;
    }

    num_files++;
  }
  return num_files;
}

int get_file_at_index(FILINFO* pfile_info, int index) {
  int cur_file_index = 0;

  if (!pfile_info) {
    return 0;
  }

  // rewind directory to first file
  if (f_readdir(&g_dir, 0) != FR_OK) {
    return 0;
  }

  while(1) {
    pfile_info->fname[0] = 0;
    if ((f_readdir(&g_dir, pfile_info) != FR_OK) || !pfile_info->fname[0]) {
      break;
    }

    if (!is_valid_file(pfile_info)) {
      continue;
    }

    if (cur_file_index == index) {
      return 1;
    }
    cur_file_index++;
  }
  return 0;  
}


FRESULT change_dir(char* dir) {
  FRESULT fr = FR_OK;
  if ((fr = f_chdir(dir)) == FR_OK) {
    return f_opendir( &g_dir, "." );
  }
  return fr;
}
