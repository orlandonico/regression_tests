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
 **                                                test_spim_flash_interrupt_ver3
 *=======================================================================================================================**/

/**=======================================================================================================================
 * ?                                             INFORMATION
 * @author         :  Bonfanti Corrado, Orlando Nico
 * @email          :  corrado.bonfanti@unibo.it, nico.orlando@studio.unibo.it, mrorlandonico@gmail.com
 * @repo           :  regression_test/peripheral/spim_flash
 * @createdOn      :  17/09/2021
 * @description    :  What the firmware does is program a flash memory through the SPI communication protocol. 
 *                    There is a first phase of writing and a second phase of reading and checking. 
 *                    The erase part is not applied because by default the memory is already erased, 
 *                    this only applies if you are running the code on the Lagrev platform.
 *                    The erase part, performed on the Lagrev platform, takes a very long time, 
 *                    after 1.5 h it had not finished the execution yet.
 *                    In any case, the part of the code where the command to erase the flash memory 
 *                    is executed is present in the code but has been commented. 
 *                    In this program, an interrupt method is applied, but without the interrupt handler.
 *                    The following code was tested on the Lagrev platform.
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

/*=========================================== Function ===========================================*/

/**=======================================================================================================================
 **                                      wait_spi_rx_event
 *? I have to activate a specific bit of the interrupt controller, and to do it use the line of code: 
 *? rt_irq_mask_set (1 << ARCHI_FC_EVT_TIMER0_HI);
 *? In this case I always have to specify the source.
 *? Then I have the while loop. The while loop uses a "polling" method and not an irq_handler.
 *? What the while loop argument does is take the value of the interrupt controller status register 
 *? and compare it with that of the event produced by SoC.
 *? If the two values are not equal then I enter the loop while and I execute the rt_irq_wait_for_interrupt() function;
 *? What this latter function does is wait for interrupts to be taken.
 *? After applying the three lines of code.
 *? In the end I have to apply the two functions:
 *? rt_irq_clr (1 << ARCHI_FC_EVT_SOC_EVT);
 *? rt_irq_mask_clr (1 << ARCHI_FC_EVT_SOC_EVT);
 *? So that another interrupt of the same type can later be taken.
 *? The correct command is ARCHI_SOC_EVENT_SPIM0_RX and not ARCHI_SOC_EVENT_SPIM_RX (id), this is due to the pulp.h library.
 *? The latter takes data from pulp-runtime-master \ include \ archi \ chips \ pulp \ properties.h and not from
 *? pulp-runtime-master \ include \ archi \ chips \ control-pulp \ properties.h
 *? SPI sends an interrupt when it has finished a read or write.
 *@param null null    
 *@return an interrupt when it has finished a read.
 *=======================================================================================================================**/
void wait_spi_rx_event()
{
  soc_eu_fcEventMask_setEvent(ARCHI_SOC_EVENT_SPIM0_RX);
  rt_irq_mask_set(1 << ARCHI_FC_EVT_SOC_EVT);
  while (!((hal_itc_status_value_get() >> ARCHI_FC_EVT_SOC_EVT) & 1))
  {
    //printf("Hey, I didn't get any interrupts from the SPI, type RX\n");
    rt_irq_wait_for_interrupt();
  }

  //printf("Hey, I just got an interrupt from the SPI communication peripheral, type RX! SPI has finished reading data!\n");

  rt_irq_clr(1 << ARCHI_FC_EVT_SOC_EVT);
  rt_irq_mask_clr(1 << ARCHI_FC_EVT_SOC_EVT);
  soc_eu_fcEventMask_clearEvent(ARCHI_SOC_EVENT_SPIM0_RX);
}

