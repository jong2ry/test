#include "pal.h"

#include <termios.h>

#include "if_lte.h"

#ifdef HAVE_LTE_INTERFACE

//----------------------------------------------------------------------------
// Definition
//----------------------------------------------------------------------------
//#define ltedebug    console_print
#define ltedebug(x...)

//----------------------------------------------------------------------------
// Function
//----------------------------------------------------------------------------
/* Comment ********************************************************************
 * lte_strtok() fucntion.
 * 문자열을 자르는 함수입니다. 
 * 공통적으로 사용되는 함수로 만들었습니다.
 *
 */
int
lte_strtok(char *pbuf, int buf_size, int index, char *pret)
{
  int count = 0;
  char *ptr;

  ptr = strtok (pbuf, " ,.:/[\"\n\r");
  ltedebug("[%d] : %s \r\n", count, ptr);
  if (count == index && ptr != NULL)
    {
      snprintf(pret, MAX_TOKEN_BUF_LENGTH, "%s", ptr);
      return 0;
    }

  while (ptr != NULL)
    {
      count++;
      ptr = strtok (NULL, " ,.:/[\"\n\r");
      ltedebug("[%d] : %s \r\n", count, ptr);
      if (count == index && ptr != NULL)
        snprintf(pret, MAX_TOKEN_BUF_LENGTH, "%s", ptr);
    }

  return 0;
}


/* Comment ********************************************************************
 * lte_strtok_by_user() fucntion.
 * 문자열을 자르는 함수입니다. 
 * 상황에 따라 사용자가 구별하는 구분자를 설정하여 사용하기 위해 만들었습니다.
 *
 */
int
lte_strtok_by_user(char *pbuf, int buf_size, int index, char *pret, char *ptok)
{
  int count = 0;
  char *ptr;

  ptr = strtok (pbuf, ptok);
  ltedebug("[%d] : %s \r\n", count, ptr);
  if (count == index && ptr != NULL)
    {
      snprintf(pret, MAX_TOKEN_BUF_LENGTH, "%s", ptr);
      return 0;
    }

  while (ptr != NULL)
    {
      count++;
      ptr = strtok (NULL, ptok);
      ltedebug("[%d] : %s \r\n", count, ptr);
      if (count == index && ptr != NULL)
        snprintf(pret, MAX_TOKEN_BUF_LENGTH, "%s", ptr);
    }

  return 0;
}


/* Comment ********************************************************************
 * src 문자열에서 dst 문자열이 찾아지면 1을 리턴합니다.
 *
 */
int
lte_lookup_string_from_buf(char *pSrc, int length, char *pDst)
{
  int i = 0;
  int dstLength = 0;

  dstLength = strlen(pDst);

  for (i = 0; (i < (length - dstLength)); i++)
  {
    if(!pal_strncmp((pSrc + i), pDst, dstLength))
    {
      return LTE_OK;
    }
  }

  return LTE_ERROR;
}


/* Comment ********************************************************************
 *  tty 인터페이스와 통신을 합니다.
 *
 */
