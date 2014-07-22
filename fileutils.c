#include <inttypes.h>

#include "ff.h"
#include "config.h"

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
	return	  ((DWORD)(2014 - 1980) << 25)	/* Year 2013 */
			| ((DWORD)6 << 21)				/* Month 7 */
			| ((DWORD)1 << 16)				/* Mday 28 */
			| ((DWORD)0 << 11)				/* Hour 0 */
			| ((DWORD)0 << 5)				/* Min 0 */
			| ((DWORD)0 >> 1);				/* Sec 0 */
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

    if (pfile_info->fname[0] == '.') {
      continue;
    }

    if (!(pfile_info->fattrib & INVALID_FILE_ATTR)) {
      num_files++;
    }
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

    if (pfile_info->fname[0] == '.') {
      continue;
    }

    if (!(pfile_info->fattrib & INVALID_FILE_ATTR)) {
      if (cur_file_index == index) {
        return 1;
      }
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