/**=======================================================================================================================
 **                                      wait_spi_tx_event
 *? I have to activate a specific bit of the interrupt controller, and to do it use the line of code: 
 *? rt_irq_mask_set (1 << ARCHI_FC_EVT_TIMER0_HI);
 *? In this case I always have to specify the source.
 *? Then I have the while loop. The while loop uses a "polling" method and not an irq_handler.
 *? What the while loop argument does is take the value of the interrupt controller status register 
 *? and compare it with that of the event produced by SoC.
 *? If the two values are not equal then I enter the loop while and I execute the rt_irq_wait_for_interrupt() function;
 *? What this latter function does is wait for interrupts to be taken.
 *? After applying the three lines of code.
 *? In the end I have to apply the two functions:
 *? rt_irq_clr (1 << ARCHI_FC_EVT_SOC_EVT);
 *? rt_irq_mask_clr (1 << ARCHI_FC_EVT_SOC_EVT);
 *? So that another interrupt of the same type can later be taken.
 *? The correct command is ARCHI_SOC_EVENT_SPIM0_RX and not ARCHI_SOC_EVENT_SPIM_RX (id), this is due to the pulp.h library.
 *? The latter takes data from pulp-runtime-master \ include \ archi \ chips \ pulp \ properties.h and not from
 *? pulp-runtime-master \ include \ archi \ chips \ control-pulp \ properties.h
 *? SPI sends an interrupt when it has finished a read or write.
 *@param null null    
 *@return an interrupt when it has finished a write.
 *=======================================================================================================================**/
