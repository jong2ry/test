#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>

#define BAUDRATE B115200
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define DBG_PRINT	printf

int STOP=FALSE;

int fileExists(const char* file) {
    struct stat buf;
    return (stat(file, &buf) == 0);
}

int open_serial_port(const char *ser_dev)
{
	int fd;
	struct termios oldtio,newtio;
	
	fd = open(ser_dev, O_RDWR | O_NOCTTY );
	if (fd <0) 
		return -1;

	DBG_PRINT("#Open %s : fd=%d !\n", ser_dev, fd);

	
	tcgetattr(fd,&oldtio); /* 현재 설정을 oldtio에 저장 */
	
	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
	
	/* set input mode (non-canonical, no echo,...) */
	newtio.c_lflag = 0;
	
	newtio.c_cc[VTIME]	  = 0;	 /* 문자 사이의 timer를 disable */
	newtio.c_cc[VMIN]	  = 0;	 /*  */
	
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd,TCSANOW,&newtio);

	return fd;
	
}

int read_data(int fd, char *read_data)
{
	char buf[255];
	int res;

	bzero(buf, 255);

	if(fd>0)
	{
		res = read(fd,buf,255);
		if(res > 0)
		{
			memcpy(read_data, buf, res);
			//DBG_PRINT("#read data %s, %d\n", buf, res);
		}

		return res;	
	}

	return -1;
}








void use_all_usbport (void)
{
	int ttyusb0_fd=-1, ttyusb1_fd=-1, aux_fd;
	int read_len0, read_len1, read_len_aux;
	//the ttyusb1 is mostly a console port.. but I'm not sure....
	int aux_out_fd=1;
	char read_data0[255], read_data1[255], read_aux[255];

	aux_fd = open_serial_port("/dev/ttyS0");

	while(1)
	{
		if(ttyusb0_fd<=0)
			ttyusb0_fd = open_serial_port("/dev/ttyUSB0");

		if(ttyusb1_fd<=0)
			ttyusb1_fd = open_serial_port("/dev/ttyUSB1");

		bzero(read_data0, 255);
		bzero(read_data1, 255);
		
		read_len0 = read_data(ttyusb0_fd, read_data0);
		read_len1 = read_data(ttyusb1_fd, read_data1);

		if( ttyusb0_fd>0 && !fileExists("/dev/ttyUSB0"))
		{
			DBG_PRINT("#close 0 %d\n", ttyusb0_fd);
			close(ttyusb0_fd);
			ttyusb0_fd=-1;
			if(aux_out_fd==0)
				aux_out_fd=-1;
		}
		
		if( ttyusb1_fd>0 && !fileExists("/dev/ttyUSB1"))
		{
			DBG_PRINT("#close 1 %d\n", ttyusb1_fd);
			close(ttyusb1_fd);
			ttyusb1_fd=-1;
			if(aux_out_fd==1)
				aux_out_fd=-1;
		}		

		if(read_len0>0 && ttyusb0_fd>0)
		{
			printf("0\n");
			write(aux_fd, read_data0, read_len0);
			aux_out_fd=0;
		}

		if(read_len1>0 && ttyusb1_fd>0)
		{
			printf("1\n");
			write(aux_fd, read_data1, read_len1);
			aux_out_fd=1;
		}

		bzero(read_aux, 255);
		read_len_aux = read_data(aux_fd, read_aux);

		if(read_len_aux > 0)
		{
			if(aux_out_fd==0)
				write(ttyusb0_fd, read_aux, read_len_aux);

			if(aux_out_fd==1)
				write(ttyusb1_fd, read_aux, read_len_aux);
		}	
	}
}




void use_standard_usbport (void)
{
	int ttyusb1_fd=-1, aux_fd;
	int read_len1, read_len_aux;
	//the ttyusb1 is mostly a console port.. but I'm not sure....
	char read_data1[255], read_aux[255];

	aux_fd = open_serial_port("/dev/ttyS0");

	while(1)
	{
		if(ttyusb1_fd<=0)
			ttyusb1_fd = open_serial_port("/dev/ttyUSB1");

		bzero(read_data1, 255);
		
		read_len1 = read_data(ttyusb1_fd, read_data1);

		if( ttyusb1_fd>0 && !fileExists("/dev/ttyUSB1"))
		{
			DBG_PRINT("#close 1 %d\n", ttyusb1_fd);
			close(ttyusb1_fd);
			ttyusb1_fd=-1;
		}		

		if(read_len1>0 && ttyusb1_fd>0)
		{
			printf("1\n");
			write(aux_fd, read_data1, read_len1);
		}

		bzero(read_aux, 255);
		read_len_aux = read_data(aux_fd, read_aux);

		if(read_len_aux > 0)
		{
		  write(ttyusb1_fd, read_aux, read_len_aux);
		}	
	}
}


int main(int argc, char *argv[])
{
  printf ("argc = %d \n", argc);

  if (argc == 1)
  {
			printf("USE STANDARD USB PORT (ttyUSB1)\n");
      use_standard_usbport ();
      return 0;
  }

  if (argc == 2 && !strncmp ("all", argv[1], strlen ("all"))) 
  {
			printf("USE ALL USB PORT (ttyUSB0, ttyUSB1)\n");
      use_all_usbport ();
      return 0;
  }

  printf("\n\nWrong command\n"); 
  printf(" - USRP [all]\n"); 
 
  return 0;
}