int
lte_xmit_sms (char *pMsgCmd, char *pRetStr, int wait)
{
  int i = 0;
  int handle;
  struct termios newtio;
  char MsgCR = 0x0d;
  char MsgLF = 0x0a;
  char Buff[LTE_BUF_LENGTH];
  int RxCount;
  int timeout=0;
  int cmdLen=0;
  int retValLen=0;
  int product = PRODUCT_NONE;

  // command를 실행할 때마다 Delay 값을 수행합니다.
  cmdLen = strlen (pMsgCmd);
  product = lte_get_product ();

  if (product == PRODUCT_NONE)
    return -1;

  if (cmdLen > 256 || cmdLen <= 0)
    return -1;

  if (product == PRODUCT_AMT)
    handle = open( TTY_AMT, O_RDWR | O_NOCTTY | O_NONBLOCK);
  else if (product == PRODUCT_KNI)
    {
      if (is_nexg_model(NEXG_MODEL_VF403))
        handle = open( TTY_KNI1, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
      else
        handle = open( TTY_KNI0, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
    }
  else if (product == PRODUCT_MEC)
    handle = open( TTY_MEC, O_RDWR | O_NOCTTY | O_NONBLOCK);
  else if (product == PRODUCT_ANYDATA)
    handle = open( TTY_ANY, O_RDWR | O_NOCTTY | O_NONBLOCK);
  else if (product == PRODUCT_NTLM200Q)
    handle = open( TTY_NTLM200Q, O_RDWR | O_NOCTTY | O_NONBLOCK);
  else if (product == PRODUCT_L300S)
    handle = open( TTY_L300S, O_RDWR | O_NOCTTY | O_NONBLOCK);
  else if (product == PRODUCT_TM400)
    handle = open( TTY_TM400, O_RDWR | O_NOCTTY | O_NONBLOCK);
  else if (product == PRODUCT_PEGASUS)
    handle = open( TTY_PEGASUS, O_RDWR | O_NOCTTY | O_NONBLOCK);
  else
    return -1;

  if (handle < 0)
    //ltedebug("Serial Open Fail [%s]\n", TTY_AMT);
    return -1;

  /*Serial Config*/
  tcgetattr( handle, &newtio );
  //memset( &newtio, 0, sizeof(newtio) );
  newtio.c_cflag = B115200 | CS8 | CLOCAL | CREAD | CRTSCTS;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;
  //set input mode (non-canonical, no echo,.....)
  newtio.c_lflag = 0;
  newtio.c_cc[VTIME] = 30;
  newtio.c_cc[VMIN] = 0;
  tcflush( handle, TCIFLUSH );
  tcsetattr( handle, TCSANOW, &newtio );

  usleep(1000);

  // write commmand message
  write( handle, pMsgCmd, strlen( pMsgCmd ) );
  write( handle, &MsgCR, sizeof( MsgCR ) );
  write( handle, &MsgLF, sizeof( MsgLF ) );

  // sms를 보내는 경우 대기시간이 필요합니다.
  while( timeout < 10 )
  {
    timeout++;
    usleep(1000);
    memset(Buff, 0x00, sizeof(Buff));
    RxCount = read( handle, Buff, LTE_BUF_LENGTH );

    if( RxCount < 0 )
    {
      ltedebug("%s Read Error\n\r" );
      // NTLM200Q 모듈을 SMS를 전송할 때 1초정도의 delay 가 생긴다.
      if (product == PRODUCT_NTLM200Q || product == PRODUCT_L300S)
        {
          sleep(1);
          continue;
        }
      break;
    }
    else if( RxCount == 0 ) /*No more data */
    {
      if (wait)
      {
        ltedebug("No more data -> continue;\n\r");
        continue;
      }
      else
      {
        ltedebug("No more data -> end!\n\r");
        break;
      }
    }
    else
    {
      ltedebug("\n\rRead data[%s]%d\r\n", Buff, RxCount);

      //Date is overflow !
      if( (retValLen + RxCount) > LTE_BUF_LENGTH )
      {
        ltedebug(" Read too much!\n\r" );
        break;
      }

      strcpy(pRetStr, Buff);
      pRetStr += RxCount;
      retValLen += RxCount;

      if( RxCount > 10 )
      {
        for (i = 1; i < RxCount; i++)
        {
          if(Buff[i-1] =='O' && Buff[i] =='K' )
          {
            ltedebug("find LTE_OK[%d], %d\n\r", retValLen, RxCount);
            goto end_send_msg;
            break;
          }
        }
      }

      if( retValLen > 8 )
      {
        for (i = 1; i < RxCount; i++)
        {
          if(Buff[i-4] =='E' && Buff[i-3] =='R'  && Buff[i-2] =='R'  && Buff[i-1] =='O'  && Buff[i] =='R' )
          {
            ltedebug("find LTE_ERROR[%d], %d\n\r", retValLen, RxCount);
            goto end_send_msg;
            break;
          }
        }
      }
    }
  }

end_send_msg :
  close( handle );

  return retValLen;
}


/* Comment ********************************************************************
 *  tty 인터페이스와 통신을 합니다.
 *
 */
int
lte_xmit_serial (char *pMsgCmd, char *pRetStr)
{
  int handle;
  struct termios newtio;
  struct termios oldtio;
  char MsgCR = 0x0d;
  char MsgLF = 0x0a;
  char Buff[LTE_BUF_LENGTH];
  int RxCount;
  int timeout=0;
  int cmdLen=0;
  int retValLen=0;
  int product = PRODUCT_NONE;

  // command를 실행할 때마다 Delay 값을 수행합니다.
  cmdLen = strlen(pMsgCmd);
  product = lte_get_product();

  if (product == PRODUCT_NONE)
    return -1;

  if( cmdLen > 256 || cmdLen <= 0)
    return -1;

  if (product == PRODUCT_AMT)
    handle = open( TTY_AMT, O_RDWR | O_NOCTTY | O_NONBLOCK);
  else if (product == PRODUCT_KNI)
    {
      if (is_nexg_model(NEXG_MODEL_VF403))
        handle = open( TTY_KNI1, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
      else
        handle = open( TTY_KNI0, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
    }
  else if (product == PRODUCT_MEC)
    handle = open( TTY_MEC, O_RDWR | O_NOCTTY | O_NONBLOCK);
  else if (product == PRODUCT_ANYDATA)
    handle = open( TTY_ANY, O_RDWR | O_NOCTTY );
  else if (product == PRODUCT_LGM2M_TYPE1)
    handle = open( TTY_LGM2M_TYPE1, O_RDWR | O_NOCTTY | O_NONBLOCK);
  else if (product == PRODUCT_NT)
    handle = open( TTY_NT, O_RDWR | O_NOCTTY | O_NONBLOCK);
  else if (product == PRODUCT_NTLM200Q)
    handle = open( TTY_NTLM200Q, O_RDWR | O_NOCTTY | O_NONBLOCK);
  else if (product == PRODUCT_L300S)
    handle = open( TTY_L300S, O_RDWR | O_NOCTTY | O_NONBLOCK);
  else if (product == PRODUCT_TM400)
    handle = open( TTY_TM400, O_RDWR | O_NOCTTY | O_NONBLOCK);
  else if (product == PRODUCT_PEGASUS)
    handle = open( TTY_PEGASUS, O_RDWR | O_NOCTTY | O_NONBLOCK);
  else
    return -1;

  if( handle < 0 ) 
    return -1;

  if (product == PRODUCT_LGM2M_TYPE1)
    {
      /*Serial Config*/
      tcgetattr( handle, &newtio );
      //memset( &newtio, 0, sizeof(newtio) );
      newtio.c_cflag = B460800 | CS8 | CLOCAL | CREAD ;
      newtio.c_iflag = IGNPAR;
      newtio.c_oflag = 0;
      //set input mode (non-canonical, no echo,.....)
      newtio.c_lflag = 0;
      newtio.c_cc[VTIME] = 30;
      newtio.c_cc[VMIN] = 0;
      tcflush( handle, TCIFLUSH );
      tcsetattr( handle, TCSANOW, &newtio );
    }
  else
    {
      /*Serial Config*/
      tcgetattr( handle, &newtio );
      //memset( &newtio, 0, sizeof(newtio) );
      newtio.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
      newtio.c_iflag = IGNPAR;
      newtio.c_oflag = 0;
      //set input mode (non-canonical, no echo,.....)
      newtio.c_lflag = 0;
      newtio.c_cc[VTIME] = 30;
      newtio.c_cc[VMIN] = 0;
      tcflush( handle, TCIFLUSH );
      tcsetattr( handle, TCSANOW, &newtio );
    }

  usleep(1000);

  // write commmand message
  write( handle, pMsgCmd, strlen( pMsgCmd ) );
  write( handle, &MsgCR, sizeof( MsgCR ) );
  write( handle, &MsgLF, sizeof( MsgLF ) );

  while( timeout < 100 )
  {
    timeout++;
    usleep(10000);
    memset(Buff, 0x00, sizeof(Buff));
    RxCount = read( handle, Buff, LTE_BUF_LENGTH );

    if( RxCount < 0 )
    {
      ltedebug("%s Read Error\n\r" );
      continue;
    }
    else if( RxCount == 0 ) /*No more data */
    {
      ltedebug("No more data -> end!\n\r");
      break;
    }
    else
    {
      ltedebug("\n\rRead data[%s]%d\n\r", Buff, RxCount);

      //Date is overflow !
      if( (retValLen + RxCount) > LTE_BUF_LENGTH )
      {
        ltedebug(" Read too much!\n\r" );
        break;
      }

      strcpy(pRetStr, Buff);
      pRetStr += RxCount;
      retValLen += RxCount;

      if( retValLen > 5 )
      {
        ltedebug("{%x[5] %x[4] %x[3] %x[2] %x[1]}\n", *(pRetStr-5), *(pRetStr-4), *(pRetStr-3), *(pRetStr-2), *(pRetStr-1) );
        if(*(pRetStr-5)==MsgLF && *(pRetStr-4)=='O' && *(pRetStr-3)=='K' )
        {
          ltedebug("find LTE_OK[%d], %d\n\r", retValLen, RxCount);
          break;
        }
      }

      if( retValLen > 8 )
      {
        ltedebug("{%x[7] %x[6] %x[5] %x[4] %x[3]}\n", *(pRetStr-7), *(pRetStr-6), *(pRetStr-5), *(pRetStr-4), *(pRetStr-3) );
        if( *(pRetStr-7)=='E' && *(pRetStr-6)=='R' &&  *(pRetStr-5)=='R' && *(pRetStr-4)=='O' && *(pRetStr-3)=='R' )
        {
          ltedebug("find LTE_ERROR[%d], %d\n\r", retValLen, RxCount);
          break;
        }
      }
    }
  }

  tcdrain(handle);
  tcsetattr(handle, TCSANOW, &oldtio);

  close( handle );

  return retValLen;
}


#endif /* HAVE_LTE_INTERFACE */

/*
 * ex: ts=8 sw=2 sts=2 et
 */

