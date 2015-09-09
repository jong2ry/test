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


/* Comment ********************************************************************
 * Ping Test
 *
 */
int ping_test (char *ip)
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

    sleep (1);
  }

  return ERROR;
}



/* Comment ********************************************************************
 * Ping Test
 *
 */
static int _is_find_usb (char *usb_id)
{
  char buf[256];
  char cmd[256];
  int count = 0;
  FILE *fp;

  memset(buf, 0x00, sizeof(buf));
  memset(cmd, 0x00, sizeof(cmd));

  sprintf(cmd, "lsusb | grep -c '%s'", usb_id);
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
 * main function.
 *
 */
int main(int argc, char *argv[])
{
  system ("echo 'Check USB ID' >> /media/disk0/chkusb.log");
  system ("date >> /media/disk0/chkusb.log");

  //if (ping_test ("8.8.8.8") == OK)
  if (_is_find_usb ("1076:9005") == OK)
    {
      printf ("USB OK \r\n");
      system ("echo 'USB OK' >> /media/disk0/chkusb.log");
      sleep(5);
      printf ("SYSTEM reboot \r\n");
      system ("echo 'SYSTEM reboot' >> /media/disk0/chkusb.log");
      sleep(5);
      system ("reboot -f");
    }
  else
    {
      printf ("USB ERROR \r\n");
      system ("echo 'USB ERROR' >> /media/disk0/chkusb.log");
    }

  return 0;
}


