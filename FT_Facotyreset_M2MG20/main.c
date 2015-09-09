#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>

#define BUF_SIZE                        16
#define STRING_LENGTH                   128
#define STAMP_LENGTH                    32

#define OK                              0
#define ERROR                           -1


#define ASCII_ESC 27

#define GOTO_TOP  "\033[1;1H"   /* ESC[1;1H begins output at the top of the terminal (line 1) */
#define GOTO_BOTTOM "\033[100;1H"   /* ESC[1;1H begins output at the bottom of the terminal (actually line 100) */
#define GOTO_BOL  "\033[100D"   /* Go to beginning of line */
#define ERASE_EOL "\033[0K"     /* Erase to end of line */
#define ERASE_EOS "\033[0J"     /* Erase to end screen */
#define ERASE_WIN "\033[2J"     /* Erase the window */
#define REVERSE   "\033[7m"     /* Reverse the display */
#define NORMAL    "\033[0m"     /* Normal display */
#define NONZEROHI "\033[33m"    /* Yellow used for non zero rows */
#define ZEROHI    "\033[32m"    /* Green used for zero rows */
#define SCROLL_FULL "\033[1;r"  /* Normal full-screen scrolling for statistics region */
#define SET_SCROLL_REGION(row) printf("\033[%d;r",row)   /* for command region */
#define CURSOR_ON  "\033[?25h"  /* Turn on cursor */
#define CURSOR_OFF "\033[?25l"  /* Turn off cursor */
#define GOTO_BOTTOM_x "\033[100;%dH" /* go to the bottom of the screen and posion x (supply to printf) */

#define M2MG20_LAN1           "LAN1"
#define M2MG20_LAN1_DST_IP    "1.1.1.254"
#define M2MG20_LAN2           "LAN2"
#define M2MG20_LAN2_DST_IP    "2.1.1.254"
#define M2MG20_LAN3           "LAN3"
#define M2MG20_LAN3_DST_IP    "3.1.1.254"
#define M2MG20_LAN4           "LAN4"
#define M2MG20_LAN4_DST_IP    "4.1.1.254"
#define M2MG20_WAN1           "WAN1"
#define M2MG20_WAN1_DST_IP    "5.1.1.254"


static void
display_err_msg(void)
{
  printf ("\r\n");
  printf ("=====================================================\r\n");
  printf ("\r\n");
  printf (" #########                       \r\n");
  printf (" #########                       \r\n");
  printf (" ###                                   \r\n");
  printf (" #########  #######  #######   #####   #######   \r\n");
  printf (" #########  ##   ##  ##   ##  ##   ##  ##   ##   \r\n");
  printf (" ###        #######  #######  ##   ##  #######   \r\n");
  printf (" #########  ## ##    ## ##    ##   ##  ## ##     \r\n");
  printf (" #########  ##   ##  ##   ##   #####   ##   ##   \r\n");
  printf ("=====================================================\r\n");
  printf ("\r\n");
}

static void
display_pass_msg(void)
{
  printf ("\r\n");
  printf ("=====================================================\r\n");
  printf ("\r\n");
  printf (" #########                      \r\n");
  printf (" #########                       \r\n");
  printf (" ###   ###                       \r\n");
  printf (" #########  #######  #######  #######  ###  ###   \r\n");
  printf (" #########  ##   ##  ###      ####     ###  ###   \r\n");
  printf (" ###        #######  #######  #######  ###  ###   \r\n");
  printf (" ###        ##   ##      ###      ###             \r\n");
  printf (" ###        ##   ##  #######  #######  ###  ###   \r\n");
  printf ("=====================================================\r\n");
  printf ("\r\n");
}

/* Comment ********************************************************************
 *
 *
 */
static int
m2mg20_rtc_test(void)
{
  FILE *fp;
  char pBuf[256];

  memset(pBuf, 0, sizeof(pBuf));

  // run date command.
  fp = popen("date 010101012014 > /dev/null;hwclock -w;hwclock -r", "r");

  if(!fp)
    return ERROR;

  fgets(pBuf, 256, fp);

  if (pclose(fp) == -1)
    return ERROR;


  if (!strncmp("Wed Jan  1 01:01:00 2014  0.000000 seconds", pBuf,
    strlen("Wed Jan  1 01:01:00 2014  0.000000 seconds")))
  {
    return OK;
  }
  else
  {
    return ERROR;
  }
}




/* Comment ********************************************************************
 *
 *
 */
static int
m2mg20_lte_test(void)
{
  char buf[256];
  char cmd[256];
  int count = 0;
  FILE *fp;

  memset(buf, 0x00, sizeof(buf));
  memset(cmd, 0x00, sizeof(cmd));

  sprintf(cmd, "ifconfig  | grep -c 'lte0'");
  fp=popen(cmd, "r");

  if (!fp)
    return ERROR;

  fgets(buf, 128, fp);

  /* Now, get the result */
  count = atoi(buf);

  /* Close the pipe */
  if (pclose(fp) == -1)
    return ERROR;

  if (count > 0)
    return OK;

  return ERROR;
}



/* Comment ********************************************************************
 * Ping Test
 *
 */
