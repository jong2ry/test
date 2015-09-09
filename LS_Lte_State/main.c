#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>  
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include <sys/stat.h>                                                           
#include <dirent.h>

#include "pal.h"
#include "lib/if_lte.h"

#define BUF_SIZE                16
#define STRING_LENGTH           128
#define STAMP_LENGTH            32

#define OK                      0
#define ERROR                   -1
#define AUX_CONSOLE_INF         "/dev/ttyS1" 
#define SENSOR_REQUEST_MESSAGE  "R"



/* Comment ********************************************************************
 * Zebos 에서 사용하는 is_nexg_model 함수를 대체하기 위하여 사용합니다. 
 * Zebos 에서 VF403 모델을 체크하기 위해서 사용한 것이기 때문에 동일하게 사용합니다.
 *
 */
int is_nexg_model (int model)
{
  if (model == NEXG_MODEL_VF403)
    return 1;
  else 
    return 0;
}


/* Comment ********************************************************************
 * Zebos 에서 사용하는 cli_out 함수를 대체하기 위하여 사용합니다. 
 *
 */
void cli_out(struct cli *cli, const char *fmt, ...) 
{
  FILE *console;
  va_list args;
  char tmpbuf[1024];
  char printbuffer[1024];

  console = fopen("/dev/console", "a");

  va_start (args, fmt);

  /* For this to work, printbuffer must be larger than
 *    * anything we ever want to print.
 *       */
  vsprintf (tmpbuf, fmt, args);
  va_end (args);

  sprintf(printbuffer, "%s", tmpbuf);

  /* Send to desired file */
  fprintf(console, "%s", printbuffer);

  fclose(console);
}



void console_print(const char *fmt, ...)
{
  FILE *console;
  va_list args;
  char tmpbuf[1024];
  char printbuffer[1024];

  console = fopen("/dev/console", "a");

  va_start (args, fmt);

  /* For this to work, printbuffer must be larger than
 *    * anything we ever want to print.
 *       */
  vsprintf (tmpbuf, fmt, args);
  va_end (args);

  sprintf(printbuffer, "%s", tmpbuf);

  /* Send to desired file */
  fprintf(console, "%s", printbuffer);

  fclose(console);
}

//#define PRINT_MSG(msg...)              {console_print(msg);fflush(stdout);}
#define PRINT_MSG(msg...)                {printf(msg);fflush(stdout);}



/* Comment ********************************************************************
 * LTE 상태를 파일로 기록합니다.
 *
 */
void lte_state_print (struct cli *cli, ZEBOS_LTE_MODULE_t *module)
{
  FILE *f;
  time_t cur_time;

  f = fopen("/tmp/lte_state", "w+");

  time(&cur_time);

  // update 된 시간을 표기하기 위하여 추가합니다.
  fprintf(f, "update : %s", ctime(&cur_time));
  fprintf(f, "manufacturer : %s\n", module->manufacturer);
  fprintf(f, "info : %s\n", module->info);
  fprintf(f, "state : %d\n", module->state);
  fprintf(f, "state_name : %s\n", module->state_name);
  fprintf(f, "name : %s\n", module->name);
  fprintf(f, "type : %s\n", module->type);
  fprintf(f, "level : %d\n", module->level);
  fprintf(f, "rssi : %d\n", module->rssi);
  fprintf(f, "rsrp : %d\n", module->rsrp);
  fprintf(f, "rsrq : %d\n", module->rsrq);
  fprintf(f, "sinr : %d\n", module->sinr);
  fprintf(f, "lgu_uicc : %s\n", module->lgu_uicc);
  fprintf(f, "number : %s\n", module->number);
  fprintf(f, "number2 : %s\n", module->number2);
  fprintf(f, "imei : %s\n", module->imei);
  fprintf(f, "if_type : %s\n", module->if_type);
  fprintf(f, "ip_addr : %s\n", module->ip_addr);
  fprintf(f, "apn_name : %s\n", module->apn_name);
  fprintf(f, "ruimid : %s\n", module->ruimid);

  fclose(f);
}

/* Comment ********************************************************************
 * main function.
 *
 */
int main(int argc, char *argv[])
{
  struct cli *cli;
  ZEBOS_LTE_MODULE_t module;

  memset (&module, 0x00, sizeof(ZEBOS_LTE_MODULE_t));
  cli = (struct cli *)malloc(sizeof(struct cli));

  for (;;)
    {
      memset (&module, 0x00, sizeof(ZEBOS_LTE_MODULE_t));
      lte_get_module_info (&module); 
      lte_state_print (cli, &module);
      sleep (3);
    } 

  free (cli);

  return -1;
}


