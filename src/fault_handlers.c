/*
 * fault_handlers.c
 *
 * ch32fun's vector table aliases every handler it does not know to
 * DefaultIRQHandler(), which spins forever.  That is fine for the ones the
 * application never enables, but a hard fault on the application side should
 * recover: reset, and let the bootloader decide whether to run the image again
 * or stay in DFU.  This is the behaviour the project always had (the WCH
 * startup file's HardFault_Handler called NVIC_SystemReset()).
 */
#include "ch32fun.h"

void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void)
{
    NVIC_SystemReset();

    while (1) {
    }
}