int ping_test (char *if_name, char *ip)
{
  char buf[256];
  char cmd[256];
  int count = 0;
  FILE *fp;
  int i = 0;

  for(i = 0;i < 30; i++)
  {
    memset(buf, 0x00, sizeof(buf));
    memset(cmd, 0x00, sizeof(cmd));

    sprintf(cmd, "ping -c 1 -W 1 %s 2> /dev/null | grep -c 'seq'", ip);
    fp=popen(cmd, "r");

    if (!fp)
      return ERROR;

    fgets(buf, 128, fp);

    /* Now, get the result */
    count = atoi(buf);

    /* Close the pipe */
    if (pclose(fp) == -1)
      return ERROR;

    if (count > 0)
      return OK;
    else
      fprintf(stderr, "\r       \r%s .... Retry %d Ping.", if_name, i);

    sleep (1);
  }

  fprintf(stderr, "\r                          \r");
  return ERROR;
}


static int
m2mg20_spd100_test (void)
{
  char dummy_ch[32];

  system("echo spd100 > /proc/nexg/rtl8367/rtl8367");
  system("ethtool -s eth1 speed 100 autoneg off");

  for(;;)
    {
      printf("Is check all Spd100 Green LED ?? (y/n) : ");
      fgets(dummy_ch, 32, stdin);
      if (dummy_ch[0] == 'y')
        return OK;
      else if (dummy_ch[0] == 'n')
        return ERROR;
    }

  return ERROR;
}


static int
m2mg20_led_on_test (void)
{
  char dummy_ch[32];

  system ("echo pup > /proc/nexg/led/lte");
  system ("echo pup > /proc/nexg/led/sig");

  for(;;)
    {
      printf("Is check all LED's ON ?? (y/n) : ");
      fgets(dummy_ch, 32, stdin);
      if (dummy_ch[0] == 'y')
        return OK;
      else if (dummy_ch[0] == 'n')
        return ERROR;
    }

  return ERROR;
}


static int
m2mg20_led_off_test (void)
{
  char dummy_ch[32];

  system ("echo pdn > /proc/nexg/led/lte");
  system ("echo pdn > /proc/nexg/led/sig");

  for(;;)
    {
      printf("Is check all LED's OFF ?? (y/n) : ");
      fgets(dummy_ch, 32, stdin);
      if (dummy_ch[0] == 'y')
        return OK;
      else if (dummy_ch[0] == 'n')
        return ERROR;
    }

  return ERROR;

}



/* Comment ********************************************************************
 * main function.
 *
 */
int main(int argc, char *argv[])
{
  int err_count = 0;

  printf ("\r\n\r\n\r\n");
  printf ("\r\nNEXG FACTORY-TEST\r\n");
  printf ("=====================================================\r\n");

  if (m2mg20_rtc_test () == OK)
    printf ("RTC    : OK\r\n");
  else
    {
      err_count++;
      printf ("RTC    : ERROR\r\n");
    }

  if (m2mg20_lte_test () == OK)
    printf ("LTE    : OK\r\n");
  else
    {
      err_count++;
      printf ("LTE    : ERROR\r\n");
    }

  if (ping_test (M2MG20_LAN1, M2MG20_LAN1_DST_IP) == OK)
    printf ("%s   : OK\r\n", M2MG20_LAN1);
  else
    {
      err_count++;
      printf ("%s   : ERROR\r\n", M2MG20_LAN1);
    }

  if (ping_test (M2MG20_LAN2, M2MG20_LAN2_DST_IP) == OK)
    printf ("%s   : OK\r\n", M2MG20_LAN2);
  else
    {
      err_count++;
      printf ("%s   : ERROR\r\n", M2MG20_LAN2);
    }

  if (ping_test (M2MG20_LAN3, M2MG20_LAN3_DST_IP) == OK)
    printf ("%s   : OK\r\n", M2MG20_LAN3);
  else
    {
      err_count++;
      printf ("%s   : ERROR\r\n", M2MG20_LAN3);
    }

  if (ping_test (M2MG20_LAN4, M2MG20_LAN4_DST_IP) == OK)
    printf ("%s   : OK\r\n", M2MG20_LAN4);
  else
    {
      err_count++;
      printf ("%s   : ERROR\r\n", M2MG20_LAN4);
    }

  if (ping_test (M2MG20_WAN1, M2MG20_WAN1_DST_IP) == OK)
    printf ("%s   : OK\r\n", M2MG20_WAN1);
  else
    {
      err_count++;
      printf ("%s   : ERROR\r\n", M2MG20_WAN1);
    }

  if (m2mg20_led_on_test () == OK)
     printf ("LED_ON  : OK\r\n");
  else
    {
      err_count++;
      printf ("LED_ON  : ERROR\r\n");
    }

  if (m2mg20_led_off_test () == OK)
     printf ("LED_OFF : OK\r\n");
  else
    {
      err_count++;
      printf ("LED_OFF : ERROR\r\n");
    }

  if (m2mg20_spd100_test () == OK)
     printf ("SPD100  : OK\r\n");
  else
    {
      err_count++;
      printf ("SPD100  : ERROR\r\n");
    }

  if (err_count == 0)
    display_pass_msg ();
  else
    display_err_msg ();


  return 0;
}


