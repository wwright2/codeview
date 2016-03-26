#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/io.h>
#include <unistd.h>

#define SIO_INDEX 0x2e
#define SIO_DATA 0x2f
#define WD_CONTROL 0x71
#define WD_CONFIG 0x72
#define WD_LSB 0x73
#define WD_MSB 0x74
#define WDTC_SECONDS 0x80
#define WDTC_KRST 0x40
#define FORCE_WD 0xffff

void usage(void)
{
  printf("Usage: watchdog [OPTION]\n");
  printf(" -h           print usage\n");
  printf(" -e<seconds>  enable watchdog for <seconds> (1-65535)\n");
  printf(" -d           disable watchdog\n");
  printf(" -v           verbose query watchdog registers\n");
  printf(" -w           activate watchdog (reboot) immediately\n");
  exit(8);
}

void enter_pnp_mode(void)
{
  /* set up the GPIO port */
  outb(0x87, SIO_INDEX); /* select the SuperIO chip */
  outb(0x01, SIO_INDEX); /* write value per mfg */
  outb(0x55, SIO_INDEX); /* write value per mfg */
  outb(0x55, SIO_INDEX); /* write value per mfg */

  outb(0x07, SIO_INDEX); /* select the LDN register */
  outb(0x07, SIO_DATA); /* write 0x7 to LDN  */
}

void exit_pnp_mode(void)
{
  /* exit PNP mode */
  outb(0x02, SIO_INDEX);
  outb(0x02, SIO_DATA);
}

void setup_watchdog(unsigned int seconds)
{
  unsigned char msb, lsb;

  lsb = (unsigned char) (seconds & 0xff);
  msb = (unsigned char) (seconds >> 8);

  if (seconds)
    {
      outb(WD_MSB, SIO_INDEX); /* select the watchdog MSB timer register */
      outb(msb, SIO_DATA); /* write the watchdog timer MSB */
      outb(WD_LSB, SIO_INDEX); /* select the watchdog LSB timer register */
      outb(lsb, SIO_DATA); /* write the watchdog timer LSB */
    }
  outb(WD_CONFIG, SIO_INDEX); /* select the watchdog config register */
  outb(WDTC_SECONDS | WDTC_KRST, SIO_DATA); /* write the watchdog control register (count register = seconds, enable KRST) */
}

void disable_watchdog(void)
{
  outb(WD_CONFIG, SIO_INDEX); /* select the watchdog config register */
  outb(0x00, SIO_DATA); /* write 0x00 to the watchdog control register to disable it */
}

void read_watchdog(void)
{
  unsigned char lsb = 0;
  unsigned char msb = 0;
  unsigned char config = 0;

  outb(WD_MSB, SIO_INDEX); /* select the watchdog MSB register */
  msb = inb(SIO_DATA); /* read register */

  outb(WD_LSB, SIO_INDEX); /* select the watchdog LSB register */
  lsb = inb(SIO_DATA); /* read register */

  outb(WD_CONFIG, SIO_INDEX); /* select the watchdog LSB register */
  config = inb(SIO_DATA); /* read register */

  printf("watchdog: msb = 0x%x, lsb = 0x%x, config = 0x%x\n", msb, lsb, config);
}

void trip_watchdog(void)
{
  outb(WD_CONFIG, SIO_INDEX); /* select the watchdog config register */
  outb(0xc0, SIO_DATA); /* write the watchdog control register */
  outb(WD_CONTROL, SIO_INDEX); /* select the watchdog control register */
  outb(0x02, SIO_DATA); /* force WD to go off! */
}  

int main(int argc, char **argv)
{
  int result;
  unsigned int getvalue = 0;
  unsigned int inputvalue = 0;
  unsigned char enable = 0, disable = 0, verbose = 0, activate = 0;

  /* set the I/O privilege level */
  if ((result = iopl(3)))
    {
      printf("watchdog: Unable to raise I/O privalege level. Error = %d\n", errno);
      return -1;
    }

  /* process command line options */
  while ((argc > 1) && (argv[1][0] == '-'))
    {
      switch (argv[1][1])
	{
	case 'e':
	  enable = 1;
	  sscanf(&argv[1][2], "%d", &inputvalue);
	  printf("%s: enable watchdog", argv[0]);
	  if (inputvalue)
	    printf(" for %d seconds", inputvalue);
	  printf("\n");
	  break;
	  
	case 'd':
	  disable = 1;
	  break;
	  
	case 'w':
	  activate = 1;
	  break;
	  
	case 'v':
	  verbose = 1;
	  break;
	  
	case 'h':
	  usage();
	  break;

	default:
	  printf("%s: invalid argument: %s\n", argv[0], argv[1]);
	  usage();
	}

      ++argv;
      --argc;
    }

  /* to set up the IT87 super IO chip, must enter PnP mode */
  enter_pnp_mode();

  if (enable)
    {
      setup_watchdog(inputvalue);
      if (verbose)
	read_watchdog();
    }
  else if (disable)
    {
      disable_watchdog();
      if (verbose)
	read_watchdog();
    }
  else if (verbose)
    read_watchdog();
  else if (activate)
    trip_watchdog();

  /* exit PnP mode per datasheet */
  exit_pnp_mode();
 
  return 0;
}
