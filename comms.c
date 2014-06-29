#include "integer.h"
#include "comms.h"
#include "serial.h"

#define RECV_WAIT_CMD       0
#define RECV_WAIT_DATA      1

BYTE g_recvState = RECV_WAIT_CMD;
BYTE g_pendingCommand = COMMAND_IDLE;
BYTE g_curCommand = COMMAND_IDLE;
BYTE g_exitFlag = 0;

BYTE player_handleInput(char ch)
{
  if (ch >= 'a' && ch <= 'z')
  {
    ch = (ch - 'a') + 'A';
  }
  
  switch(g_recvState)
  {
    case RECV_WAIT_CMD:
    {
      switch(ch)
      {
        case 'S':
        {
          g_pendingCommand = COMMAND_SELECT;
          g_recvState = RECV_WAIT_DATA;
          break;
        }
        case 'N':
        {
          g_pendingCommand = COMMAND_NEXT;
          g_recvState = RECV_WAIT_DATA;
          break;
        }
        case 'P':
        {
          g_pendingCommand = COMMAND_PREVIOUS;
          g_recvState = RECV_WAIT_DATA;
          break;
        }
        case 'A':
        {
          g_pendingCommand = COMMAND_ABORT;
          g_recvState = RECV_WAIT_DATA;
          break;
        }
      }
      break;
    }
    
    case RECV_WAIT_DATA:
    {
      if (ch != '\n')
      {
        if (ch == '\r')
        {
          // ignore carriage return
          return(0);
        }
        else
        {
          // invalid command
          g_recvState = RECV_WAIT_CMD;
        }
      }
      else
      {
        g_curCommand = g_pendingCommand;
        g_exitFlag = 1;
        g_recvState = RECV_WAIT_CMD;
      }
      break;
    }
  }
  return(0);
}

unsigned char input_callback()
{
  if (serial_available() > 0)
  {
    char ch = serial_read();
    return(player_handleInput(ch));
  }
  return(0);
}

