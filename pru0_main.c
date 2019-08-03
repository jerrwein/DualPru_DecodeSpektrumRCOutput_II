/* Notes:
 *
 * The following version is intended to read serial input from P9.11
 * which is routed through GPIO0.30
 * Make certain P9.11 is exported & configured as in input in host_main.c
 *
 * Alternativly, input can be taken from the DSMX serial port (CONN-13)
 * This signal is input via PRU0-R30.5
 *
 * ----- Run with BBG-PRUS-1G & BBG-GPIO-1E device overlays loaded -----
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include "linux_types.h"

// Leave this defined for simulated DXe traffic.
#define USE_UART4_RX 1

#ifndef USE_UART4_RX
  #define USE_SPEKTRUM_RX 1
  #define RX_BAUD_RATE 115200
#else
  #define RX_BAUD_RATE 115200
#endif

#define DPRAM_SHARED 0x00012000
// #define DPRAM_SHARED 0x00003000
//#define DPRAM_SHARED 0x00013000

/* Note: PRU number should be defined prior to pru specific headers */
#define PRU0
#include "pru_defs.h"
#include "pru_sbus.h"
#include "pru_hal.h"

/* ------- Run with BBG-PRUS-1G & BBG-GPIO-1E device overlays loaded -------- */

/* #define GPIO0_27_PIN 0x08000000     BAT-LEV-1  */
/* #define GPIO0_11_PIN 0x00000800     BAT-LEV-2  */
/* #define GPIO1_29_PIN 0x20000000     BAT-LEV-3  */
/* #define GPIO0_26_PIN 0x04000000     BAT-LEV-4  */

// #define GPIO0_START_ADDR 0x44E07000
// #define GPIO1_START_ADDR 0x4804C000
// #define GPIO2_START_ADDR 0x481AC000
// #define GPIO3_START_ADDR 0x481ae000
// #define GPIO_DATAIN (0x138)
// #define GPIO_DATAOUT (0x13C)
// #define GPIO_CLEARDATAOUT (0x190)
// #define GPIO_SETDATAOUT (0x194)

/* This field must also be changed in host_main.c */
#define nPruNum 0

/* This code intended to decode Spectrum DSMX protocol */

#ifdef RX_BAUD_RATE
  /* The following constants are intended for 19.2 KHz. traffic */
  #if (RX_BAUD_RATE == 19200)
  const uint32_t	tm_poll[] = {5208, 15625, 26042, 36458, 46875, 57292,
	  			     67708, 78125, 88542, 98958, 109375 };
  /* The following constants are intended for 57.6 KHz. traffic */
  #elif (RX_BAUD_RATE == 57600)
  const uint32_t	tm_poll[] = { 1736, 5208, 8681, 12153, 15625, 19097,
				      22569, 26042, 29514, 32986, 36458 };
  /* The following constants are intended for 115.2 KHz. traffic */
  /* The following constants are intended to decode 115.2 kHz. Spectrum DSMX protocol */
  #elif (RX_BAUD_RATE == 115200)
  const uint32_t	tm_poll[] = { 868, 2604, 4340, 6076, 7812, 9549,
			   	      11285, 13021, 14757, 16493, 18229 };
  /* The following constants are intended to decode 125 kHz. Spectrum DSMX protocol */
  #elif (RX_BAUD_RATE == 125000)
  const uint32_t	tm_poll[] = { 800, 2400, 4000, 5600, 7200, 8800,
				      10400, 12000, 13600, 15200, 16800 };
  #endif
#else
  #error Please #define a baudrate to use for Rx input.
#endif


/* The following constants are intended for 19.2 KHz. traffic */
//const uint32_t	tm_poll[] = {5208, 15625, 26042, 36458, 46875, 57292,
//							 67708, 78125, 88542, 98958, 109375 };

