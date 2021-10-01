/*
 * Copyright (C) 2020 ETH Zurich and University of Bologna
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Authors: Emmanuel Botte, Dolphin Design 
 */

//===== include files ========================================================== *
#include <stdint.h>
#include "pulp.h"
#include "Driver_SPI.h"

#pragma GCC push_options

extern void DD_SPI_IrqCallback( uint32_t evt );

typedef void (*callback_t)( uint32_t evt );

//lw   a0, evt      \n\t\


__attribute__((interrupt)) void irq_handler()
{
    callback_t cb = NULL;
    uint32_t evt = *(volatile unsigned int *)(long)(ARCHI_FC_ITC_ADDR + ITC_FIFO_OFFSET);

  switch( evt )
  {
  case ARCHI_SOC_EVENT_SPIM0_EOT:
    cb = DD_SPI_IrqCallback;
    break;
  default:
    break;
  }
  if( cb != NULL )
  {
    //save context
    asm volatile(
       "add  sp, sp, -128 \n\t\
        sw   ra, 0x00(sp) \n\t\
        sw   gp, 0x04(sp) \n\t\
        sw   tp, 0x08(sp) \n\t\
        sw   t0, 0x0C(sp) \n\t\
        sw   t1, 0x10(sp) \n\t\
        sw   t2, 0x14(sp) \n\t\
        sw   a3, 0x24(sp) \n\t\
        sw   a4, 0x28(sp) \n\t\
        sw   a5, 0x2C(sp) \n\t\
        sw   a6, 0x30(sp) \n\t\
        sw   a7, 0x34(sp) \n\t\
        sw   t3, 0x38(sp) \n\t\
        sw   t4, 0x3C(sp) \n\t\
        sw   t5, 0x40(sp) \n\t\
        sw   t6, 0x4C(sp) \n\t");
    //pass argument in a0
    asm volatile("addi a0, %[evt], 0    \n\t" : : [evt] "r"(evt) );
    //call the callback
    asm volatile("jalr ra, %[cb]    \n\t" : : [cb] "r"(cb) );
    //load saved context
    asm volatile(
       "lw   ra, 0x00(sp) \n\t\
        lw   gp, 0x04(sp) \n\t\
        lw   tp, 0x08(sp) \n\t\
        lw   t0, 0x0C(sp) \n\t\
        lw   t1, 0x10(sp) \n\t\
        lw   t2, 0x14(sp) \n\t\
        lw   a3, 0x24(sp) \n\t\
        lw   a4, 0x28(sp) \n\t\
        lw   a5, 0x2C(sp) \n\t\
        lw   a6, 0x30(sp) \n\t\
        lw   a7, 0x34(sp) \n\t\
        lw   t3, 0x38(sp) \n\t\
        lw   t4, 0x3C(sp) \n\t\
        lw   t5, 0x40(sp) \n\t\
        lw   t6, 0x4C(sp) \n\t\
        add  sp, sp, 128  \n\t\
        ");
  }
}
#pragma GCC pop_options



void DD_IRQ_Init( void )
{
    hal_irq_disable();
    pos_irq_init();
    rt_irq_set_handler( ARCHI_FC_EVT_SOC_EVT, irq_handler );
    rt_irq_mask_set( 1<<ARCHI_FC_EVT_SOC_EVT );
    hal_irq_enable();
}


