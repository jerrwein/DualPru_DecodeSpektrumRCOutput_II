# DualPru_DecodeSpektrumRCOutput_II
RCInput-SpectrumDecoder-RCOutput-ServoOutputs

# Info:
  PRU-1 is intended to drive the servo pins to provide signals to the ESCs.
  The BBBL servo pins are arranged accordingly:

  ---------------------------------------------------
  |                                                  |
  |                                                  |
  |                                                  |
  |  - + s  (SVO1 - Chan #7)  R30.8                  |
  |  - + s  (SVO2 - Chan #4)  R30.10                 |
  |  - + s  (SVO3 - Chan #1)  R30.9                  |
  |  - + s  (SVO4 - Chan #x)  R30.11                 |
  |  - + s  (SVO5 - Chan #5)  R30.6                  |
  |  - + s  (SVO6 - Chan #6)  R30.7                  |
  |  - + s  (SVO7 - Chan #2)  R30.4                  |
  |  - + s  (SVO8 - Chan #3)  R30.5                  |
  |                                                  |
   --------------------------------------------------

SVO7, T1, "pr1_pru1_pru_r30[4]",  "SERVO", "pru_r30[4]",  GPIO2.10, P8.27
SVO8, T2, "pr1_pru1_pru_r30[5]",  "SERVO", "pru_r30[5]",  GPIO2.11, P8.28
SVO5, T3, "pr1_pru1_pru_r30[6]",  "SERVO", "pru_r30[6]",  GPIO2.12, P8.29
SVO6, T4, "pr1_pru1_pru_r30[7]",  "SERVO", "pru_r30[7]",  GPIO2.13, P8.30
SVO1, U5, "pr1_pru1_pru_r30[8]",  "SERVO", "pru_r30[8]",  GPIO2.22, P8.39
SVO3, R5, "pr1_pru1_pru_r30[9]",  "SERVO", "pru_r30[9]",  GPIO2.23, P8.40
SVO2, V5, "pr1_pru1_pru_r30[10]", "SERVO", "pru_r30[10]", GPIO2.24, P8.41
SVO4, R6, "pr1_pru1_pru_r30[11]", "SERVO", "pru_r30[11]", GPIO2.25, P8.42

# Prerequisites:
  Two BBBL applications are needed to test PWM servo outputs.
    a.) ./decodeRC_outputPWM, which loads code into each PRU & begins them running.
        PRU-0 decodes a (simulated) DSMx data stream and injects it in a circular FIFO
              via SHARED-MEM.
        PRU-1 process the PWM channel data and drives the servo pins at a configurable
              PWMfrequency and duty cycle.
    b.) ./RCInput_To_RCOutput_Test.
        The module syncs to the incoming DSMx stream and processes it accordingly,
        presenting it via SHARED-MEM to PRU-1 for output.

  A third QT application 'SBUS_Simulation' is used to generate the DSMx serial stream
  via a serial UART.
 
 ## Update:
    The latest ardupilot librairies have changed the structures for passing PRU-1 data.
    Use the '/DualPru_DecodeSpektrumRCOutput_II' project if building for this version.
    
 # Beaglebone Blue GPIO pins:

#define GPIO0_27_PIN 0x08000000         /* BAT-LEV-1  */
#define GPIO0_11_PIN 0x00000800         /* BAT-LEV-2  */
#define GPIO1_29_PIN 0x20000000         /* BAT-LEV-3  */
#define GPIO0_26_PIN 0x04000000         /* BAT-LEV-4  */

/* Red User LED */
gpio_num = 66;
/* Green User LED */
 gpio_num = 67;

/* Battery Level #1 LED (Red) */
gpio_num = 27;
/* Battery Level #2 LED (Green) */
gpio_num = 11;
/* Battery Level #3 LED (Green) */
gpio_num = 61;
/* Battery Level #4 LED (Green) */
gpio_num = 26;

/* User-0 */
gpio_num = 53;
/* User-1 */
gpio_num = 54;
/* User-2 */
gpio_num = 55;
/* User-3*/
gpio_num = 56;
