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


void serial_test (void)
{
  int fd;
  struct termios newtio;
  char buf_msg[64];

  fd = open("/dev/ttyS1", O_RDWR);

  if (fd < 0) {
    return;
  }

  tcgetattr(fd,&newtio);
  newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME] = 1;
  newtio.c_cc[VMIN] = 0;

  tcflush(fd, TCIFLUSH);
  tcsetattr(fd,TCSANOW,&newtio);

  for (;;)
  {
    memset(buf_msg, 0x00, sizeof(buf_msg));
    read(fd, buf_msg, sizeof(buf_msg));
    printf("TEST: %s \r\n", buf_msg);

    // send it to the serial port
    write(fd, "abcd", 4);
  }

  close(fd);
}


/* Comment ********************************************************************
 * main function.
 *
 */
int main(int argc, char *argv[])
{
  system ("echo 'RS485 Return Test");

  serial_test ();

  return 0;
}