/* The following constants are intended for 57.6 KHz. traffic */
//const uint32_t	tm_poll[] = { 1736, 5208, 8681, 12153, 15625, 19097,
//							  22569, 26042, 29514, 32986, 36458 };

/* The following constants are intended for 115.2 KHz. traffic */
//const uint32_t	tm_poll[] = { 868, 2604, 4340, 6076, 7812, 9549,
//							  11285, 13021, 14757, 16493, 18229 };

/* The following constants are intended for 125 KHz. Spectrum traffic */
//const uint32_t	tm_poll[] = { 800, 2400, 4000, 5600, 7200, 8800,
//							 10400, 12000, 13600, 15200, 16800 };

typedef enum
{
	ST_LookForIdle,
	ST_LookForStart,
	ST_ConfirmStart,
	ST_ReadData,
	ST_ReadStopOne,
} PRIMARY_STATE;

#if 0
bool read_rx_pin (void)
{
	uint32_t	u32_reg_val;

	__R30 |= 0x00000020;	/* R30.5 - P9.27 */

	/* Read P9.26 - GPIO0.14 */
	u32_reg_val = *((volatile unsigned long *)0x44E07138);

	/* Read P9.11 - GPIO0.30 */
	u32_reg_val = *((volatile unsigned long *)0x44E07138);


	__R30 &= 0xffffffdf;	/* R30.5 - P9.27 */

    return (u32_reg_val & 0x40000000);

    return (u32_reg_val & 0x00004000);
}
#endif

static inline bool read_rx_pin (void)
{
#ifdef USE_UART4_RX
	/* Read P9.11 - GPIO0.30 - Uart4 Rx */
	uint32_t	u32_reg_val;
	u32_reg_val = *((volatile unsigned long *)0x44E07138);
	return (u32_reg_val & 0x40000000);
#else
  #ifdef USE_SPEKTRUM_RX
	/* Read P9.27 - R31.5 - Spektrum Rx */
	return (__R31 & 0x00000020);
  #else
    #error Please #define which I/O pin to use for input.
  #endif
#endif
}

static inline u32 read_PIEP_COUNT(void)
{
    return PIEP_COUNT;
}

void add_to_ring_buffer (uint16_t val, uint16_t delta_tm)
{
    RBUFF->buffer[RBUFF->ring_tail].pin_value = val;
    RBUFF->buffer[RBUFF->ring_tail].delta_t = delta_tm;
    RBUFF->ring_tail = (RBUFF->ring_tail + 1) % NUM_RING_ENTRIES;
}

