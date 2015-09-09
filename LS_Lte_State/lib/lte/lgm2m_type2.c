#include "pal.h"

#ifdef HAVE_LTE_INTERFACE
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib.h"
#include "cli.h"
#include "lib/buffer.h"
#include "if_lte.h"
#include "lib/lte/lgm2m_type2.h"
#include "common/common_drv.h"
#include "lib/lte/serial.h" 


//----------------------------------------------------------------------------
// Definition
//----------------------------------------------------------------------------
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(A) (sizeof (A) / sizeof ((A)[0]))
#endif /* !ARRAY_SIZE */

//#define DBG    console_print
#define DBG(x...)


//----------------------------------------------------------------------------
// Function
//----------------------------------------------------------------------------
/* Comment ********************************************************************
 * LG RNDIS 모듈에 연결을 확인합니다.
 *
 */
int
is_ready_lgm2m_type2 (void)
{
  int i = 0;
  int result = 0;
  FILE *fp = NULL;
  char buf[LTE_BUF_LENGTH + 1] = {0};
  char ping_cmd[LTE_BUF_LENGTH + 1] = {0};

  memset (ping_cmd, 0x00, LTE_BUF_LENGTH);
  sprintf (ping_cmd, "ping -c 1 -W 1 10.0.0.1 2> /dev/null | grep -c 'seq'");

  // 총 5번의 시도록 하도록 합니다.
  for (i = 0; i < 5; i++)
    {
      fp = popen (ping_cmd, "r");

      if (!fp)
        return LTE_ERROR;

      memset (buf, 0x00, sizeof(buf));
      fread ((void*)buf, sizeof(char), LTE_BUF_LENGTH - 1, fp);

      pclose (fp);

      /* Now, get the result */
      result = atoi(buf);

      if(result > 0)
        return LTE_OK;
    }

  return LTE_ERROR;
}


/* Comment ********************************************************************
 * LGM2M TYPE2 장비에 연결할 socket을 open합니다.
 *
 */
static int
open_lgm2m_sock(void)
{
  int client_len;
  int client_sockfd;
  struct timeval tv;

  struct sockaddr_in clientaddr;

  client_sockfd = socket (AF_INET, SOCK_STREAM, 0);
  clientaddr.sin_family = AF_INET;
  clientaddr.sin_addr.s_addr = inet_addr (LGM2M_TYPE2_SOCK_IP);
  clientaddr.sin_port = htons (LGM2M_TYPE2_SOCK_PORT);

  tv.tv_sec = 0;
  tv.tv_usec = 100000;

  setsockopt (client_sockfd,
              SOL_SOCKET,
              SO_RCVTIMEO,
              (char *)&tv,
              sizeof(struct timeval));

  client_len = sizeof(clientaddr);

  if (connect (client_sockfd, (struct sockaddr *)&clientaddr, client_len) < 0)
    {
      close (client_sockfd);
      DBG("connection fail \r\n");
      return -1;
    }

  return client_sockfd;
}

/* Comment ********************************************************************
 *
 * LGM2M TYPE2 장비의 연결된 socket을 close 합니다.
 *
 */
static void
close_lgm2m_sock (int sockfd)
{
  close(sockfd);
}

/* Comment ********************************************************************
 * LGM2M TYPE2 장비의 data를 읽어옵니다.
 *
 */
