#include <stdint.h>
#include <stdbool.h>
#include "linux_types.h"

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

#define DPRAM_SHARED 0x00010000

/* Note: PRU number should be defined prior to pru specific headers */
#define PRU1
#include "pru_defs.h"
#include "pru_pwm.h"
#include "pru_hal.h"

#define MAX_CHS 8

volatile uint32_t       pwm_period[MAX_CHS];
volatile uint32_t       pwm_pulse_width[MAX_CHS];
volatile uint32_t       pwm_pulse_width_sorted[MAX_CHS];
volatile uint8_t        pwm_enabled[MAX_CHS];

//      pwm_period[i] = 0x003d0900;     /* 50 Hz */
//      pwm_period[i] = 0x0032dcd5;     /* 60 Hz */
//      pwm_period[i] = 0x00186a00;     /* 125 Hz */
//      pwm_period[i] = 0x000c3500;     /* 250 Hz */
static void pwm_setup(void)
{
	uint8_t i;

	for (i=0; i<MAX_CHS; i++)
	{
		pwm_period[i] = 0xc3500;     /* 250 Hz */
	}

        /* This is the unsorted version of the PWM data
         * Times are given in IEP counts (200 cnts/us)
         * eg. 1ms = 1000us = 200,000 IEP cnts
         */
	pwm_pulse_width[0] = 200000;       /* 1000 us */
	pwm_pulse_width[1] = 200000;       /* 1500 us */
	pwm_pulse_width[2] = 200000;       /* 2000 us */
	pwm_pulse_width[3] = 200000;       /* 2000 us */
	pwm_pulse_width[4] = 200000;       /* 2000 us */
	pwm_pulse_width[5] = 200000;       /* 2000 us */
	pwm_pulse_width[6] = 200000;       /* 2000 us */
	pwm_pulse_width[7] = 200000;       /* 2000 us */

	/* Enable/Disable all channels */
	for (i=0; i<MAX_CHS; i++)
	{
		pwm_enabled[i] = 1;
	}

	/* Setup DPSHRAM - each PWM period is defined, however all must be the same for now */
	for (i=0; i<MAX_CHS; i++)
	{
		PWM_INFO->channel[i].time_t = 0x3d0900;
	}

	/* Again, the unsorted version of the PWM data */
	PWM_INFO->channel[0].time_high = 200000;
	PWM_INFO->channel[1].time_high = 200000;
	PWM_INFO->channel[2].time_high = 200000;
	PWM_INFO->channel[3].time_high = 200000;
	PWM_INFO->channel[4].time_high = 200000;
	PWM_INFO->channel[5].time_high = 200000;
	PWM_INFO->channel[6].time_high = 200000;
	PWM_INFO->channel[7].time_high = 200000;

	/* OFF mask TBD, all 8 PWM on for now */
	PWM_INFO->channelenable = 0x000000ff;
}

static void bubble_sort_with_index (uint8_t sorted_indexes[])
{
	uint8_t     i, j;
	uint32_t    temp_val;
	uint8_t     temp_index;

	/* Populate the sorted array with the unsorted data */
	for (i=0; i<MAX_CHS; i++)
	{
		pwm_pulse_width_sorted[i] = pwm_pulse_width[i];
		sorted_indexes[i] = i;
/*		Offset for SVO1-8 PRU bit offset of 4  */
/*		sorted_indexes[i] = i + 4; */
	}

	for (i=0; i<MAX_CHS; i++)
	{
		for (j=i; j<MAX_CHS; j++)
		{
			/* Sort all values from smallest to largest */
			if (pwm_pulse_width_sorted[j] < pwm_pulse_width_sorted[i])
			{
				temp_val = pwm_pulse_width_sorted[i];
				temp_index = sorted_indexes[i];
				pwm_pulse_width_sorted[i] = pwm_pulse_width_sorted[j];
				pwm_pulse_width_sorted[j] = temp_val;
				sorted_indexes[i] = sorted_indexes[j];
				sorted_indexes[j] = temp_index;
			}
		}
	}

#if 0
	/* Debug - place values in DPRAM */
	for (i=0; i<MAX_CHS; i++)
	{
//		PWM_CMD->periodhi[8+i][0] = 0;
		PWM_INFO->channel[8+i].time_high = 0;
//		PWM_CMD->periodhi[8+i][1] = sorted_indexes[i];
		PWM_INFO->channel[8+i].time_t = sorted_indexes[i];
	}
#endif
}

static inline u32 read_PIEP_COUNT(void)
{
	return PIEP_COUNT;
}

