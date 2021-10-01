/*
 * Copyright (C) 2018 ETH Zurich and University of Bologna
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

/**=======================================================================================================================
 **                                                test_spim_flash_interrupt_handler
 *=======================================================================================================================**/

/**=======================================================================================================================
 * ?                                             INFORMATION
 * @author         :  Bonfanti Corrado, Orlando Nico
 * @email          :  corrado.bonfanti@unibo.it, nico.orlando@studio.unibo.it, mrorlandonico@gmail.com
 * @repo           :  regression_test/peripheral/spim_flash_interrupt_handler
 * @createdOn      :  20/09/2021
 * @description    :  Interrupt Handler Work
 *=======================================================================================================================**/

/*=========================================== Include ===========================================*/
#include <stdio.h>
#include "pulp.h"
#include "flash_page.h"

/*=========================================== Define ===========================================*/
#define OUT 1
#define IN 0
#define BUFFER_SIZE 16
#define TEST_PAGE_SIZE 256

/*=========================================== Global Variable ===========================================*/
int var = 0;

/*=========================================== Function ===========================================*/
void printBits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;

    for (i=size-1;i>=0;i--){
        for (j=7;j>=0;j--){
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
    puts("");
}

static __attribute__((interrupt)) void __irq_handler()
{
  var = var + 1;
}

/*=========================================== Main ===========================================*/
int main()
{
  int error = 0;
  int udma_busy_check_id = 0;
  int udma_busy_erase = 0;
  int wip_erase = 0;
  int udma_busy_write = 0;
  int wip_write = 0;
  int udma_busy_read = 0;
  int u = 0;
  int rx_page[TEST_PAGE_SIZE];
  int addr_buffer[4] = {0x00, 0x00, 0x00, 0x00}; //--- reading address
  volatile int rems_resp[6];

  /**================================================================================================
   * *                                           INFO
   *   Refer to this manual for the commands: https://www.cypress.com/file/216421/download
   *   The commands in this order are related to the flash memory you are using,
   *   they are not standard.
   *   The commands we send on the SPI bus are specific to the memory we attach to the bus
   *   There are two parts for the command buffer for writing to flash memory:
   *   1) I tell the uDMA that I want to use the SPI communication device for the write operation
   *   2) I first tell the uDMA the address of the memory to write to and then the data I want to write
   *   
   *   Beware of the SPI_CMD_TX_DATA_ (words, wordstrans, bitsword, qpi, lsbfirst) command!
   *   words -> number of words to send
   *   wordstrans -> number of words you can send per transfer from L2 memory, for example 1, 2 or 4
   *   bitsword -> bit size of each word sent
   *   So if for example you want to transfer 16 words in 2 transfers, then the system sends first 8 words and then another 8.
   *   Attention! In case of overflow of wordstrans, this is set to 0.
   *   
   *   Beware of the SPI_CMD_SEND_CMD_ (cmd, bits, qpi) command!
   *   cmq -> command I want to send
   *   bits -> indicates the length of the command
   *   qpi -> I send the command using QuadSPI
   *   For the Flash memory in question I have to send appropriate commands, for example:
   *   0x05 -> You access the Program Status Register 1, that is the register where you have the WIP
   *   0x06 -> I tell Register Access that I enable Writing
   *   0x04 -> I tell Register Access that I DO NOT enable Writing
   *   0x12 -> I say how the Flash memory is programmed, ie Page Program
   *   0x13 -> I tell the Read Flash Array that I want to read the values ??from the memory
   *   0x21 -> It is used to delete a certain memory partition 
   *================================================================================================**/

  int tx_buffer_cmd_program[BUFFER_SIZE] = {SPI_CMD_CFG(1, 0, 0),
                                            SPI_CMD_SOT(0),               //Sets the Chip Select (CS)
                                            SPI_CMD_SEND_CMD(0x06, 8, 0), //Transmits up to 16bits of data sent in the command, 0x06 dice per esempio voglio scrivere, 8 indica la lunghezza del comando in bit
                                            SPI_CMD_EOT(0, 0),            //Clears the Chip Select (CS)
                                            SPI_CMD_SOT(0),               //Sets the Chip Select (CS)
                                            SPI_CMD_SEND_CMD(0x12, 8, 0),
                                            SPI_CMD_TX_DATA(4, 0, 8, 0, 0),                           // --- write 4B addr to the addr buffer (first 4 bytes of the "page" array) // Sends data (max 256Kbits) // These are the 4 bytes of address                          //--- write 4B addr to the addr buffer (first 4 bytes of the "page" array)
                                            SPI_CMD_TX_DATA(TEST_PAGE_SIZE, TEST_PAGE_SIZE, 8, 0, 0), //--- write 256B page data to the page buffer
                                            SPI_CMD_EOT(0, 0)};                                       //Clears the Chip Select (CS)

  int tx_buffer_cmd_read_WIP[BUFFER_SIZE] = {SPI_CMD_CFG(1, 0, 0),
                                             SPI_CMD_SOT(0),
                                             SPI_CMD_SEND_CMD(0x05, 8, 0),
                                             SPI_CMD_RX_DATA(1, TEST_PAGE_SIZE, 8, 0, 0),
                                             SPI_CMD_EOT(0, 0)};

  printf("\n...test_spim_flash_interrupt_handler...\n");
  printf("[%d, %d] Start test flash page programming over qspi %d\n", get_cluster_id(), get_core_id(), u);

  /*
  ** If I set 1 to the plp_udma_cg_set function I have that the device is not in clock-gates -> I activate the communication device.
  ** If I set 0 to the plp_udma_cg_set function I have that the device is in clock-gates -> I do not activate the communication device.
  ** The plp_udma_cg_get function instead gives me the state of the clock-gate at that moment.
  ** Consequently (plp_udma_cg_get () | (0xffffffff) tells me by doing OR, |, between the two values, since 0xffffffff = 1 in binary,
  ** then the result of the | will always be 1, even if plp_udma_cg_get returns me a 0 or a 1,
  ** in this way I always set 1 to the plp_udma_cg_set function and consequently activate all the peripherals.
  */
  plp_udma_cg_set(plp_udma_cg_get() | (0xffffffff));

  /*
  ** Write the Flash Page
  */
  printf("\n...Write the Flash Page...\n");
  //* write the flash page
  plp_udma_enqueue(UDMA_SPIM_TX_ADDR(u), (int)page, TEST_PAGE_SIZE * 4 + 4 * 4, UDMA_CHANNEL_CFG_EN | UDMA_CHANNEL_CFG_SIZE_32);
  plp_udma_enqueue(UDMA_SPIM_CMD_ADDR(u), (int)tx_buffer_cmd_program, 36, UDMA_CHANNEL_CFG_EN | UDMA_CHANNEL_CFG_SIZE_32);
  //* I am waiting for interrupt from the SPI, which tells me that the command has been sent.
  printf("...I just Sent the Commands to Write the Flash Page...\n");

  /*
  ** In order to manage the interrupt, initially I have to set bit 3 of register 0x300 to 1.
  ** As a debug function, I am going to read it to see that the set has occurred correctly.
  */
  pos_irq_init();
  int check_300 = 0;
  asm volatile("csrs 0x300, %0" :: "r" (1 << 3));
  __asm__ __volatile__ ("" : : : "memory");
  asm volatile("csrr %0, 0x300" : "=r" (check_300));
  printf("Valore del registro 0x300: 0x%x, in bit: ", check_300);
  printBits(sizeof(check_300), &check_300);

  /*
  ** With the command plp_udma_busy I can not check the status of the WIP register but only if
  ** the uDMA is performing some operation or not.
  ** The WIP status register tells me that: if WIP = 0 then I accept commands,
  ** if instead WIP = 1 then some operation is in progress.
  ** This register is important in the event of an interrupt.
  ** Since the WIP register is internal to the flash memory, with the wait_spi_rx_event()
  ** function I expect to receive interrupts from the SPI.
  ** This tells me that the WIP registry value has been read and transferred.
  ** When I read that the wip = 0, I am sure that the write operation on the Flash memory
  ** is completed and I can move on to the next step.
  */
  do
  {   
    //rt_irq_set_fc_vector_base(pos_irq_vector_base());
    soc_eu_fcEventMask_setEvent(ARCHI_SOC_EVENT_SPIM0_RX);
    //printf("rt_irq_set_fc_vector_base: 0x%x\n", rt_irq_set_fc_vector_base(pos_irq_vector_base()));
    // Interrupt handler

    rt_irq_set_handler(ARCHI_FC_EVT_SOC_EVT, __irq_handler);
    rt_irq_mask_set(1 << ARCHI_FC_EVT_SOC_EVT);
    
    plp_udma_enqueue(UDMA_SPIM_RX_ADDR(u), (unsigned int)&wip_write, 1 * 4, UDMA_CHANNEL_CFG_EN | UDMA_CHANNEL_CFG_SIZE_32);
    plp_udma_enqueue(UDMA_SPIM_CMD_ADDR(u), (int)tx_buffer_cmd_read_WIP, 20, UDMA_CHANNEL_CFG_EN | UDMA_CHANNEL_CFG_SIZE_32);

    /*
    ** When an interrupt occurs, then the interrupt handler is executed, and the variable var is incremented.
    ** Then as long as the var = 0 the interrupt handler was not caught.
    */
    while (var == 0)
    {
      printf("Hey, I didn't get any interrupts from the SPI, type RX\n");
    }

    printf("Hey, I just got an interrupt from the SPI communication peripheral, type RX! SPI has finished reading data!\n");
    wip_write &= (1);
    rt_irq_clr(1 << ARCHI_FC_EVT_SOC_EVT);
    rt_irq_mask_clr(1 << ARCHI_FC_EVT_SOC_EVT);
    soc_eu_fcEventMask_clearEvent(ARCHI_SOC_EVENT_SPIM0_RX);
    printf("I just got a new WIP register value for Write, WIP register value: %d\n", wip_write);
  }while (wip_write != 0);

  printf("Var: %d\n", var);
  return 0;
}