static int
send_lgm2m_sock(int sockfd, char *pMsgCmd, char *pRetStr)
{
  int i = 0;
  int timeout = 0;
  int cmd_length = 0;
  int tot_length = 0;
  int read_bytes = 0;
  int write_bytes = 0;
  char send_buf[LTE_BUF_LENGTH + 1];
  char recv_buf[LTE_BUF_LENGTH + 1];

  cmd_length = strlen(pMsgCmd);
  memset (send_buf, 0x00, sizeof(send_buf));
  send_buf[0] = 0x07;
  send_buf[1] = 0x33;
  send_buf[2] = 0xff & (cmd_length + 2);
  send_buf[3] = 0x00;
  memcpy (&send_buf[4], pMsgCmd, cmd_length);
  send_buf[LGM2M_TYPE2_HEADER_SIZE + cmd_length] = 0x0d;
  send_buf[LGM2M_TYPE2_HEADER_SIZE + cmd_length + 1] = 0x0a;

  write_bytes = write (sockfd,
                        send_buf,
                        cmd_length + LGM2M_TYPE2_HEADER_SIZE + 2);
  
  DBG ("send_buf = %d \r\n", write_bytes);
  for (i = 0; i < (cmd_length + LGM2M_TYPE2_HEADER_SIZE + 2); i++)
    DBG ("\r\n %d - 0x%x ", i, send_buf[i]);
  DBG ("\r\n");

  while (timeout < 100)
    {
      timeout++;
      memset(recv_buf, 0x00, LTE_BUF_LENGTH);
      read_bytes = read(sockfd, recv_buf, LTE_BUF_LENGTH);
      if (read_bytes < 0)
        {
          DBG(" read_bytes < 0 \n\r" );
          continue;
        }
      else if (read_bytes == 0)
        {
          DBG(" read_bytes == 0 \n\r" );
          continue;
        }
      else
        {
          DBG ("\r\n##recv (%d)  = %d \r\n", timeout, read_bytes);
          for (i = 0; i < read_bytes; i++)
            DBG ("%d - 0x%x \r\n", i, recv_buf[i]);
          DBG("\r\n");

          //Date is overflow !
          if( (tot_length + read_bytes) > LTE_BUF_LENGTH )
            {
              DBG(" Read too much!\n\r" );
              break;
            }

          memcpy(pRetStr, recv_buf, read_bytes);
          pRetStr += read_bytes;
          tot_length += read_bytes;

          if( tot_length > 5 )
          {
            DBG("{%x[5] %x[4] %x[3] %x[2] %x[1]} \r\n",
                            *(pRetStr-5),
                            *(pRetStr-4),
                            *(pRetStr-3),
                            *(pRetStr-2),
                            *(pRetStr-1) );
            if(*(pRetStr - 4) == 'O' &&
                *(pRetStr - 3) == 'K' &&
                *(pRetStr - 2) == 0x0d &&
                *(pRetStr - 1) == 0x0a )
            {
              DBG("find LTE_OK (sock), %d\n\r", tot_length);
              break;
            }
          }
        }
      usleep(20000); // 20 msec
    }

  return tot_length;
}

/* Comment ********************************************************************
 * get LG M2M Typ2 module information
 * LG M2M type2 모듈은 소켓 통신을 통해서 상태값을 읽어옵니다.
 *
 */
