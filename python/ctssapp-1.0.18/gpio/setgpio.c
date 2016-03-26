#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/io.h>
#include <unistd.h>

#define GPIO_PORT_OUT 0x0a21
#define GPIO_PORT_IN 0x0a22
#define SIO_INDEX 0x2e
#define SIO_DATA 0x2f

// #define DEBUG 1 // define only if you want debug messages to stdout

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

void setup_gpio_regs(void)
{
  outb(0x26, SIO_INDEX); /* select GPIO set 2 config register */
  outb(0xFF, SIO_DATA); /* set all pins as GPIO */
  outb(0x27, SIO_INDEX); /* select GPIO set 3 config register */
  outb(0xFF, SIO_DATA); /* set all pins as GPIO */
  outb(0xB1, SIO_INDEX); /* select GPIO set 2 invert register */
  outb(0x00, SIO_DATA); /* set GP0-7 as non invert mode */
  outb(0xB2, SIO_INDEX); /* select GPIO set 3 invert register */
  outb(0x00, SIO_DATA); /* set GP0-7 as non invert mode */
  outb(0xC1, SIO_INDEX); /* select GPIO set 2 simple register */
  outb(0xFF, SIO_DATA); /* set GP0-7 as simple IO mode */
  outb(0xC2, SIO_INDEX); /* select GPIO set 3 simple register */
  outb(0xFF, SIO_DATA); /* set GP0-7 as simple IO mode */
  outb(0xBA, SIO_INDEX); /* select GPIO set 3 pull-up register */
  outb(0xF0, SIO_DATA); /* set GP0-3 as no pull-up */
  outb(0xC9, SIO_INDEX); /* select GPIO set 2 direction register */
  outb(0x0F, SIO_DATA); /* set GP0-3 as output */
  outb(0xCA, SIO_INDEX); /* select GPIO set 3 direction register */
  outb(0xF0, SIO_DATA); /* set GP0-3 as input */
}

void read_gpio_regs(void)
{
  unsigned char regvalue = 0;

  outb(0x26, SIO_INDEX); /* select GPIO set 2 config register */
  regvalue = inb(SIO_DATA); /* read register */
  printf("0x26 = 0x%02x, ", regvalue);

  outb(0x27, SIO_INDEX); /* select GPIO set 3 config register */
  regvalue = inb(SIO_DATA); /* read register */
  printf("0x27 = 0x%02x, ", regvalue);

  outb(0xB1, SIO_INDEX); /* select GPIO set 2 invert register */
  regvalue = inb(SIO_DATA); /* read register */
  printf("0xB1 = 0x%02x, ", regvalue);

  outb(0xB2, SIO_INDEX); /* select GPIO set 3 invert register */
  regvalue = inb(SIO_DATA); /* read register */
  printf("0xB2 = 0x%02x, ", regvalue);

  outb(0xC1, SIO_INDEX); /* select GPIO set 2 simple register */
  regvalue = inb(SIO_DATA); /* read register */
  printf("0xC1 = 0x%02x, ", regvalue);

  outb(0xC2, SIO_INDEX); /* select GPIO set 3 simple register */
  regvalue = inb(SIO_DATA); /* read register */
  printf("0xC2 = 0x%02x, ", regvalue);

  outb(0xBA, SIO_INDEX); /* select GPIO set 3 pull-up register */
  regvalue = inb(SIO_DATA); /* read register */
  printf("0xBA = 0x%02x, ", regvalue);

  outb(0xC9, SIO_INDEX); /* select GPIO set 2 direction register */
  regvalue = inb(SIO_DATA); /* read register */
  printf("0xC9 = 0x%02x ", regvalue);

  outb(0xCA, SIO_INDEX); /* select GPIO set 3 direction register */
  regvalue = inb(SIO_DATA); /* read register */
  printf("0xCA = 0x%02x\n", regvalue);
}

int main(int argc, char **argv)
{
  int result;
  unsigned char setvalue;
  unsigned int inputvalue;
  unsigned char dioinvalue;
  unsigned char diooutvalue;

  /* set the I/O privilege level */
  if ((result = iopl(3)))
    {
      printf("setgpio: Unable to raise I/O privalege level. Error = %d\n", errno);
      return -1;
    }

  /* to set up the IT87 super IO chip, must enter PnP mode */
  enter_pnp_mode();

  /* configure the GPIO subsystem */
  setup_gpio_regs();

#ifdef DEBUG
  /* read the registers and display them */
  read_gpio_regs();
#endif

  /* exit PnP mode per datasheet */
  exit_pnp_mode();
 
  /* Read the DIO input register */
  dioinvalue = inb(GPIO_PORT_IN);

  /* Read the DIO output register */
  diooutvalue = inb(GPIO_PORT_OUT);

  /* check for command line parameter */
  if (argc > 1)
    sscanf(argv[1], "%x", &inputvalue);
  else
    {
#ifdef DEBUG
      printf("setgpio: contents of GPIO input port is 0x%02x\n", dioinvalue);
      printf("setgpio: contents of GPIO output port is 0x%02x\n", diooutvalue);
#endif
      return 0;
    }

  /* Set the output */
  setvalue = inputvalue & 0xff;

  /* Write to the DIO port */
  outb(setvalue, GPIO_PORT_OUT);

#ifdef DEBUG
  /* Read the DIO port again */
  diooutvalue = inb(GPIO_PORT_OUT);

  printf("setgpio: contents of GPIO output port is 0x%02x\n", diooutvalue);
#endif

  return 0;
}