int main(int argc, char *argv[])
{
	uint8_t			i;
	volatile uint32_t	cnt_now;
	volatile uint32_t	cnt_next;
	volatile uint32_t	lLedCount = 0;
	volatile bool		bLedState = false;
	volatile bool		bDone = false;

	uint8_t			sorted_indexes[MAX_CHS];

	uint32_t		reg_bit_mask;
	uint32_t		next_on_cnt[MAX_CHS];
	uint32_t		next_off_cnt[MAX_CHS];
	uint32_t		cnt_ready;
	uint32_t		staging_cnt;

	uint32_t		last_read_enmask;       /* enable mask */
        uint32_t		latest_enmask;
 

	/* enable OCP master port */
	PRUCFG_SYSCFG &= ~SYSCFG_STANDBY_INIT;
	PRUCFG_SYSCFG = (PRUCFG_SYSCFG &
					~(SYSCFG_IDLE_MODE_M | SYSCFG_STANDBY_MODE_M)) |
					SYSCFG_IDLE_MODE_NO | SYSCFG_STANDBY_MODE_NO;

	/* our PRU wins arbitration */
	PRUCFG_SPP |=  SPP_PRU1_PAD_HP_EN;

	/* configure timer */
	PIEP_GLOBAL_CFG = GLOBAL_CFG_DEFAULT_INC(1) | GLOBAL_CFG_CMP_INC(1);
	PIEP_CMP_STATUS = CMD_STATUS_CMP_HIT(1); /* clear the interrupt */
	PIEP_CMP_CMP1   = 0x0;
	PIEP_CMP_CFG |= CMP_CFG_CMP_EN(1);
	PIEP_GLOBAL_CFG |= GLOBAL_CFG_CNT_ENABLE;

	/* Setup some default values */
	pwm_setup();

	/* Sort pulse width data in asending order */
	bubble_sort_with_index (sorted_indexes);

	/* Initialize count */
	cnt_now = read_PIEP_COUNT();

	/* Setup turn ON counts */
	next_on_cnt[0] = cnt_now + 200;
	for (i=1; i<MAX_CHS; i++)
	{
		next_on_cnt[i] = next_on_cnt[i-1] + 200;
	}

	/* Setup turn OFF counts */
	for (i=0; i<MAX_CHS; i++)
	{
		next_off_cnt[i] = next_on_cnt[i] + pwm_pulse_width_sorted[i];
	}


//	cnt_next = cnt_now + 800000;
	last_read_enmask = PWM_INFO->channelenable;


//	*((volatile unsigned long *)0x44E07194) = 0x08000000;  /* BAT-LEV-1 */
//	*((volatile unsigned long *)0x44E07194) = 0x00000800;  /* BAT-LEV-2 */

	while (!bDone)
	{
		/* Wait for turn ON counts */
		for (i=0; i<MAX_CHS; i++)
		{
			if (pwm_enabled[sorted_indexes[i]])
			{
				cnt_ready = next_on_cnt[i];
				/* Offset of 4 for SVO1-8 PRU bit offset */
				reg_bit_mask = (1U << (sorted_indexes[i]+4));
				while ((read_PIEP_COUNT() - cnt_ready) & 0x80000000);
				__R30 |= reg_bit_mask;
			}
		}

		/* Wait for turn OFF counts */
		for (i=0; i<MAX_CHS; i++)
		{
			if (pwm_enabled[sorted_indexes[i]])
			{
				cnt_ready = next_off_cnt[i];
				/* Offset of 4 for SVO1-8 PRU bit offset */
				reg_bit_mask = ~(1U << (sorted_indexes[i])+4);
				while ((read_PIEP_COUNT() - cnt_ready) & 0x80000000);
				__R30 &= reg_bit_mask;
			}
		}

                /* Get potentially new pulse width data and sort it */
#define ABC_1 2
#ifdef ABC_1
		for (i=0; i<MAX_CHS; i++)
		{
			pwm_pulse_width[i] = PWM_INFO->channel[i].time_high;
		}

                bubble_sort_with_index (sorted_indexes);
#endif

		/* Advance channel timers to period - staggered ON times by 1 us */
		next_on_cnt[0] += pwm_period[0];
		for (i=1; i<MAX_CHS; i++)
		{
			next_on_cnt[i] = next_on_cnt[i-1] + 200;
		}
		/* Next turn OFF */
		for (i=0; i<MAX_CHS; i++)
		{
			next_off_cnt[i] = next_on_cnt[i] + pwm_pulse_width_sorted[i];
		}

#if 0
		for (i=0; i<MAX_CHS; i++)
			PWM_CMD->hilo_read[i][1] = pwm_pulse_width[i];
#endif

		latest_enmask = PWM_INFO->channelenable;
//		latest_enmask = 0xff;

		if (last_read_enmask != latest_enmask)
		{
			/* Enable/Disable */
			for (i=0; i<MAX_CHS; i++)
			{
				if (latest_enmask & (1U << i))
					pwm_enabled[i] = 1;
				else
				{
					/* Turn off any disabled channel */
					__R30 &= ~(1U << i);
					pwm_enabled[i] = 0;
				}
			}
			last_read_enmask = latest_enmask;
		}

#if 0
		/* Spend some time doing busy work till just before next PWM cycle */
		while ((read_PIEP_COUNT() - cnt_next) & 0x80000000)
		{
			for (i=0; i<10; i++);
		};
		/* Set up for next period */
		cnt_next += 800000;
#endif

		/* Spend some time doing busy work till just before next PW cycle */
		staging_cnt = next_on_cnt[0] - 1000;
		while ((read_PIEP_COUNT() - staging_cnt) & 0x80000000)
		{
			for (i=0; i<10; i++);
		};



		// Exit if we receive a Host->PRU1 interrupt
		if (__R31 & 0x80000000)
		{
			bDone = true;
		}

//		if (led_flash_cnt < ++lLedCount)
		if (50 < ++lLedCount)
		{
			if (bLedState)
			{
				/* Clear LED */
				*((volatile unsigned long *)0x44E07190) = 0x00000800;  /* BAT-LEV-2 */
				bLedState = false;
			}
			else
			{
				/* Set LED */
				*((volatile unsigned long *)0x44E07194) = 0x00000800;  /* BAT-LEV-2 */
				bLedState = true;
			}
			lLedCount = 0;
		}
	}

	__R31 = 35;	// PRUEVENT_0 on PRU_R31_VEC_VALID
	__halt();

	return 0;
}

