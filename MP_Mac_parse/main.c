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
 * main function.
 *
 */
int main(int argc, char *argv[])
{
  char *mac = "00:1f:00:00:43:03";
  unsigned int mac_addr [6];                                                                   

  printf("#J: mac = %s \r\n", mac);

  if (sscanf(mac, "%x:%x:%x:%x:%x:%x",
             &mac_addr[0],
             &mac_addr[1],
             &mac_addr[2],
             &mac_addr[3],
             &mac_addr[4],
             &mac_addr[5]) < 6)
  {
      fprintf(stderr, "could not parse %s\n", mac);
  }


  printf("#J: new_mac = %x:%x:%x:%x:%x:%x \r\n", 
    mac_addr[0], 
    mac_addr[1], 
    mac_addr[2], 
    mac_addr[3], 
    mac_addr[4], 
    mac_addr[5]);
#if 0
  u64 mac = *pmac + offset;                                                        
  int r;                                                                           
                                                                                   
  new_mac[0] = (mac >> 40) & 0xff;                                                 
  new_mac[1] = (mac >> 32) & 0xff;                                                 
  new_mac[2] = (mac >> 24) & 0xff;                                                 
  new_mac[3] = (mac >> 16) & 0xff;                                                 
  new_mac[4] = (mac >> 8) & 0xff;                                                  
  new_mac[5] = mac & 0xff;   

#endif

  return 0;
}