void wait_spi_tx_event()
{
  soc_eu_fcEventMask_setEvent(ARCHI_SOC_EVENT_SPIM0_TX);
  rt_irq_mask_set(1 << ARCHI_FC_EVT_SOC_EVT);
  while (!((hal_itc_status_value_get() >> ARCHI_FC_EVT_SOC_EVT) & 1))
  {
    //printf("Hey, I didn't get any interrupts from the SPI, type TX\n");
    rt_irq_wait_for_interrupt();
  }

  //printf("Hey, I just got an interrupt from the SPI communication peripheral, type TX! SPI has finished sending data!\n");

  rt_irq_clr(1 << ARCHI_FC_EVT_SOC_EVT);
  rt_irq_mask_clr(1 << ARCHI_FC_EVT_SOC_EVT);
  soc_eu_fcEventMask_clearEvent(ARCHI_SOC_EVENT_SPIM0_TX);
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

  int tx_buffer_cmd_read[BUFFER_SIZE] = {SPI_CMD_CFG(1, 0, 0),
                                         SPI_CMD_SOT(0),
                                         SPI_CMD_SEND_CMD(0x13, 8, 0),                //--- read command
                                         SPI_CMD_TX_DATA(4, 0, 8, 0, 0),              //--- send the read address
                                         SPI_CMD_RX_DATA(TEST_PAGE_SIZE, 0, 8, 0, 0), //Receives data (max 256Kbits)
                                         SPI_CMD_EOT(0, 0)};

  int tx_buffer_cmd_read_WIP[BUFFER_SIZE] = {SPI_CMD_CFG(1, 0, 0),
                                             SPI_CMD_SOT(0),
                                             SPI_CMD_SEND_CMD(0x05, 8, 0),
                                             SPI_CMD_RX_DATA(1, TEST_PAGE_SIZE, 8, 0, 0),
                                             SPI_CMD_EOT(0, 0)};

  int tx_buffer_cmd_read_ID[BUFFER_SIZE] = {SPI_CMD_CFG(1, 0, 0),
                                            SPI_CMD_SOT(0),
                                            SPI_CMD_SEND_CMD(0x9F, 8, 0), //--- read command
                                            SPI_CMD_RX_DATA(6, 0, 8, 0, 0),
                                            SPI_CMD_EOT(0, 0)};

  int tx_buffer_cmd_erase[BUFFER_SIZE] = {SPI_CMD_CFG(1, 0, 0),
                                          SPI_CMD_SOT(0),
                                          SPI_CMD_SEND_CMD(0x06, 8, 0),
                                          SPI_CMD_EOT(0, 0),
                                          SPI_CMD_SOT(0),
                                          SPI_CMD_SEND_CMD(0x60, 8, 0),
                                          SPI_CMD_EOT(0, 0)};

  int tx_buffer_cmd_erase_sector[BUFFER_SIZE] = {SPI_CMD_CFG(1, 0, 0),
                                                 SPI_CMD_SOT(0),
                                                 SPI_CMD_SEND_CMD(0x06, 8, 0),
                                                 SPI_CMD_EOT(0, 0),
                                                 SPI_CMD_SOT(0),
                                                 SPI_CMD_SEND_CMD(0x21, 8, 0),
                                                 SPI_CMD_TX_DATA(4, 0, 8, 0, 0), //--- write 4B addr to the addr buffer (first 4 bytes of the "page" array)
                                                 SPI_CMD_EOT(0, 0)};

  for (u = 0; u < 1; u++)
  {

    printf("[%d, %d] Start test flash page programming over qspi %d\n", get_cluster_id(), get_core_id(), u);

    /**=======================================================================================================================
     * *                                                     INFO
     *    If I set 1 to the plp_udma_cg_set function I have that the device is not in clock-gates -> I activate the communication device.
     *    If I set 0 to the plp_udma_cg_set function I have that the device is in clock-gates -> I do not activate the communication device.
     *    The plp_udma_cg_get function instead gives me the state of the clock-gate at that moment.
     *    Consequently (plp_udma_cg_get () | (0xffffffff) tells me by doing OR, |, between the two values, since 0xffffffff = 1 in binary,
     *    then the result of the | will always be 1, even if plp_udma_cg_get returns me a 0 or a 1,
     *    in this way I always set 1 to the plp_udma_cg_set function and consequently activate all the peripherals.
     *=======================================================================================================================**/
    plp_udma_cg_set(plp_udma_cg_get() | (0xffffffff));

    /**================================================================================================
     * *                                           INFO
     *    I'm going to modify bit 3 of the Machine Status register,
     *    in particular by setting bit number 3 to 1, I go to activate irq.
     *    I identify the register with 300, I see this from:
     *    https://cv32e40p.readthedocs.io/en/latest/control_status_registers/
     *    CSR Address: 0x300
     *    3 RW Machine Interrupt Enable: If you want to enable interrupt handling in your exception handler,
     *    set the Interrupt Enable MIE to 1 inside your handler code.   
     *================================================================================================**/
    asm volatile("csrw 0x300, %0"
                 :
                 : "r"(2));

    /**================================================================================================
     * *                                           INFO
     *    Going to modify one or more bits, i.e. 11, 7 or 3,
     *    of the Machine Interrupt Enable register, you may have
     *    3 different interrupts. For instance:
     *    11 RW Machine External Interrupt Enable (MEIE): If set, irq_i [11] is enabled.
     *    7 RW Machine Timer Interrupt Enable (MTIE): If set, irq_i [7] is enabled.
     *    3 RW Machine Software Interrupt Enable (MSIE): if set, irq_i [3] is enabled.
     *    In this case I also have to tell him what the source of the interrupt is,
     *    in this case it is the ARCHI_FC_EVT_SOC_EVT. These definitions can be found in:
     *    pulp-runtime-master \ include \ archi \ chips \ control-pulp \ properties.h
     *================================================================================================**/
    asm volatile("csrw 0x304, %0"
                 :
                 : "r"(1 << ARCHI_FC_EVT_SOC_EVT));

    /**================================================================================================
     **                                         CHECK ID
     *================================================================================================**/
    printf("\n...Check ID...\n");
    
    //* Check Id
    for (int i = 0; i < 6; i++)
    {
      rems_resp[i] = 0;
    }

    plp_udma_enqueue(UDMA_SPIM_RX_ADDR(u), (unsigned int)rems_resp, 6 * 4, UDMA_CHANNEL_CFG_EN | UDMA_CHANNEL_CFG_SIZE_32);
    plp_udma_enqueue(UDMA_SPIM_CMD_ADDR(u), (int)tx_buffer_cmd_read_ID, 20, UDMA_CHANNEL_CFG_EN | UDMA_CHANNEL_CFG_SIZE_32);

    //* I am waiting for interrupt from the SPI, which tells me that the command has been received.
    wait_spi_rx_event();
    printf("...I just got the Flash Memory ID data...\n");

    for (int i = 0; i < 6; i++)
    {
      printf("rems_resp[%d] = %8x\n", i, rems_resp[i]);
    }

    //* get the base address of the SPIMx udma channels
    unsigned int udma_spim_channel_base = hal_udma_channel_base(UDMA_CHANNEL_ID(ARCHI_UDMA_SPIM_ID(u)));
    printf("uDMA spim%d base channel address %8x\n", u, udma_spim_channel_base);

    /**================================================================================================
     **                                         Erase the Flash
     *================================================================================================**/
    printf("\n...Erase the Flash Page...\n");
    /**================================================================================================
     * *                                           INFO
     *    To find out why this part of the code is commented, 
     *    read the "description" section in information.   
     *================================================================================================**/

    /**================================================================================================
     * *                                           INFO
     *    If you don't want to delete the entire flash memory but only a sector of it, 
     *    then replace the line of code:
     *    plp_udma_enqueue(UDMA_SPIM_CMD_ADDR(u), (int)tx_buffer_cmd_erase, 28, UDMA_CHANNEL_CFG_EN | UDMA_CHANNEL_CFG_SIZE_32);
     *    with:
     *    plp_udma_enqueue(UDMA_SPIM_TX_ADDR(u), (int)addr_buffer, 4 * 4, UDMA_CHANNEL_CFG_EN | UDMA_CHANNEL_CFG_SIZE_32);
     *    plp_udma_enqueue(UDMA_SPIM_CMD_ADDR(u), (int)tx_buffer_cmd_erase_sector, 32, UDMA_CHANNEL_CFG_EN | UDMA_CHANNEL_CFG_SIZE_32);
     *    
     *    The do-while loop, on the other hand, must not be modified.
     *================================================================================================**/

    //* erase all the flash
    //plp_udma_enqueue(UDMA_SPIM_CMD_ADDR(u), (int)tx_buffer_cmd_erase, 28, UDMA_CHANNEL_CFG_EN | UDMA_CHANNEL_CFG_SIZE_32);

    /**================================================================================================
     * *                                           INFO
     *    With the command plp_udma_busy I can not check the status of the WIP register but only if 
     *    the uDMA is performing some operation or not.
     *    The WIP status register tells me that: if WIP = 0 then I accept commands, 
     *    if instead WIP = 1 then some operation is in progress.
     *    This register is important in the event of an interrupt.
     *    Since the WIP register is internal to the flash memory, with the wait_spi_rx_event()
     *    function I expect to receive interrupts from the SPI.  
     *    This tells me that the WIP registry value has been read and transferred.
     *    When I read that the wip = 0, I am sure that the erase operation on the Flash memory 
     *    is completed and I can move on to the next step.
     *================================================================================================**/
/*
**     do
**     {
**       plp_udma_enqueue(UDMA_SPIM_RX_ADDR(u), (unsigned int)&wip_erase, 1 * 4, UDMA_CHANNEL_CFG_EN | UDMA_CHANNEL_CFG_SIZE_32);
**       plp_udma_enqueue(UDMA_SPIM_CMD_ADDR(u), (int)tx_buffer_cmd_read_WIP, 20, UDMA_CHANNEL_CFG_EN | UDMA_CHANNEL_CFG_SIZE_32);
** 
**       wait_spi_rx_event();
** 
**       wip_erase &= (1);
**       printf("I just got a new WIP register value for Erase, WIP register value: %d\n", wip_erase);
**     } while (wip_erase != 0);
*/
    /**================================================================================================
     **                                         Write the Flash Page
     *================================================================================================**/
    printf("\n...Write the Flash Page...\n");

    //* write the flash page
    plp_udma_enqueue(UDMA_SPIM_TX_ADDR(u), (int)page, TEST_PAGE_SIZE * 4 + 4 * 4, UDMA_CHANNEL_CFG_EN | UDMA_CHANNEL_CFG_SIZE_32);
    plp_udma_enqueue(UDMA_SPIM_CMD_ADDR(u), (int)tx_buffer_cmd_program, 36, UDMA_CHANNEL_CFG_EN | UDMA_CHANNEL_CFG_SIZE_32);

    //* I am waiting for interrupt from the SPI, which tells me that the command has been sent.
    wait_spi_tx_event();
    printf("...I just Sent the Commands to Write the Flash Page...\n");

    /**================================================================================================
     * *                                           INFO
     *    With the command plp_udma_busy I can not check the status of the WIP register but only if 
     *    the uDMA is performing some operation or not.
     *    The WIP status register tells me that: if WIP = 0 then I accept commands, 
     *    if instead WIP = 1 then some operation is in progress.
     *    This register is important in the event of an interrupt.
     *    Since the WIP register is internal to the flash memory, with the wait_spi_rx_event()
     *    function I expect to receive interrupts from the SPI.  
     *    This tells me that the WIP registry value has been read and transferred.
     *    When I read that the wip = 0, I am sure that the write operation on the Flash memory 
     *    is completed and I can move on to the next step.
     *================================================================================================**/
    do
    {
      plp_udma_enqueue(UDMA_SPIM_RX_ADDR(u), (unsigned int)&wip_write, 1 * 4, UDMA_CHANNEL_CFG_EN | UDMA_CHANNEL_CFG_SIZE_32);
      plp_udma_enqueue(UDMA_SPIM_CMD_ADDR(u), (int)tx_buffer_cmd_read_WIP, 20, UDMA_CHANNEL_CFG_EN | UDMA_CHANNEL_CFG_SIZE_32);

      wait_spi_rx_event();

      wip_write &= (1);
      printf("I just got a new WIP register value for Write, WIP register value: %d\n", wip_write);
    } while (wip_write != 0);

    /**================================================================================================
     **                                         Read Data
     *================================================================================================**/
    printf("\n...Read Data...\n");

    //* try to read back data
    plp_udma_enqueue(UDMA_SPIM_RX_ADDR(u), (int)rx_page, TEST_PAGE_SIZE * 4, UDMA_CHANNEL_CFG_EN | UDMA_CHANNEL_CFG_SIZE_32);
    plp_udma_enqueue(UDMA_SPIM_TX_ADDR(u), (int)addr_buffer, 4 * 4, UDMA_CHANNEL_CFG_EN | UDMA_CHANNEL_CFG_SIZE_32);
    plp_udma_enqueue(UDMA_SPIM_CMD_ADDR(u), (int)tx_buffer_cmd_read, 26, UDMA_CHANNEL_CFG_EN | UDMA_CHANNEL_CFG_SIZE_32);

    //* When I get interrupts I am sure that SPI has received the data from the flash page and saved it in the rx_page array.
    wait_spi_rx_event();
    printf("...I have Read the Flash Page values...\n");

    /**================================================================================================
   **                                        Check
   *================================================================================================**/
    printf("\n...Check...\n");
    for (int i = 0; i < TEST_PAGE_SIZE; ++i)
    {
      printf("Position Value: %d -> read %8x, expected %8x \n", i, rx_page[i], page[i + 4]);
      if (rx_page[i] != page[i + 4])
      {
        error++;
      }
    }
  }

  if (error == 0)
  {
    printf("TEST SUCCEDED\n");
  }
  else
  {
    printf("TEST FAILED with %d errors\n", error);
  }

  return error;
}

