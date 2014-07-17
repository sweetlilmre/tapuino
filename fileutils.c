#include <inttypes.h>

#include "pff.h"
#include "config.h"

#define DEFAULT_DIR "/"
#define INVALID_FILE_ATTR   (AM_LFN | AM_VOL)

FATFS g_fs;
DIR g_dir;
uint8_t g_fat_buffer[FAT_BUF_SIZE];

int get_num_files(FILINFO* pfile_info) {
  int num_files = 0;
  if (!pfile_info) {
    return 0;
  }
  // rewind directory to first file
  if (pf_readdir(&g_dir, 0) != FR_OK) {
    return 0;
  }
  
  while(1) {
    pfile_info->fname[0] = 0;
    if ((pf_readdir(&g_dir, pfile_info) != FR_OK) || !pfile_info->fname[0]) {
      break;
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
  if (pf_readdir(&g_dir, 0) != FR_OK) {
    return 0;
  }

  while(1) {
    pfile_info->fname[0] = 0;
    if ((pf_readdir(&g_dir, pfile_info) != FR_OK) || !pfile_info->fname[0]) {
      break;
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
  if ((fr = pf_chdir(dir)) == FR_OK) {
    return pf_opendir( &g_dir, "." );
  }
  return fr;
}
