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

#define BUF_SIZE                16
#define STRING_LENGTH           128
#define STAMP_LENGTH            32

#define OK                      0
#define ERROR                   -1
#define AUX_CONSOLE_INF         "/dev/ttyS1" 
#define SENSOR_REQUEST_MESSAGE  "R"
#define SENSOR_INIT_MESSAGE  "I"


void console_print(const char *fmt, ...)
{
  FILE *console;
  va_list args;
  uint i;
  char tmpbuf[1024];
  char printbuffer[1024];

  console = fopen("/dev/console", "a");

  va_start (args, fmt);

  /* For this to work, printbuffer must be larger than
 *    * anything we ever want to print.
 *       */
  i = vsprintf (tmpbuf, fmt, args);
  va_end (args);

  sprintf(printbuffer, "%s", tmpbuf);

  /* Send to desired file */
  fprintf(console, "%s", printbuffer);

  fclose(console);
}
#define PRINT_MSG(msg...)              {console_print(msg);fflush(stdout);}



/* Comment ********************************************************************
 * GYRO 센서를 체크합니다.
 *
 */
int gyro_test(void)
{
  int fd;
  struct termios newtio;
  char buf_msg[64];

  fd = open(AUX_CONSOLE_INF, O_RDWR | O_NOCTTY );

  if (fd < 0) {
    PRINT_MSG("SENSOR\t>>>>>>\tAUX ERROR\n\r");
    return ERROR;
  }

  tcgetattr(fd,&newtio);
  newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;
  // set input mode (non-canonical, no echo,...)
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME] = 1;
  newtio.c_cc[VMIN] = 0;

  tcflush(fd, TCIFLUSH);
  tcsetattr(fd,TCSANOW,&newtio);

  // send it to the serial port
  write(fd, SENSOR_REQUEST_MESSAGE, strlen(SENSOR_REQUEST_MESSAGE));

  sleep (1);

  // read it from the serial port.
  memset(buf_msg, 0x00, sizeof(buf_msg));
  read(fd, buf_msg, sizeof(buf_msg));
  //PRINT_MSG("\n\r %s", buf_msg);

  close(fd);

  if (!strncmp("C", buf_msg, strlen("C")))
  {
    PRINT_MSG("SENSOR\t>>>>>>\tCHANGED !!!!!\n\r");
    return OK;
  }
  else if (!strncmp("N", buf_msg, strlen("N")))
  {
    PRINT_MSG("SENSOR\t>>>>>>\tNORMAL\n\r");
    return OK;
  }
  else
  {
    PRINT_MSG("SENSOR\t>>>>>>\tREQUEST ERROR\n\r");
    return OK;
  }
  
  return ERROR;
}



/* Comment ********************************************************************
 * GYRO 센서를 체크합니다.
 *
 */
int gyro_list_test(void)
{
  int fd;
  struct termios newtio;
  char buf_msg[64];

  fd = open(AUX_CONSOLE_INF, O_RDWR | O_NOCTTY );

  if (fd < 0) {
    PRINT_MSG("SENSOR\t>>>>>>\tAUX ERROR\n\r");
    return ERROR;
  }

  tcgetattr(fd,&newtio);
  newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;
  // set input mode (non-canonical, no echo,...)
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME] = 1;
  newtio.c_cc[VMIN] = 0;

  tcflush(fd, TCIFLUSH);
  tcsetattr(fd,TCSANOW,&newtio);

  // send it to the serial port
  write(fd, "R", 1);

  sleep (1);

  // read it from the serial port.
  memset(buf_msg, 0x00, sizeof(buf_msg));
  read(fd, buf_msg, sizeof(buf_msg));
  //PRINT_MSG("\n\r %s", buf_msg);

  close(fd);
  PRINT_MSG("SENSOR LIST : %s\n\r",buf_msg);

  return OK;
}



/* Comment ********************************************************************
 * GYRO 상태값을 초기화 하도록 합니다.
 *
 */
int gyro_init(void)
{
  int fd;
  struct termios newtio;
  char buf_msg[64];

  fd = open(AUX_CONSOLE_INF, O_RDWR | O_NOCTTY );

  if (fd < 0) {
    PRINT_MSG("SENSOR\t>>>>>>\tAUX ERROR\n\r");
    return ERROR;
  }

  tcgetattr(fd,&newtio);
  newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;
  // set input mode (non-canonical, no echo,...)
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME] = 1;
  newtio.c_cc[VMIN] = 0;

  tcflush(fd, TCIFLUSH);
  tcsetattr(fd,TCSANOW,&newtio);

  // send it to the serial port
  write(fd, SENSOR_INIT_MESSAGE, strlen(SENSOR_INIT_MESSAGE));

  sleep (1);

  // read it from the serial port.
  memset(buf_msg, 0x00, sizeof(buf_msg));
  read(fd, buf_msg, sizeof(buf_msg));
  //PRINT_MSG("\n\r %s", buf_msg);

  close(fd);

  if (!strncmp("i", buf_msg, strlen("i")))
  {
    PRINT_MSG("SENSOR\t>>>>>>\tINIT !!!!!\n\r");
    return OK;
  }
  else
  {
    PRINT_MSG("SENSOR\t>>>>>>\tREQUEST ERROR\n\r");
    return OK;
  }
  
  return ERROR;
}




/* Comment ********************************************************************
 * main function.
 *
 */
int main(int argc, char *argv[])
{
  for (;;)
    {
      //gyro_test();
      gyro_test ();
      gyro_test ();
      gyro_test ();
      gyro_init ();
    }

  return -1;
}


