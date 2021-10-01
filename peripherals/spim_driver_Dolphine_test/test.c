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

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "Driver_SPI.h"
#include "Driver_IRQ.h"
#include "pulp.h"
#include <stdlib.h>

#define MAX_BAUDRATE 10000000UL //Hz
#define MIN_BAUDRATE 19532L //Hz
#define SPI_BAUDRATE 1000000UL //Hz

//S25FL256S Instruction Set
#define CMD_WRR     0x01 //Write REG 1
#define CMD_RDID    0x9F //Read ID (JEDEC Manufacturer & JEDEC CFI
#define CMD_RDSR1   0x05 //Read status register-1
#define CMD_WREN    0x06 //Write enable
#define CMD_4P4E    0x21 //4KB Erase
#define CMD_4PP     0x12 //Page program (4 bytes address)
#define CMD_4QPP    0x34 //Page program QSPI (4 bytes address)
#define CMD_4READ   0x13 //Read (4 bytes address)
#define CMD_4QOREAD 0x6C //Read QSPI(4 bytes address)

#define ID_CFI_SIZE 81
#define BUFFER_SIZE 256

const uint8_t expected_id[ID_CFI_SIZE] = { 0x01, 0x02, 0x19, 0x4D, 0x01, 0x80, 0x52, 0x30,
                                0x81, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                0x51, 0x52, 0x59, 0x02, 0x00, 0x40, 0x00, 0x53,
                                0x46, 0x51, 0x00, 0x27, 0x36, 0x00, 0x00, 0x06,
                                0x08, 0x08, 0x10, 0x02, 0x02, 0x03, 0x03, 0x19,
                                0x02, 0x01, 0x08, 0x00, 0x02, 0x1F, 0x00, 0x10,
                                0x00, 0xFD, 0x01, 0x00, 0x01, 0xFF, 0xFF, 0xFF,
                                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                0x50, 0x52, 0x49, 0x31, 0x33, 0x21, 0x02, 0x01,
                                0x00, 0x08, 0x00, 0x01, 0x03, 0x00, 0x00, 0x07,
                                0x01 };

uint8_t TxData[ BUFFER_SIZE ];
uint8_t RxData[ BUFFER_SIZE ];



void WaitFlashReady( DD_DRIVER_SPI* SPIdrv )
{
    uint8_t  cmd = CMD_RDSR1; //command to read status register 1
    uint8_t  status = 0xFF;

    while( status & 0x01 != 0 ) //flash is buzy if status != 0
    {
        while( SPIdrv->GetStatus().busy != 0);
        SPIdrv->Send( &cmd, 1, DD_SPI_XFER_PENDING_ENABLE );
        while( SPIdrv->GetStatus().busy != 0);
        SPIdrv->Receive(&status, 1, 0 );
        printf("WIP Register: %d\n", status);
    }
}


