/******************************************************************************/
/*  AM3359_PRU.cmd                                                            */
/*    Description: This file is a sample linker command file that can be      */
/*    used for linking programs built with the PRU C compiler and running the */
/*    resulting .out file on an AM3359 PRU0. Use it as a guideline.  You will */
/*    want to change the memory layout to match your specific target system.  */
/*    You may want to change the allocation scheme according to the size of   */
/*    your program.                                                           */
/******************************************************************************/

-cr
-stack 0x200
-heap 0x200

MEMORY
{
    PAGE 0:
      PRU_PMEM:   o = 0x00000000  l = 0x00001D00  /* 8kB PRU0 Instruction RAM */

    PAGE 1:
      PRU_DMEM  : org = 0x00000000  len = 0x00000400  /* 8kB PRU Data RAM 0 */
      C0        : org = 0x00020000  len = 0x00000300 CREGISTER=0
      C4        : org = 0x00026000  len = 0x00000100 CREGISTER=4
      C26       : org = 0x0002E000  len = 0x00000100 CREGISTER=26
}

SECTIONS
{
    .text          >  PRU_PMEM, PAGE 0
    .stack         >  PRU_DMEM, PAGE 1
    .bss           >  PRU_DMEM, PAGE 1
    .cio           >  PRU_DMEM, PAGE 1
    .const         >  PRU_DMEM, PAGE 1
    .rodata        >  PRU_DMEM, PAGE 1
    .data          >  PRU_DMEM, PAGE 1
    .switch        >  PRU_DMEM, PAGE 1
    .sysmem        >  PRU_DMEM, PAGE 1
    .cinit         >  PRU_DMEM, PAGE 1
}