int main (void)
{
	PRIMARY_STATE 		ePrimState = ST_LookForIdle;
	volatile bool		pin_val, last_pin_val = true;
	volatile uint32_t	tm_start_detected, tm_elapsed, tm_now;
	uint32_t 		tm_last_byte;
	uint8_t 		data_in; //, num_high_bits;
	uint8_t 		data_bit_index, data_bit_mask;
	bool			bStopOne;

	volatile uint32_t	lSameValCnt;
	uint32_t		lLinkActiveCount = 0;
	uint16_t		nLedByteCount = 0;
	uint32_t		lLedCount = 0;
	bool			bLedState = false;
	bool			bDone = false;

	/* PRU Initialisation */
	PRUCFG_SYSCFG &= ~SYSCFG_STANDBY_INIT;
	PRUCFG_SYSCFG = (PRUCFG_SYSCFG &
					~(SYSCFG_IDLE_MODE_M | SYSCFG_STANDBY_MODE_M)) |
					SYSCFG_IDLE_MODE_NO | SYSCFG_STANDBY_MODE_NO;

	/* Our PRU wins arbitration */
	PRUCFG_SPP |=  SPP_PRU1_PAD_HP_EN;

	/* Configure timer */
	PIEP_GLOBAL_CFG = GLOBAL_CFG_DEFAULT_INC(1) | GLOBAL_CFG_CMP_INC(1);
	PIEP_CMP_STATUS = CMD_STATUS_CMP_HIT(1); /* clear the interrupt */
	PIEP_CMP_CMP1   = 0x0;
	PIEP_CMP_CFG |= CMP_CFG_CMP_EN(1);
	PIEP_GLOBAL_CFG |= GLOBAL_CFG_CNT_ENABLE;

	RBUFF->ring_head = 0;
	RBUFF->ring_tail = 0;

	add_to_ring_buffer (0x7531, 0x1357);
	add_to_ring_buffer (0x8642, 0x2468);
	add_to_ring_buffer (0, 0);

	while (!bDone)
	{
// for PRU R31 read		if ((lLinkActiveCount == 0) && (2100000 < ++lLedCount))
		if ((lLinkActiveCount == 0) && (1050000 < ++lLedCount))
		{
			if (bLedState)
			{
				/* Clear P8.8 - LED4 - GPIO2.3 */
//				__R30 |= 0x00000020;	/* R30.5 - P9.27 */
//				*((volatile unsigned long *)0x481ac190) = 0x00000008;
//				*((volatile unsigned long *)0x481ac190) |= 0x00000008;
				/* Clear BAT-LEV-0 LED1 - GPIO0.27 */
//				*((volatile unsigned long *)0x44E07190) = 0x00000800;	/* BAT-LEV-2 */
				*((volatile unsigned long *)0x44E07190) = 0x08000000;	/* BAT-LEV-1 (Red) */
				bLedState = false;
			}
			else
			{
				/* Set P8.8 - LED4 - GPIO2.3 */
//				__R30 &= 0xffffffdf;	/* R30.5 - P9.27 */
//				*((volatile unsigned long *)0x481ac194) = 0x00000008;
//				*((volatile unsigned long *)0x481ac194) |= 0x00000008;
				/* Set BAT-LEV-0 LED1 - GPIO0.27 */
//				*((volatile unsigned long *)0x44E07194) = 0x00000800;	/* BAT-LEV-2 */
				*((volatile unsigned long *)0x44E07194) = 0x08000000;	/* BAT-LEV-1 (Red) */
				bLedState = true;
			}
			lLedCount = 0;
		}

		switch (ePrimState)
		{
			case ST_LookForIdle:
//				__R30 |= 0x00000001;		/* R30.0 - P9.31 */
//				pin_val = __R31 & 0x20;		/* P9.27 - R31.5 - Spektrum Rx */
				pin_val = read_rx_pin();
				if (last_pin_val && pin_val)
				{
//					if (42000 < (++lSameValCnt))	/* ~ 10 ms */
					if (21000 < (++lSameValCnt))	/* ~ 5 ms */
//					if (12750 < (++lSameValCnt))	/* ~ 3 ms */
					{
						lSameValCnt = 0;
						ePrimState = ST_LookForStart;
					}
				}
				last_pin_val = pin_val;
//				__R30 &= 0xfffffffe;	/* R30.0 - P9.31 */
				break;

			case ST_LookForStart:
//				pin_val = __R31 & 0x20;
				pin_val = read_rx_pin();
				if (!last_pin_val && !pin_val)
				{
					if (3 < (++lSameValCnt))	/* ~ 1 us */
					{
						tm_start_detected = read_PIEP_COUNT();
						/* Adjust for the time spent aquiring 4 consequtive values */
						tm_start_detected -= 300;
						lSameValCnt = 0;
						ePrimState = ST_ConfirmStart;
					}
				}
				else
				{
					/* decrement link activity counter */
					if (0 <	lLinkActiveCount)
						lLinkActiveCount--;
//					else
//					{
						/* Clear P9.23 - LED4 */
//						*((volatile unsigned long *)0x4804c190) |= 0x00020000;
//						bLedState = false;
//					}
				}
				last_pin_val = pin_val;
				break;

			case ST_ConfirmStart:
				/* 1/2 bit time - confirm start bit here */
				tm_now = read_PIEP_COUNT();
				tm_elapsed = tm_now - tm_start_detected;
				if (tm_poll[0] <= tm_elapsed)
				{
//					__R30 |= 0x00000001;	/* R30.0 - P9.31 */
//					if (!(__R31 & 0x20))
					if (!read_rx_pin())
					{
						data_in = 0;
//						num_high_bits = 0;
						ePrimState = ST_ReadData;
						data_bit_index = 1;
						data_bit_mask = 0x01;
					}
					else
						ePrimState = ST_LookForIdle;
//					__R30 &= 0xffffff01;	/* R30.0 - P9.31 */
				}
				break;

			case ST_ReadData:
				/* 1 1/2 - 8 1/2 bit times - read D0-D7 here */
				tm_now = read_PIEP_COUNT();
				tm_elapsed = tm_now - tm_start_detected;
				if (tm_poll[data_bit_index] <= tm_elapsed)
				{
//					__R30 |= 0x00000001;	/* R30.0 - P9.31 */
//					if (__R31 & 0x20)
					if (read_rx_pin())
					{
						data_in |= data_bit_mask;
//						num_high_bits++;	/* Used later for parity check */
					}
					data_bit_mask <<= 1;
					data_bit_index++;
					if (8 < data_bit_index)
						ePrimState = ST_ReadStopOne;
//					__R30 &= 0xfffffffe;	/* R30.0 - P9.31 */
				}
				break;

			case ST_ReadStopOne:
				/* 9 1/2 bit times - read stop bit here */
				tm_now = read_PIEP_COUNT();
				tm_elapsed = tm_now - tm_start_detected;
				if (tm_poll[9] <= tm_elapsed)
				{
//					__R30 |= 0x00000001;	/* R30.0 - P9.31 */
//					pin_val = (__R31 & 0x20) != 0;
					pin_val = read_rx_pin();
					if (pin_val)
						bStopOne = true;
					else
						bStopOne = false;

					/* Start with new byte */
					last_pin_val = pin_val;
					ePrimState = ST_LookForStart;

					/* Make sure all conditions are met */
					if (bStopOne)
					{
						/* Time delta between bytes in us. */
						tm_elapsed = (tm_now - tm_last_byte) / 200;
						tm_last_byte = tm_now;
						add_to_ring_buffer ((uint16_t)tm_elapsed, data_in);
#if 1
						/* Link active LED flash */
						if (50 < ++nLedByteCount)
						{
							if (bLedState)
							{
								/* Clear P8.8 - LED4 - GPIO2.3 */
//								*((volatile unsigned long *)0x481ac190) |= 0x00000008;
//								*((volatile unsigned long *)0x481ac190) = 0x00000008;
								*((volatile unsigned long *)0x44E07190) = 0x08000000;

								bLedState = false;
							}
							else
							{
								/* Set P8.8 - LED4 - GPIO2.3 */
//								*((volatile unsigned long *)0x481ac194) |= 0x00000008;
//								*((volatile unsigned long *)0x481ac194) = 0x00000008;
								*((volatile unsigned long *)0x44E07194) = 0x08000000;

								bLedState = true;
							}
							nLedByteCount = 0;
						}
						/* This maitains LED activity while positive */
						lLinkActiveCount = 250000;
#endif
					}
					else
						add_to_ring_buffer (0x0000, data_in);
//					__R30 &= 0xfffffffe;	/* R30.0 - P9.31 */
				}
				break;

			default:
				ePrimState = ST_LookForIdle;
		} /* End of switch (ePrimState) */

		// Exit if we receive a Host->PRU0 interrupt
		if (__R31 & 0x40000000)
			bDone = true;
	};

	/* Clear P8.8 - LED4 - GPIO2.3 */
	*((volatile unsigned long *)0x4804c190) |= 0x00000008;

	/* Shutdown */
	__R31 = 35;	// PRUEVENT_0 on PRU_R31_VEC_VALID
	__halt();

	return 0;
}