int main (void)
{

    //Get SPI driver instance
    DD_DRIVER_SPI* SPIdrv = DD_SPI_GetDriver( SPIM0 );
    printf("...Get SPI driver instance...\n");
    DD_IRQ_Init();
    printf("...DD_IRQ_Init...\n");
    //1 cmd byte + 4 address bytes + dummy byte(used in qspi mode)
    uint8_t add_buffer[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint8_t  rx_buffer[ID_CFI_SIZE];

    //SPI driver init
    SPIdrv->Initialize(NULL);
    printf("...SPIdrv->Initialize(NULL);...\n");
    SPIdrv->Control( DD_SPI_MODE_MASTER | DD_SPI_CPOL0_CPHA0 | DD_SPI_MSB_LSB | DD_SPI_DATA_BITS(8), SPI_BAUDRATE );
    printf("...SPIdrv->Control...\n");
    ///////////////
    //Read flash ID
    ///////////////
    printf("...Read flash ID...\n");
    //Clear rx buffer
    memset( rx_buffer, 0, ID_CFI_SIZE );
    printf("...memset...\n");
    uint8_t  cmd = CMD_RDID;
    //read CFI
    SPIdrv->Send( &cmd, 1, DD_SPI_XFER_PENDING_ENABLE );
    printf("...SPIdrv->Send...\n");
    while( SPIdrv->GetStatus().busy != 0){
        printf("...SPIdrv->GetStatus().busy != 0...\n");
    }
    SPIdrv->Receive(rx_buffer, ID_CFI_SIZE, 0 );
    printf("...SPIdrv->Receive...\n");
    //waiting for end of receive
    while( SPIdrv->GetStatus().busy != 0){
        printf("...SPIdrv->GetStatus().busy != 0...\n");
    }
    //check received bytes
    if( memcmp( rx_buffer, expected_id, ID_CFI_SIZE ) != 0 )
    {
        printf("SPI_flash test ""read CFI ID"" failed\n");
        return 0;
    }

    //test in single & quad mode
    for(uint8_t quad_mode = 0; quad_mode < 2; quad_mode++ )
    {
        //////////////////////////////
        //Erase the flash (1st sector)
        //////////////////////////////
        //Set write enabled
        int32_t cmd = CMD_WREN;
        SPIdrv->Send( &cmd, 1, 0 );
        while( SPIdrv->GetStatus().busy != 0);

        //erase
        add_buffer[0] = CMD_4P4E;
        while( SPIdrv->GetStatus().busy != 0);
        SPIdrv->Send( add_buffer, sizeof(add_buffer) - 1, 0 );
        //wait until erase operation is in progress
        WaitFlashReady(SPIdrv);
        //check erased bytes

        memset( RxData, 0, BUFFER_SIZE );
        add_buffer[0] = quad_mode == 0? CMD_4READ : CMD_4QOREAD;
        SPIdrv->Send( add_buffer, sizeof(add_buffer) - (quad_mode == 0), DD_SPI_XFER_PENDING_ENABLE );
        while( SPIdrv->GetStatus().busy != 0);
        //read data
        if( quad_mode == 1 )
        {
            SPIdrv->Receive(RxData, BUFFER_SIZE, DD_SPI_XFER_QSPI_ENABLE );
        }
        else
        {
            SPIdrv->Receive(RxData, BUFFER_SIZE, 0 );
        }
        //waiting for end of receive
        while( SPIdrv->GetStatus().busy != 0);
        //check the page was erased
        for (uint16_t nb = 0; nb < BUFFER_SIZE; nb++)
        {
            if( RxData[nb] != 0xFF )
            {
                printf("SPI_flash test ""erase"" failed\n");
                return 0;
            }
        }

        ///////////////////
        //Program the flash
        ///////////////////
        for (uint16_t nb = 0; nb < BUFFER_SIZE; nb++)
        {
            TxData[nb] = nb;
        }
        //write enabled
        cmd = CMD_WREN;
        SPIdrv->Send( &cmd, 1, 0 );

        //send page address
        add_buffer[0] = quad_mode == 0? CMD_4PP : CMD_4QPP;
        while( SPIdrv->GetStatus().busy != 0);
        SPIdrv->Send( add_buffer, sizeof(add_buffer) - 1, DD_SPI_XFER_PENDING_ENABLE );
        //send data
        while( SPIdrv->GetStatus().busy != 0);
        if( quad_mode == 1 )
        {
            SPIdrv->Send( TxData, sizeof(TxData), DD_SPI_XFER_QSPI_ENABLE );
        }
        else
        {
            SPIdrv->Send( TxData, sizeof(TxData), 0 );
        }
        //wait until program operation is in progress
        WaitFlashReady(SPIdrv);

        ///////////////////
        //read the flash
        ///////////////////
        memset( RxData, 0, BUFFER_SIZE );
        //send page address
        add_buffer[0] = quad_mode == 0? CMD_4READ : CMD_4QOREAD;
        SPIdrv->Send( add_buffer, sizeof(add_buffer) - (quad_mode == 0), DD_SPI_XFER_PENDING_ENABLE );
        while( SPIdrv->GetStatus().busy != 0);
        //read data
        if( quad_mode == 1 )
        {
            SPIdrv->Receive(RxData, BUFFER_SIZE, DD_SPI_XFER_QSPI_ENABLE );
        }
        else
        {
            SPIdrv->Receive(RxData, BUFFER_SIZE, 0 );
        }
        //waiting for end of receive
        while( SPIdrv->GetStatus().busy != 0);

        //check received bytes
        for (uint16_t nb = 0; nb < BUFFER_SIZE; nb++)
        {
            if( TxData[nb] != RxData[nb] )
            {
                if( quad_mode == 0 )
                {
                    printf("SPI_flash test ""erase/write/read data"" failed\n");
                }
                else
                {
                    printf("SPI_flash test QSPI ""erase/write/read data"" failed\n");
                }
                return 0;
            }
        }
    }
    printf("SPI_flash test succeed\n");
    return 1;
}



