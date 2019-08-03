#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include "pruss/prussdrv.h"
#include "pruss/pruss_intc_mapping.h"
#include "mio.h"
#include "AM335X_GPIO.h"

// PRU data declarations
#define PRU_NUM0 0
#define PRU_NUM1 1

#define PATH_PRU_BINS "./"

extern volatile unsigned int   *gpio0_setdataout_addr;
extern volatile unsigned int   *gpio1_setdataout_addr;
extern volatile unsigned int   *gpio2_setdataout_addr;
extern volatile unsigned int   *gpio3_setdataout_addr;
extern volatile unsigned int   *gpio0_cleardataout_addr;
extern volatile unsigned int   *gpio1_cleardataout_addr;
extern volatile unsigned int   *gpio2_cleardataout_addr;
extern volatile unsigned int   *gpio3_cleardataout_addr;

#define GPIO1_21_PIN 0x00200000         /* User LED_0  */
#define GPIO1_22_PIN 0x00400000         /* User LED_1  */
#define GPIO1_23_PIN 0x00800000         /* User LED_2  */
#define GPIO1_24_PIN 0x01000000         /* User LED_3 */

#define GPIO0_27_PIN 0x08000000         /* BAT-LEV-1  */
#define GPIO0_11_PIN 0x00000800         /* BAT-LEV-2  */
#define GPIO1_29_PIN 0x20000000         /* BAT-LEV-3  */
#define GPIO0_26_PIN 0x04000000         /* BAT-LEV-4  */


void Cycle_LEDS(void);


/* sigint handler */
static volatile unsigned int is_sigint = 0;
static volatile unsigned int is_sigterm = 0;

static void sig_handler (int sig_num)
{
	printf ("***** on_sigint(%d) *****\n", sig_num);

	if (sig_num == SIGINT)
	{
	//	printf ("***** on_sigint(%d) = SIGINT *****\n", sig_num);
		is_sigint = 1;
	}
	else if (sig_num == SIGTERM)
	{
	//	printf ("***** on_sigint(%d) = SIGTERM *****\n", sig_num);
		is_sigterm = 1;
	}
}