void
get_lgm2m_type2_status(ZEBOS_LTE_MODULE_t *mod)
{
  int sockfd = -1;
  int read_bytes = 0;
  char buf[LTE_BUF_LENGTH+1] = {0};
  char real_msg[LTE_BUF_LENGTH+1] = {0};
  char buf_temp[LTE_BUF_LENGTH+1] = {0};
  static int imei_check = 0;
  static int number_check = 0;
  static char imei_backup[MAX_BUF_LENGTH] = {0};
  static char number_backup[MAX_BUF_LENGTH] = {0};

  if (is_ready_lgm2m_type2() == LTE_ERROR)
    {
      imei_check = 0;
      number_check = 0;
      return;
    }

  /* netork name */
  snprintf(mod->name, sizeof(mod->name), "%s", "LGU+");

  /* LGU+ use only LTE  */
  snprintf(mod->type, sizeof(mod->type), "%s", "LTE");

  /* open sock */
  sockfd = open_lgm2m_sock();
  if (sockfd < 0)
    return;

  //////////////////////////////////////////////////////////////
  // DUMMY READ
  //////////////////////////////////////////////////////////////
  memset(buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = send_lgm2m_sock (sockfd, "AT", buf);

  //////////////////////////////////////////////////////////////
  // GET INFO
  //////////////////////////////////////////////////////////////
  memset(buf, 0x00, LTE_BUF_LENGTH);
  memset(real_msg, 0x00, LTE_BUF_LENGTH);
  read_bytes = send_lgm2m_sock (sockfd, "AT$LGTSTINFO?", buf);
  if (read_bytes > 0)
    {
      // LGM2M type2 프로토콜에 의해 앞의 4byte는 header가 옵니다.
      // 그리고 0xd, 0xa 2byte가 먼저 받게 됩니다.
      // 그러므로 앞의 6byte는 무시합니다.
      memcpy(real_msg, buf + 6, LTE_BUF_LENGTH - 6);
      lte_strtok(real_msg, (read_bytes - 6), LGM2M_TYPE2_STATE_INDEX, buf_temp);

      switch (*(buf_temp))
        {
          case 'I':
            snprintf(mod->state_name,
                      sizeof(mod->state_name),
                      "%s",
                      "IN SERVICE");
            break;

          case 'A':
            snprintf(mod->state_name,
                      sizeof(mod->state_name),
                      "%s",
                      "Access State");
            break;

          case 'P':
            snprintf(mod->state_name,
                      sizeof(mod->state_name),
                      "%s",
                      "Paging State");
            break;

          case 'T':
            snprintf(mod->state_name,
                      sizeof(mod->state_name),
                      "%s",
                      "IN SERVICE");
            break;

          case 'N':
            snprintf(mod->state_name,
                      sizeof(mod->state_name),
                      "%s",
                      "No Service");
            break;

          default:
             snprintf(mod->state_name,
                      sizeof(mod->state_name),
                      "%s",
                      "Unknown");
            break;
        }
    }


  /* get network type. (internet / mobile) */
  memset(buf, 0x00, LTE_BUF_LENGTH);
  memset(real_msg, 0x00, LTE_BUF_LENGTH);
  read_bytes = send_lgm2m_sock (sockfd, "AT+CGDCONT?", buf);
  if (read_bytes > 0)
    {
      // LGM2M type2 프로토콜에 의해 앞의 4byte는 header가 옵니다.
      // 그리고 0xd, 0xa 2byte가 먼저 받게 됩니다.
      // 그러므로 앞의 6byte는 무시합니다.
      memcpy(real_msg, buf + 6, LTE_BUF_LENGTH - 6);

      if (lte_lookup_string_from_buf (real_msg, read_bytes, "mobiletelemetry") == LTE_OK)
        snprintf(mod->apn_name, sizeof(mod->apn_name), "%s", "Mobile");
      else if (lte_lookup_string_from_buf (real_msg, read_bytes, "internet") == LTE_OK)
        snprintf(mod->apn_name, sizeof(mod->apn_name), "%s", "Internet");
      else
        snprintf(mod->apn_name, sizeof(mod->apn_name), "%s", "M2M-Router");
    }

  //////////////////////////////////////////////////////////////
  // GET IME
  //////////////////////////////////////////////////////////////
  if (imei_check == 0)
  {
    memset(buf, 0x00, LTE_BUF_LENGTH);
    memset(real_msg, 0x00, LTE_BUF_LENGTH);
    read_bytes = send_lgm2m_sock (sockfd, "AT+CGSN", buf);
    if (read_bytes > 0)
      {
        // LGM2M type2 프로토콜에 의해 앞의 4byte는 header가 옵니다.
        // 그리고 0xd, 0xa 2byte가 먼저 받게 됩니다.
        // 그러므로 앞의 6byte는 무시합니다.
        memcpy(real_msg, buf + 6, LTE_BUF_LENGTH - 6);
        lte_strtok(real_msg, (read_bytes - 6), LGM2M_TYPE2_IMEI_INDEX, mod->imei);

        // imei 값을 백업합니다.
        imei_check = 1;
        memcpy(imei_backup, mod->imei, MAX_BUF_LENGTH);
      }
  }
  else
  {
    // 백업된 값을 불러옵니다.
    memcpy(mod->imei, imei_backup, MAX_BUF_LENGTH);
  }

  //////////////////////////////////////////////////////////////
  // GET MDN
  //////////////////////////////////////////////////////////////
  if (number_check == 0)
  {
    memset(buf, 0x00, LTE_BUF_LENGTH);
    memset(real_msg, 0x00, LTE_BUF_LENGTH);
    read_bytes = send_lgm2m_sock (sockfd, "AT$LGTMDN?", buf);
    if (read_bytes > 0)
      {
        // LGM2M type2 프로토콜에 의해 앞의 4byte는 header가 옵니다.
        // 그리고 0xd, 0xa 2byte가 먼저 받게 됩니다.
        // 그러므로 앞의 6byte는 무시합니다.
        memcpy(real_msg, buf + 6, LTE_BUF_LENGTH - 6);
        lte_strtok(real_msg,
                    (read_bytes - 6),
                    LGM2M_TYPE2_LGTMDN_INDEX,
                    mod->number);

        // number 값을 백업합니다.
        if (strlen(mod->number) < 8)
          {
            number_check = 1;
            memcpy(number_backup, mod->number, MAX_BUF_LENGTH);
          }
      }

    // 전화번호가 8자리 이상 들어오게 된 경우에만 전화번호로 인식합니다.
    if (strlen(mod->number) < 8)
      memset(mod->number, 0x00, sizeof(mod->number));
  }
  else
  {
    // 백업된 값을 불러옵니다.
    memcpy(mod->number, number_backup, MAX_BUF_LENGTH);
  }


  //////////////////////////////////////////////////////////////
  // GET RSS
  //////////////////////////////////////////////////////////////
  {
    memset(buf, 0x00, LTE_BUF_LENGTH);
    memset(real_msg, 0x00, LTE_BUF_LENGTH);
    memset(buf_temp, 0x00, LTE_BUF_LENGTH);
    read_bytes = send_lgm2m_sock (sockfd, "AT$LGTRSSI?", buf);
    if (read_bytes > 0)
      {
        // LGM2M type2 프로토콜에 의해 앞의 4byte는 header가 옵니다.
        // 그리고 0xd, 0xa 2byte가 먼저 받게 됩니다.
        // 그러므로 앞의 6byte는 무시합니다.
        memcpy(real_msg, buf + 6, LTE_BUF_LENGTH - 6);
        lte_strtok(real_msg,
                    (read_bytes - 6),
                    LGM2M_TYPE2_LGTRSSI_INDEX,
                    buf_temp);
      }

    mod->rssi = pal_atoi(buf_temp);

    if (mod->rssi <= 9)
      mod->level = 0;
    else if (mod->rssi <= 11)
      mod->level = 25;
    else if (mod->rssi <= 13)
      mod->level = 50;
    else if (mod->rssi <= 15)
      mod->level = 75;
    else if (mod->rssi <= 62)
      mod->level = 100;
    else
      mod->level = 0;
  }

  close_lgm2m_sock (sockfd);;

  return;
}

/* Comment ********************************************************************
 * LGM2M TYPE2 모듈의 APN을 설정하는 함수입니다.
 *
 */
int
set_lgm2m_type2_apn (int type, char *apn_url)
{
  char buf[LTE_BUF_LENGTH+1] = {0};
  char apn_cmd[LTE_BUF_LENGTH+1] = {0};
  int read_bytes;
  int sockfd;

  memset (apn_cmd, 0x00, LTE_BUF_LENGTH);

  if (type == APN_TYPE_DEFAULT)
    snprintf (apn_cmd,
              LTE_BUF_LENGTH,
              "%s",
              "AT+CGDCONT=1,\"IPV4V6\",\"m2m-router.lguplus.co.kr\",\"\",0,0,0" );
  else if (type == APN_TYPE_LGU)
    snprintf (apn_cmd,
              LTE_BUF_LENGTH,
              "AT+CGDCONT=1,\"IPV4V6\",\"%s\",\"\",0,0,0",
              apn_url);
  else
    return LTE_ERROR;

  /* open sock */
  sockfd = open_lgm2m_sock();
  if (sockfd < 0)
    return LTE_ERROR;

  /* send at command */
  memset (buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = send_lgm2m_sock (sockfd, apn_cmd, buf);

  close_lgm2m_sock (sockfd);

  return LTE_OK;
}

/* Comment ********************************************************************
 * LGM2M TYPE2 모듈의 APN 정보를 읽어옵니다.
 *
 */
void
read_lgm2m_type2_apn (lte_apn_info_t *apn_info)
{
  int sockfd = -1;
  int read_bytes = 0;
  char buf[LTE_BUF_LENGTH+1] = {0};
  char real_msg[LTE_BUF_LENGTH+1] = {0};

  if (is_ready_lgm2m_type2() == LTE_ERROR)
    {
      snprintf (apn_info->short_apn,
                sizeof(apn_info->short_apn),
                "Unknown");
      snprintf (apn_info->long_apn,
                sizeof(apn_info->long_apn),
                "Unknown");
      return;
    }

  /* open sock */
  sockfd = open_lgm2m_sock();
  if (sockfd < 0)
    return;

  memset(buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = send_lgm2m_sock (sockfd, "AT", buf);

  memset(buf, 0x00, LTE_BUF_LENGTH);
  memset(real_msg, 0x00, LTE_BUF_LENGTH);
  read_bytes = send_lgm2m_sock (sockfd, "at+cgdcont?", buf);

  if (read_bytes > 0)
    {
      // socket으로 받은 데이터의 앞 6byte는 헤더입니다.
      memcpy(real_msg, buf + 6, LTE_BUF_LENGTH - 6);

      // apn이 출력되는 라인을 모두 복사합니다.
      // show lte apn detail CLI를 동작시킬 때 표기됩니다.
      lte_strtok_by_user (real_msg,
                         read_bytes,
                         1,
                         apn_info->long_apn,
                         " :\n\r");

      snprintf (real_msg,
                LTE_BUF_LENGTH,
                "%s",
                apn_info->long_apn);

      // apn 만 복사합니다.
      lte_strtok_by_user (real_msg,
                         read_bytes,
                         3,
                         apn_info->short_apn,
                         "\"\n\r");
      close_lgm2m_sock (sockfd);;
      return;
    }

  close_lgm2m_sock (sockfd);;

  snprintf (apn_info->short_apn, sizeof(apn_info->short_apn), "Unknown");
  snprintf (apn_info->long_apn, sizeof(apn_info->long_apn), "Unknown");
}


/* Comment ********************************************************************
 * LG M2M type2 모듈의 상태를 리턴합니다.
 *
 */
int
read_lgm2m_type2_network_status (void)
{
  int sockfd = -1;
  int status = 0;
  int read_bytes = 0;
  char buf[LTE_BUF_LENGTH+1] = {0};
  char real_msg[LTE_BUF_LENGTH+1] = {0};
  char buf_temp[LTE_BUF_LENGTH+1] = {0};

  if (is_ready_lgm2m_type2() == LTE_ERROR)
    return LTE_ERROR;

  sockfd = open_lgm2m_sock();
  if (sockfd < 0)
    return LTE_ERROR;

  memset(buf, 0x00, LTE_BUF_LENGTH);
  read_bytes = send_lgm2m_sock (sockfd, "AT", buf);

  memset(buf, 0x00, LTE_BUF_LENGTH);
  memset(real_msg, 0x00, LTE_BUF_LENGTH);
  read_bytes = send_lgm2m_sock (sockfd, "AT$LGTSTINFO?", buf);
  if (read_bytes > 0)
    {
      // LGM2M type2 프로토콜에 의해 앞의 4byte는 header가 옵니다.
      // 그리고 0xd, 0xa 2byte가 먼저 받게 됩니다.
      // 그러므로 앞의 6byte는 무시합니다.
      memcpy(real_msg, buf + 6, LTE_BUF_LENGTH - 6);
      lte_strtok(real_msg, (read_bytes - 6), LGM2M_TYPE2_STATE_INDEX, buf_temp);


      if (*(buf_temp) == 'I' || *(buf_temp) == 'T')
        status = LTE_OK;
      else
        status = LTE_ERROR;
    }
  else
    status = LTE_ERROR;

  close_lgm2m_sock (sockfd);;

  return status;
}









#endif /* HAVE_LTE_INTERFACE */

/*
 * ex: ts=8 sw=2 sts=2 et
 */
