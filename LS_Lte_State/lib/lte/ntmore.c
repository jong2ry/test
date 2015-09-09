#include "pal.h"

#ifdef HAVE_LTE_INTERFACE
#include <unistd.h>
#include <fcntl.h>
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
#include "common/common_drv.h"
#include "if_lte.h"
#include "lib/lte/ntmore.h"
#include "lib/lte/serial.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(A) (sizeof (A) / sizeof ((A)[0]))
#endif /* !ARRAY_SIZE */


/* Comment *******************************************************
 * NTmore 모듈의 정보를 읽어옵니다.
 *
 */
void
get_ntmore_status(ZEBOS_LTE_MODULE_t *mod)
{
  enum {
    NT_DUMMY_CMD= 0,
    NT_CREG_CMD,
    NT_GSN_CMD,
    NT_GMM_CMD
  };
  char *pCmd[] = {"at",
                  "at+creg?",
                  "at+gsn",
                  "at+gmm",
                  NULL};
  char buf[LTE_BUF_LENGTH+1] = {0};
  char buf_temp[LTE_BUF_LENGTH+10] = {0};
  char *pbuf = buf;
  char *pbuf_temp = buf_temp;
  int value = 0;
  //int rssi = 0;
  int read_bytes = 0;

  // dummy read
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[NT_DUMMY_CMD], pbuf);

  // dummy read
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[NT_DUMMY_CMD], pbuf);

  //state
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[NT_CREG_CMD], pbuf);
  ltedebug("%s \r\n", pbuf);
  if (read_bytes > 0)
    {
      memset(pbuf_temp, 0x00, LTE_BUF_LENGTH);
      lte_strtok(pbuf, read_bytes, 3, pbuf_temp);

      value = atoi(pbuf_temp);

      if (value == 0)
        snprintf(mod->state_name, MAX_BUF_LENGTH, "%s", "DISCONNECTED");
      else if (value == 1)
        snprintf(mod->state_name, MAX_BUF_LENGTH, "%s", "CONNECTED");
      else if (value == 2)
        snprintf(mod->state_name, MAX_BUF_LENGTH, "%s", "SEARCHING");
      else if (value == 3)
        snprintf(mod->state_name, MAX_BUF_LENGTH, "%s", "LIMITED");
      else if (value == 4)
        snprintf(mod->state_name, MAX_BUF_LENGTH, "%s", "CONNECTED(ROAM)");
      else
        snprintf(mod->state_name, MAX_BUF_LENGTH, "%s", "DISCONNECTED");
    }

  // IMEI (GSN)
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  memset(pbuf_temp, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[NT_GSN_CMD], pbuf);
  ltedebug("%s \r\n", pbuf);
  if (read_bytes > 0)
    lte_strtok(pbuf, read_bytes, 1, mod->imei);

  // module info (GMM)
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  memset(pbuf_temp, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[NT_GMM_CMD], pbuf);
  ltedebug("%s \r\n", pbuf);
  if (read_bytes > 0)
    lte_strtok(pbuf, read_bytes, 1, mod->info);



#if 0
  // network name / network type
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[AMT_NSI_CMD], pbuf);
  ltedebug("%s \r\n", pbuf);
  if (read_bytes > 0)
    {
      // network name
      {
        if (lte_lookup_string_from_buf(pbuf, read_bytes, "skt") == LTE_OK ||
            lte_lookup_string_from_buf(pbuf, read_bytes, "SKT") == LTE_OK )
          snprintf(mod->name, sizeof(mod->name), "%s", "SKT");
        else if (lte_lookup_string_from_buf(pbuf, read_bytes, "olleh") == LTE_OK||
            lte_lookup_string_from_buf(pbuf, read_bytes, "kt") == LTE_OK ||
            lte_lookup_string_from_buf(pbuf, read_bytes, "OLLEH") == LTE_OK ||
            lte_lookup_string_from_buf(pbuf, read_bytes, "KT") == LTE_OK)
          snprintf(mod->name, sizeof(mod->name), "%s", "KT");
        else
          snprintf(mod->name, sizeof(mod->name), "%s", "UNKNOWN");
      }

      // network type
      lte_strtok(pbuf, read_bytes, AMT_NETTYPE_INDEX, mod->type);
  }

  // RSSI (signal quality)
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  memset(pbuf_temp, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[AMT_CSQ_CMD], pbuf);
  ltedebug("%s \r\n", pbuf);
  if (read_bytes > 0)
    {
      lte_strtok(pbuf, read_bytes, AMT_RSSI_INDEX, pbuf_temp);

      rssi = pal_atoi(pbuf_temp);
      //ltedebug("@buff = %s (%d)\r\n", buff, rssi );
      if (rssi == 99)
        rssi = 0;

      mod->level = (rssi / 6) * 25; // 100% 단위로 표시하기 위하여 25를 곱합니다.
      if (mod->level > 100)
        mod->level = 100;
      else if (mod->level < 0)
        mod->level = 0;

      // 실제 dBm 값 구하는 공식((113 - (mod->rssi * 2)) * -1) )
      mod->rssi = ((113 - (rssi * 2)) * -1);
    }
  // NUMBER (CNUM)
  memset(pbuf, 0x00, LTE_BUF_LENGTH);
  memset(pbuf_temp, 0x00, LTE_BUF_LENGTH);
  read_bytes = lte_xmit_serial(pCmd[AMT_CNUM_CMD], pbuf);
  ltedebug("%s \r\n", pbuf);
  if (read_bytes > 0)
    {
      // KT의 경우 전화번호 Index가 차이가 있습니다.
      if (strncmp(mod->name, "SKT", 3) ==0)
        lte_strtok(pbuf, read_bytes, AMT_CNUM_INDEX, mod->number);
      else if (strncmp(mod->name, "KT", 2) ==0)
        lte_strtok(pbuf, read_bytes, AMT_CNUM_INDEX + 2, mod->number);
    }
#endif
}




#endif /* HAVE_LTE_INTERFACE */

/*
 * ex: ts=8 sw=2 sts=2 et
 */