/* main */
int main (int ac, char** av)
{
	int			ret;
	tpruss_intc_initdata 	pruss_intc_initdata = PRUSS_INTC_INITDATA;
//	uint16_t		nLedState = 1;

//	uint32_t x[32], x2[64];
//	const size_t n = sizeof(x) / sizeof(x[0]);
//	const size_t n2 = sizeof(x2) / sizeof(x2[0]);
//	size_t i;

	/* Setup the SIGINT signal handling */
	if (signal(SIGINT, sig_handler) == SIG_ERR)
	{
  		printf ("\n**** Can't start SIGINT handler ****\n");
	}
	if (signal(SIGTERM, sig_handler) == SIG_ERR)
	{
  		printf ("\n**** Can't start SIGTERM handler ****\n");
	}

	gpio_fast_init (0);

	/* Setup the GPIO pins */
	/* Red User LED */
	int	gpio_num = 66;
	gpio_export (gpio_num);
	gpio_set_dir (gpio_num, GPIO_OUTPUT);

	/* Green User LED */
	gpio_num = 67;
	gpio_export (gpio_num);
	gpio_set_dir (gpio_num, GPIO_OUTPUT);

	/* Battery Level #1 LED (Red) */
	gpio_num = 27;
	gpio_export (gpio_num);
	gpio_set_dir (gpio_num, GPIO_OUTPUT);

	/* Battery Level #2 LED (Green) */
	gpio_num = 11;
	gpio_export (gpio_num);
	gpio_set_dir (gpio_num, GPIO_OUTPUT);

	/* Battery Level #3 LED (Green) */
	gpio_num = 61;
	gpio_export (gpio_num);
	gpio_set_dir (gpio_num, GPIO_OUTPUT);

	/* Battery Level #4 LED (Green) */
	gpio_num = 26;
	gpio_export (gpio_num);
	gpio_set_dir (gpio_num, GPIO_OUTPUT);

	/* P9.11 - GPIO0.30 - UART4-Rx */
	gpio_num = 30;
	gpio_export (gpio_num);
	gpio_set_dir (gpio_num, GPIO_INPUT);

	Cycle_LEDS();

	/* Initialize the PRUs */
	/* If this segfaults, make sure you're executing as root. */
	ret = prussdrv_init();
	if (0 != ret)
	{
		printf ("prussdrv_init() failed\n");
		return (ret);
	}

	/* Open PRU event Interrupt */
	ret = prussdrv_open (PRU_EVTOUT_0);
	if (ret)
	{
		printf ("prussdrv_open failed\n");
		return (ret);
	}

	/* Get the PRU interrupt initialized */
	ret = prussdrv_pruintc_init (&pruss_intc_initdata);
	if (ret != 0)
	{
		printf ("prussdrv_pruintc_init() failed\n");
		return (ret);
	}

	/* Write program data from data.bin to pru-0 */
	ret = prussdrv_load_datafile (PRU_NUM0, PATH_PRU_BINS"pru0_data.bin");
	if (ret < 0)
	{
		printf ("prussdrv_load_datafile(PRU-0) failed\n");
		return (ret);
	}

	/* Write program data from data.bin to pru-1 */
	ret = prussdrv_load_datafile (PRU_NUM1, PATH_PRU_BINS"pru1_data.bin");
	if (ret < 0)
	{
		printf ("prussdrv_load_datafile(PRU-1) failed\n");
		return (ret);

	}

	/* Load/Execute code on pru-0 */
	prussdrv_exec_program_at (PRU_NUM0, PATH_PRU_BINS"pru0_text.bin", PRU0_START_ADDR);
	if (ret < 0)
	{
		printf ("prussdrv_exec_program_at(PRU-0) failed\n");
		return (ret);
	}

	/* Load/Execute code on pru-1 */
	prussdrv_exec_program_at (PRU_NUM1, PATH_PRU_BINS"pru1_text.bin", PRU1_START_ADDR);
	if (ret < 0)
	{
		printf ("prussdrv_exec_program_at(PRU-1) failed\n");
		return (ret);
	}

	/* Wait for PRUs */
	printf ("\tINFO: Pausing for Ctl-C signal.\r\n");

	while (!is_sigint && !is_sigterm)
	{
#if 0
		switch (nLedState)
                {
                        case 1:         /* Turn on #1 */
                                *gpio0_setdataout_addr = GPIO0_27_PIN;
                                *gpio0_cleardataout_addr = GPIO0_11_PIN;
                                *gpio1_cleardataout_addr = GPIO1_29_PIN;
                                *gpio0_cleardataout_addr = GPIO0_26_PIN;
				nLedState = 2;
                                break;
                        case 2:         /* Turn on #2 */
                                *gpio0_cleardataout_addr = GPIO0_27_PIN;
                                *gpio0_setdataout_addr = GPIO0_11_PIN;
                                *gpio1_cleardataout_addr = GPIO1_29_PIN;
                                *gpio0_cleardataout_addr = GPIO0_26_PIN;
				nLedState = 3;
                                break;
                        case 3:         /* Turn on #1 */
                                *gpio0_cleardataout_addr = GPIO0_27_PIN;
                                *gpio0_cleardataout_addr = GPIO0_11_PIN;
                                *gpio1_setdataout_addr = GPIO1_29_PIN;
                                *gpio0_cleardataout_addr = GPIO0_26_PIN;
				nLedState = 4;
                                break;
                        case 4:         /* Turn on #1 */
                                *gpio0_cleardataout_addr = GPIO0_27_PIN;
                                *gpio0_cleardataout_addr = GPIO0_11_PIN;
                                *gpio1_cleardataout_addr = GPIO1_29_PIN;
                                *gpio0_setdataout_addr = GPIO0_26_PIN;
				nLedState = 1;
                                break;
                        default:
                                nLedState = 1;
                                break;
                };
#endif
                usleep(500000);
	}

	printf ("\tINFO: Shutting down...\r\n");



//	printf ("\tINFO: Shutting down PRU-%d.\r\n", (int)PRU_NUM0);
	prussdrv_pru_send_event (ARM_PRU0_INTERRUPT);

//	printf ("\tINFO: Shutting down PRU-%d.\r\n", (int)PRU_NUM1);
	prussdrv_pru_send_event (ARM_PRU1_INTERRUPT);

	/* Wait until PRU has finished execution */
//	printf ("\tINFO: Waiting for HALT command from PRU.\r\n");
	ret = prussdrv_pru_wait_event (PRU_EVTOUT_0);
	printf ("\tINFO: PRU program completed, event number %d\n", ret);

	usleep(100000);

	/* Clear PRU events */
	if (0 != (ret = prussdrv_pru_clear_event (PRU_EVTOUT_0, PRU0_ARM_INTERRUPT)))
	{
		printf ("prussdrv_pru_clear_event() failed, result = %d\n", ret);
		return (ret);
	}
	if (0 != (ret = prussdrv_pru_clear_event (PRU_EVTOUT_0, PRU1_ARM_INTERRUPT)))
	{
		printf ("prussdrv_pru_clear_event() failed, result = %d\n", ret);
		return (ret);
	}

	/* Disable PRU-0 and close memory mapping*/
	if (0 != (ret =	prussdrv_pru_disable (PRU_NUM0)))
	{
		printf ("prussdrv_pru_disable() failed\n");
		return (ret);
	}

	/* Disable PRU-0 and close memory mapping*/
	if (0 != (ret =	prussdrv_pru_disable (PRU_NUM1)))
	{
		printf ("prussdrv_pru_disable() failed\n");
		return (ret);
	}

	if (0 != (ret = prussdrv_exit()))
	{
		printf ("prussdrv_pru_exit() failed\n");
		return (ret);
	}

	/* Release GPIO exports */
	gpio_num = 66;
	gpio_unexport (gpio_num);
	gpio_num = 67;
	gpio_unexport (gpio_num);

	gpio_num = 27;
	gpio_unexport (gpio_num);
	gpio_num = 11;
	gpio_unexport (gpio_num);
	gpio_num = 61;
	gpio_unexport (gpio_num);
	gpio_num = 26;
	gpio_unexport (gpio_num);

	gpio_num = 30;
	gpio_unexport (gpio_num);

	return 0;
}

