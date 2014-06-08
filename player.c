#include <avr/io.h>
#include <util/delay.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "integer.h"
#include "spi.h"
#include "pff.h"
#include "diskio.h"
#include "player.h"

#define FILE_TYPE_UNK 0
#define FILE_TYPE_MP3 1
#define FILE_TYPE_WAV 2
#define FILE_TYPE_MID 3
#define FILE_TYPE_WMA 4

#define CMD_ARG_MAX 12
#define FAT_BUF_SIZE 512
#define DEFAULT_DIR "/"
#define INVALID_FILE_ATTR ( AM_DIR | AM_LFN | AM_VOL )

static FATFS fs;
static BYTE g_fatBuffer[ FAT_BUF_SIZE ];
static DIR g_dir;

BYTE file_type_from_filename( char* filename )
{
  if( strstr( filename, ".MP3") != 0)
  {
    return( FILE_TYPE_MP3 );
  }
  else if( strstr( filename, ".WAV") != 0)
  {
    return( FILE_TYPE_WAV );
  }
  else if( strstr( filename, ".MID") != 0)
  {
    return( FILE_TYPE_MID );
  }
  else if( strstr( filename, ".WMA") != 0)
  {
    return( FILE_TYPE_WMA );
  }
  return( FILE_TYPE_UNK );
}



int find_first_file( char* pFile, FILINFO* pfile_info )
{
  FRESULT res;

  if( !pfile_info )
  {
    return( 0 );
  }
  // rewind directory to first file
  pf_readdir( &g_dir, 0 );
  
  while( 1 )
  {
    pfile_info->fname[0] = 0;
    res = pf_readdir( &g_dir, pfile_info );
    if( res != FR_OK || !pfile_info->fname[0] )
    {
      break;
    }

    if( !( pfile_info->fattrib & INVALID_FILE_ATTR ) )
    {
      responseCallback( pfile_info->fname );
      if( file_type_from_filename( pfile_info->fname ) != FILE_TYPE_UNK )
      {
        if( !pFile || *pFile == 0 || !strncmp( pfile_info->fname, pFile, 12 ) )
        {
          // found the file we were looking for or a playable file
          return( 1 );
        }
      }
    }
  }
  return( 0 );
}

int find_next_file( FILINFO* pfile_info )
{
  FRESULT res;
  BYTE count = 0;

  if( !pfile_info )
  {
    return( 0 );
  }
  
  while( 1 )
  {
    pfile_info->fname[0] = 0;
    res = pf_readdir( &g_dir, pfile_info );
    if( res != FR_OK )
    {
      break;
    }
    
    if( !pfile_info->fname[0] )
    {
      if( count++ < 1 )
      {
        // rewind directory to first file and start from the top
        pf_readdir( &g_dir, 0 );
        continue;
      }
      return( 0 );
    }

    if( !( pfile_info->fattrib & INVALID_FILE_ATTR ) )
    {
      responseCallback( pfile_info->fname );
      if( file_type_from_filename( pfile_info->fname ) != FILE_TYPE_UNK )
      {
        return( 1 );
      }
    }
  }
  return( 0 );
}

FRESULT change_dir( char* path )
{
  FRESULT res;
  
  res = pf_chdir( path );
  if( res == FR_OK )
  {
    res = pf_opendir( &g_dir, "." );
  }
  return( res );
}

void printcode(int code)
{
  switch(code)
  {
    case 	FR_OK:
      responseCallback("FR_OK");
    break;
    case 	FR_DISK_ERR:
      responseCallback("FR_DISK_ERR");
    break;
    case 	FR_NOT_READY:
      responseCallback("FR_NOT_READY");
    break;
    case 	FR_NO_FILE:
      responseCallback("FR_NO_FILE");
    break;
    case 	FR_NO_PATH:
      responseCallback("FR_NO_PATH");
    break;
    case 	FR_NOT_OPENED:
      responseCallback("FR_NOT_OPENED");
    break;
    case 	FR_NOT_ENABLED:
      responseCallback("FR_NOT_ENABLED");
    break;
    case 	FR_NO_FILESYSTEM:
      responseCallback("FR_NO_FILESYSTEM");
    break;
    default:
      responseCallback("UNKNOWN ERROR");
    break;
  }

}

int player_hardwareSetup( void )
{
  FRESULT res;
  
  SPI_Init();
  SPI_Speed_Slow();
  SPI_Send (0xFF);
  
  //_delay_ms( 200 );
  
  res = pf_mount( &fs );
  
  
  if( res == FR_OK )
  {
  
    res = pf_opendir( &g_dir, DEFAULT_DIR );
  }
  else
  {
    printcode(res);
  }
    
  return( res == FR_OK );
}



void player_run()
{
  FILINFO file_info;
  if (!player_hardwareSetup( ))
	return;

  if( !find_first_file( NULL, &file_info ) )
  {
    responseCallback( ":rpno file" );
    return;
  }
  
  while( 1 )
  {
	responseCallback(file_info.fname);
	if (!find_next_file( &file_info ))
	{
	   find_first_file( NULL, &file_info );
	}
	_delay_ms(1000);
  }
}