void Cycle_LEDS(void)
{
	int nLedState = 1;
	int nCycles = 0;

	while (nCycles++ < 25)
	{
		switch (nLedState)
		{
			case 1:         /* Turn on #1 */
				*gpio0_setdataout_addr = GPIO0_27_PIN;
				*gpio0_cleardataout_addr = GPIO0_11_PIN;
				*gpio1_cleardataout_addr = GPIO1_29_PIN;
				*gpio0_cleardataout_addr = GPIO0_26_PIN;
				nLedState = 2;
                                break;
			case 2:         /* Turn on #2 */
				*gpio0_cleardataout_addr = GPIO0_27_PIN;
				*gpio0_setdataout_addr = GPIO0_11_PIN;
				*gpio1_cleardataout_addr = GPIO1_29_PIN;
				*gpio0_cleardataout_addr = GPIO0_26_PIN;
				nLedState = 3;
                                break;
			case 3:         /* Turn on #3 */
				*gpio0_cleardataout_addr = GPIO0_27_PIN;
				*gpio0_cleardataout_addr = GPIO0_11_PIN;
				*gpio1_setdataout_addr = GPIO1_29_PIN;
				*gpio0_cleardataout_addr = GPIO0_26_PIN;
				nLedState = 4;
                                break;
			case 4:         /* Turn on #4 */
				*gpio0_cleardataout_addr = GPIO0_27_PIN;
				*gpio0_cleardataout_addr = GPIO0_11_PIN;
				*gpio1_cleardataout_addr = GPIO1_29_PIN;
				*gpio0_setdataout_addr = GPIO0_26_PIN;
				nLedState = 1;
                                break;
			default:
				nLedState = 1;
				break;
		}
		usleep(50000);
	};

	/* Turn all LEDs off */
	*gpio0_cleardataout_addr = GPIO0_27_PIN;
	*gpio0_cleardataout_addr = GPIO0_11_PIN;
	*gpio1_cleardataout_addr = GPIO1_29_PIN;
	*gpio0_cleardataout_addr = GPIO0_26_PIN;
}
