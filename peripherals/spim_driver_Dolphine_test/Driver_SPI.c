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
#include <stdlib.h>
#include <stdio.h>
#include "pulp.h"
#include "Driver_Common.h"
#include "Driver_SPI.h"

//===== definitions ============================================================ *

#define DD_SPI_DRV_VERSION DD_DRIVER_VERSION_MAJOR_MINOR(1,0)  // DRV version

static const DD_DRIVER_VERSION DriverVersion = { DD_SPI_API_VERSION, DD_SPI_DRV_VERSION };

// Driver Capabilities
static const DD_SPI_CAPABILITIES DriverCapabilities =
{
    0,  // Simplex Mode (Master and Slave)
    0,  // TI Synchronous Serial Interface
    0,  // Microwire Interface
    0   // Signal Mode Fault event:  DD_SPI_EVENT_MODE_FAULT
};


enum
{
    CMD_CFG_ID = 0,  //Clock configuration index
    CMD_SOT_ID = 1,  //Set nCS (Start Of Transfert) index
    CMD_RUN_ID = 2,  //Run SPI transmission command index
    CMD_EOT_ID = 3,  //Clear nCS (End Of Transfert) index
    NB_SPIM_CMD = 4  //Nb of commands
};

//Driver structure
typedef struct Driver_SPI_t
{
  DD_DRIVER_SPI driver;             // Driver interfaces
  uint8_t bitsword;                  // Nb bits per transmit word 
  uint8_t lsbfirst;                  // transmit mode indicator
  uint8_t sizefactor;                // size factor according to data type
  int32_t bytesword;                 // Nb bytes per transmit word
  int32_t buffer_cmd[ NB_SPIM_CMD ]; // sdma spi command buffer
  int32_t data_left;                 // data left indicator
  bool buzy;                         // buzy indicator
  DD_SPI_SignalEvent_t callback;    // end of transmit callback
} Driver_SPI_t;



static const DD_DRIVER_VERSION DD_SPI_GetVersion( void );
static const DD_SPI_CAPABILITIES DD_SPI_GetCapabilities( void );
static const int32_t   DD_SPI_Initialize(DD_SPI_ID spi_id, DD_SPI_SignalEvent_t cb_event);
static const int32_t   DD_SPI_Uninitialize( DD_SPI_ID spi_id );
static const int32_t   DD_SPI_PowerControl (DD_POWER_STATE state);
static const int32_t   DD_SPI_Send( DD_SPI_ID spi_id, const void *data, uint32_t num, uint8_t cfg );
static const int32_t   DD_SPI_Receive( DD_SPI_ID spi_id, const void *data, uint32_t num, uint8_t cfg );
static const int32_t   DD_SPI_Transfer( DD_SPI_ID spi_id, const void *data_out, void *data_in, uint32_t num );
static const uint32_t  DD_SPI_GetDataCount( DD_SPI_ID spi_id );
static const int32_t   DD_SPI_Control( DD_SPI_ID spi_id, uint32_t control, uint32_t arg);
static const DD_SPI_STATUS   DD_SPI_GetStatus( DD_SPI_ID spi_id );




//===== global variables ======================================================== *
static Driver_SPI_t SPI_Driver_List[ ARCHI_UDMA_NB_SPIM ];

//wrapper to ensure compliance with CMSIS interfaces
static int32_t   DD_SPI_Initialize0 ( DD_SPI_SignalEvent_t cb_event ) { return DD_SPI_Initialize ( SPIM0, cb_event ); }
/*
** static int32_t   DD_SPI_Initialize1 ( DD_SPI_SignalEvent_t cb_event ) { return DD_SPI_Initialize ( SPIM1, cb_event ); }
** static int32_t   DD_SPI_Initialize2 ( DD_SPI_SignalEvent_t cb_event ) { return DD_SPI_Initialize ( SPIM2, cb_event ); }
** static int32_t   DD_SPI_Initialize3 ( DD_SPI_SignalEvent_t cb_event ) { return DD_SPI_Initialize ( SPIM3, cb_event ); }
** const int32_t (*DD_SPI_InitializeList[ARCHI_UDMA_NB_SPIM])(DD_SPI_SignalEvent_t cb_event) = { DD_SPI_Initialize0, DD_SPI_Initialize1, DD_SPI_Initialize2, DD_SPI_Initialize3 };
*/
const int32_t (*DD_SPI_InitializeList[ARCHI_UDMA_NB_SPIM])(DD_SPI_SignalEvent_t cb_event) = { DD_SPI_Initialize0};

static int32_t   DD_SPI_Uninitialize0 ( void ) { return DD_SPI_Uninitialize ( SPIM0 ); }
/*
** static int32_t   DD_SPI_Uninitialize1 ( void ) { return DD_SPI_Uninitialize ( SPIM1 ); }
** static int32_t   DD_SPI_Uninitialize2 ( void ) { return DD_SPI_Uninitialize ( SPIM2 ); }
** static int32_t   DD_SPI_Uninitialize3 ( void ) { return DD_SPI_Uninitialize ( SPIM3 ); }
** const int32_t (*DD_SPI_UninitializeList[ARCHI_UDMA_NB_SPIM])( void ) = { DD_SPI_Uninitialize0, DD_SPI_Uninitialize1, DD_SPI_Uninitialize2, DD_SPI_Uninitialize3 };
*/
const int32_t (*DD_SPI_UninitializeList[ARCHI_UDMA_NB_SPIM])( void ) = { DD_SPI_Uninitialize0};

static int32_t   DD_SPI_Control0 ( uint32_t control, uint32_t arg ) { return DD_SPI_Control ( SPIM0, control, arg );}
/*
** static int32_t   DD_SPI_Control1 ( uint32_t control, uint32_t arg ) { return DD_SPI_Control ( SPIM1, control, arg );}
** static int32_t   DD_SPI_Control2 ( uint32_t control, uint32_t arg ) { return DD_SPI_Control ( SPIM2, control, arg );}
** static int32_t   DD_SPI_Control3 ( uint32_t control, uint32_t arg ) { return DD_SPI_Control ( SPIM3, control, arg );}
** const int32_t (*DD_SPI_ControlList[ARCHI_UDMA_NB_SPIM])(uint32_t control, uint32_t arg) = { DD_SPI_Control0, DD_SPI_Control1, DD_SPI_Control2, DD_SPI_Control3 };
*/
const int32_t (*DD_SPI_ControlList[ARCHI_UDMA_NB_SPIM])(uint32_t control, uint32_t arg) = { DD_SPI_Control0};

static int32_t   DD_SPI_Send0 ( const void *data, uint32_t num, uint8_t cfg ) { return DD_SPI_Send ( SPIM0, data, num, cfg );}
/*
** static int32_t   DD_SPI_Send1 ( const void *data, uint32_t num, uint8_t cfg ) { return DD_SPI_Send ( SPIM1, data, num, cfg );}
** static int32_t   DD_SPI_Send2 ( const void *data, uint32_t num, uint8_t cfg ) { return DD_SPI_Send ( SPIM2, data, num, cfg );}
** static int32_t   DD_SPI_Send3 ( const void *data, uint32_t num, uint8_t cfg ) { return DD_SPI_Send ( SPIM3, data, num, cfg );}
** const int32_t (*DD_SPI_SendList[ARCHI_UDMA_NB_SPIM])(const void *data, uint32_t num, uint8_t cfg ) = { DD_SPI_Send0, DD_SPI_Send1, DD_SPI_Send2, DD_SPI_Send3 }; 
*/
const int32_t (*DD_SPI_SendList[ARCHI_UDMA_NB_SPIM])(const void *data, uint32_t num, uint8_t cfg ) = { DD_SPI_Send0};

static int32_t   DD_SPI_Receive0 ( const void *data, uint32_t num, uint8_t cfg ) { return DD_SPI_Receive ( SPIM0, data, num, cfg );}
/*
** static int32_t   DD_SPI_Receive1 ( const void *data, uint32_t num, uint8_t cfg ) { return DD_SPI_Receive ( SPIM1, data, num, cfg );}
** static int32_t   DD_SPI_Receive2 ( const void *data, uint32_t num, uint8_t cfg ) { return DD_SPI_Receive ( SPIM2, data, num, cfg );}
** static int32_t   DD_SPI_Receive3 ( const void *data, uint32_t num, uint8_t cfg ) { return DD_SPI_Receive ( SPIM3, data, num, cfg );}
** const int32_t (*DD_SPI_ReceiveList[ARCHI_UDMA_NB_SPIM])(const void *data, uint32_t num, uint8_t cfg) = { DD_SPI_Receive0, DD_SPI_Receive1, DD_SPI_Receive2, DD_SPI_Receive3 }; 
*/
const int32_t (*DD_SPI_ReceiveList[ARCHI_UDMA_NB_SPIM])(const void *data, uint32_t num, uint8_t cfg) = { DD_SPI_Receive0};

static int32_t   DD_SPI_Transfer0 (const void *data_out, void *data_in, uint32_t num) { return DD_SPI_Transfer ( SPIM0, data_out, data_in, num );}
/*
** static int32_t   DD_SPI_Transfer1 (const void *data_out, void *data_in, uint32_t num) { return DD_SPI_Transfer ( SPIM1, data_out, data_in, num );}
** static int32_t   DD_SPI_Transfer2 (const void *data_out, void *data_in, uint32_t num) { return DD_SPI_Transfer ( SPIM2, data_out, data_in, num );}
** static int32_t   DD_SPI_Transfer3 (const void *data_out, void *data_in, uint32_t num) { return DD_SPI_Transfer ( SPIM3, data_out, data_in, num );}
** const int32_t (*DD_SPI_TransferList[ARCHI_UDMA_NB_SPIM])(const void *data_out, void *data_in, uint32_t num) = { DD_SPI_Transfer0, DD_SPI_Transfer1, DD_SPI_Transfer2, DD_SPI_Transfer3 };
*/
const int32_t (*DD_SPI_TransferList[ARCHI_UDMA_NB_SPIM])(const void *data_out, void *data_in, uint32_t num) = { DD_SPI_Transfer0};


static DD_SPI_STATUS   DD_SPI_GetStatus0 ( void ) { return DD_SPI_GetStatus ( SPIM0 );}
/*
** static DD_SPI_STATUS   DD_SPI_GetStatus1 ( void ) { return DD_SPI_GetStatus ( SPIM1 );}
** static DD_SPI_STATUS   DD_SPI_GetStatus2 ( void ) { return DD_SPI_GetStatus ( SPIM2 );}
** static DD_SPI_STATUS   DD_SPI_GetStatus3 ( void ) { return DD_SPI_GetStatus ( SPIM3 );}
** const DD_SPI_STATUS (*DD_SPI_GetStatusList[ARCHI_UDMA_NB_SPIM])( void ) = { DD_SPI_GetStatus0, DD_SPI_GetStatus1, DD_SPI_GetStatus2, DD_SPI_GetStatus3 };
*/
const DD_SPI_STATUS (*DD_SPI_GetStatusList[ARCHI_UDMA_NB_SPIM])( void ) = { DD_SPI_GetStatus0};

static uint32_t   DD_SPI_GetDataCount0 ( void ) { return DD_SPI_GetDataCount ( SPIM0 );}
/*
** static uint32_t   DD_SPI_GetDataCount1 ( void ) { return DD_SPI_GetDataCount ( SPIM1 );}
** static uint32_t   DD_SPI_GetDataCount2 ( void ) { return DD_SPI_GetDataCount ( SPIM2 );}
** static uint32_t   DD_SPI_GetDataCount3 ( void ) { return DD_SPI_GetDataCount ( SPIM3 );}
** const uint32_t (*DD_SPI_GetDataCountList[ARCHI_UDMA_NB_SPIM])( void ) = { DD_SPI_GetDataCount0, DD_SPI_GetDataCount1, DD_SPI_GetDataCount2, DD_SPI_GetDataCount3 }; 
*/
const uint32_t (*DD_SPI_GetDataCountList[ARCHI_UDMA_NB_SPIM])( void ) = { DD_SPI_GetDataCount0}; 
//===== static functions ======================================================== *


// \fn          void   DD_SPI_GetVersion( void )
// \brief       returns version information of the driver implementation in DD_DRIVER_VERSION.
// \return      DD_DRIVER_VERSION
static const DD_DRIVER_VERSION DD_SPI_GetVersion( void )
{
    return DriverVersion;
}


//  \fn          DD_SPI_CAPABILITIES DD_SPI_GetCapabilities (void)
//  \brief       Get driver capabilities.
//  \return      DD_SPI_CAPABILITIES
static const DD_SPI_CAPABILITIES DD_SPI_GetCapabilities( void )
{
    return DriverCapabilities;
}


//  \fn          int32_t DD_SPI_PowerControl( DD_POWER_STATE state )
//  \brief       Control SPI Interface Power.
//  \param[in]   state: Power state
//  \return      execution_status
static const int32_t DD_SPI_PowerControl( DD_POWER_STATE state )
{
    return DD_DRIVER_ERROR_UNSUPPORTED;
}

//  \fn          void   DD_SPI_StopTransfer(DD_SPI_ID spi_id)
//  \brief       Abort current transfer.
//  \param[in]   spi_id: SPI id
static volatile void   DD_SPI_AbortTransfer(DD_SPI_ID spi_id)
{
    Driver_SPI_t *pdriver = &SPI_Driver_List[spi_id];

    //  clear and stop all udma transfer
    pulp_write32( UDMA_SPIM_RX_ADDR( spi_id ) + UDMA_CHANNEL_CFG_OFFSET, UDMA_CHANNEL_CFG_CLEAR );
    pulp_write32( UDMA_SPIM_TX_ADDR( spi_id ) + UDMA_CHANNEL_CFG_OFFSET, UDMA_CHANNEL_CFG_CLEAR );
    pulp_write32( UDMA_SPIM_CMD_ADDR( spi_id ) + UDMA_CHANNEL_CFG_OFFSET, UDMA_CHANNEL_CFG_CLEAR );
    pdriver->buzy = false;
}


// \fn          int32_t   DD_SPI_Initialize(uint8_t spi_id, DD_SPI_SignalEvent_t cb_event)
// \brief       Initialize SPI Interface.
// \param[in]   spi_id: Number of SPI interface
// \param[in]   cb_event:  Pointer to \ref DD_SPI_SignalEvent
// \return      Status Error Codes
static volatile const int32_t   DD_SPI_Initialize(DD_SPI_ID spi_id, DD_SPI_SignalEvent_t cb_event)
{
    int periph_id = ARCHI_UDMA_SPIM_ID(spi_id);
    Driver_SPI_t *pdriver = &SPI_Driver_List[spi_id];

    //set clock enable
    plp_udma_cg_set(plp_udma_cg_get() | (1<<periph_id));
    //register callback
    pdriver->callback = NULL;
    pdriver->buzy = false;
    if( cb_event != NULL )
    {
        pdriver->callback = cb_event;
    }
    //enable event generation
    soc_eu_fcEventMask_setEvent( UDMA_EVENT_ID( periph_id ) + ARCHI_UDMA_SPIM_EOT_EVT );
    pdriver->data_left = 0;
    return DD_DRIVER_OK;
}



// \fn          int32_t  DD_SPI_Uninitialize( DD_SPI_ID spi_id )
// \brief       De-initialize SPI Interface.
// \param[in]   spi_id: Number of SPI interface
// \return      Status Error Codes
static volatile const int32_t   DD_SPI_Uninitialize( DD_SPI_ID spi_id )
{
    Driver_SPI_t *pdriver = &SPI_Driver_List[spi_id];

    DD_SPI_AbortTransfer( spi_id );
    pdriver->callback = NULL;
    pdriver->data_left = 0;
    //disable spi peripheral clock
    plp_udma_cg_set( plp_udma_cg_get() & ~( 1<<ARCHI_UDMA_SPIM_ID( spi_id ) ) );
    //disable event generation
    soc_eu_fcEventMask_clearEvent( UDMA_EVENT_ID( ARCHI_UDMA_SPIM_ID(spi_id) ) + ARCHI_UDMA_SPIM_EOT_EVT );
    return DD_DRIVER_OK;
}


// \fn          int32_t    DD_SPI_Control( DD_SPI_ID spi_id, uint32_t control, uint32_t arg)
// \brief       Spi driver setting
// \param[in]   spi_id: Number of SPI interface
// \param[in]   control  Operation
// \param[in]   arg      Argument of operation (optional)
// \return      Status Error Codes
static volatile const int32_t   DD_SPI_Control( DD_SPI_ID spi_id, uint32_t control, uint32_t arg)
{
    int32_t error = DD_DRIVER_OK;
    uint32_t freq_periph;
    uint8_t clockdiv;
    Driver_SPI_t *pdriver = &SPI_Driver_List[spi_id];

    // if Miscellaneous Controls
    if( control & DD_SPI_MISCELLANEOUS_Msk )
    {
        switch( control )
        {
        case DD_SPI_SET_BUS_SPEED:
            freq_periph = pi_freq_get( PI_FREQ_DOMAIN_PERIPH );
            //get baudrate in arg
            if( ( arg <= freq_periph ) && ( arg > ( freq_periph >> 9 ) ) )
            {
                // compute clock divider according to the baudrate
                clockdiv = freq_periph / ( (arg ) << 1 );
                pdriver->buffer_cmd[CMD_CFG_ID] = ( pdriver->buffer_cmd[CMD_CFG_ID] & 0xFFFFFF00 ) | clockdiv;
            }
            else
            {
                error = DD_SPI_ERROR_MODE;
            }
            break;
        case DD_SPI_GET_BUS_SPEED:
            clockdiv = pdriver->buffer_cmd[CMD_CFG_ID] & 0xFF;
            error = pi_freq_get( PI_FREQ_DOMAIN_PERIPH ) / clockdiv;
            break;
        case DD_SPI_ABORT_TRANSFER:
            DD_SPI_AbortTransfer( spi_id );
            break;
        default:
            error = DD_DRIVER_ERROR_PARAMETER;
        }
    }
    else
    {
        //mode checking
        if( ( control & DD_SPI_CONTROL_Msk ) == DD_SPI_MODE_MASTER )
        {
            freq_periph = pi_freq_get( PI_FREQ_DOMAIN_PERIPH );
            //get baudrate in arg
            if( ( arg <= freq_periph ) && ( arg > ( freq_periph >> 9 ) ) )
            {
                // compute clock divider according to the baudrate
                clockdiv = freq_periph / ( (arg ) << 1 );
                //enable spi peripheral clock
                plp_udma_cg_set( plp_udma_cg_get() | ( 1<<ARCHI_UDMA_SPIM_ID( spi_id ) ) );
            }
            else
            {
                error = DD_SPI_ERROR_MODE;
            }
        }
        else // DD_SPI_MODE_INACTIVE
        {
            DD_SPI_AbortTransfer( spi_id );
        }
        //set clock divisor, polarity & phase in command buffer
        pdriver->buffer_cmd[CMD_CFG_ID] = ( SPI_CMD_CFG_ID << SPI_CMD_ID_OFFSET ) | ( control & DD_SPI_FRAME_FORMAT_Msk ) | clockdiv;

        //data bits
        pdriver->bitsword = ( control & DD_SPI_DATA_BITS_Msk ) >> DD_SPI_DATA_BITS_Pos;
        if( (pdriver->bitsword < 1) || (pdriver->bitsword > 32 ) )
        {
            pdriver->bitsword = 8;
            error = DD_SPI_ERROR_DATA_BITS;
        }
        //data order
        pdriver->lsbfirst = ( control & DD_SPI_BIT_ORDER_Msk ) >> DD_SPI_BIT_ORDER_Pos;
    }
    if( pdriver->bitsword <= 8 )
    {
        pdriver->bytesword = UDMA_CHANNEL_CFG_SIZE_8;
        pdriver->sizefactor = 0;
    }
    else if ( pdriver->bitsword <= 16 )
    {
        pdriver->bytesword = UDMA_CHANNEL_CFG_SIZE_16;
        pdriver->sizefactor = 1;
    }
    else
    {
        pdriver->bytesword = UDMA_CHANNEL_CFG_SIZE_32;
        pdriver->sizefactor = 2;
    }
    pdriver->buffer_cmd[CMD_SOT_ID] = SPI_CMD_SOT( 0 );
    //Enable event generation
    pdriver->buffer_cmd[CMD_EOT_ID] = SPI_CMD_EOT( SPI_CMD_EOT_EVENT_ENA, 0 );
    return error;
}

// \fn          int32_t    DD_SPI_Send( DD_SPI_ID spi_id, const void *data, uint32_t num)
// \brief       Start sending data to SPI Interface.
// \param[in]   spi_id: Number of SPI interface
// \param[in]   data:  Pointer to buffer with data to send to SPI transmitter
// \param[in]   num: Number of data items to send
// \param[in]   config: keep CS and qspi option
// \return      Status Error Codes
static volatile const int32_t    DD_SPI_Send( DD_SPI_ID spi_id, const void *data, uint32_t num, uint8_t config )
{
    int32_t error = DD_DRIVER_ERROR_BUSY;
    Driver_SPI_t *pdriver = &SPI_Driver_List[spi_id];
    uint8_t qspi = (( config & DD_SPI_XFER_QSPI_Msk ) != 0 );
    uint8_t pending = (( config & DD_SPI_XFER_PENDING_Msk ) != 0 );
    if(num != 0)
    {
        if( pdriver->buzy == false )
        {
            pdriver->buzy = true;
            if(num == 1)
            {
                pdriver->buffer_cmd[CMD_RUN_ID] =  SPI_CMD_SEND_CMD( ((uint8_t *)data)[0],pdriver->bitsword, qspi );
            }
            else
            {
                pdriver->buffer_cmd[CMD_RUN_ID] = SPI_CMD_TX_DATA( num, 0, pdriver->bitsword, qspi, pdriver->lsbfirst );
                pdriver->data_left = num;
                plp_udma_enqueue( UDMA_SPIM_TX_ADDR( spi_id ), (int)data, num << pdriver->sizefactor, pdriver->bytesword);
            }
            //keep CS if needed
            pdriver->buffer_cmd[CMD_EOT_ID] = SPI_CMD_EOT( SPI_CMD_EOT_EVENT_ENA, pending );
            plp_udma_enqueue( UDMA_SPIM_CMD_ADDR( spi_id ), (int)pdriver->buffer_cmd , sizeof(pdriver->buffer_cmd), UDMA_CHANNEL_CFG_SIZE_32);
            error = DD_DRIVER_OK;
        }
    }
   
    printf("......Send Error: %d......\n", error);
    return error;
}


// \fn          int32_t    DD_SPI_Receive( DD_SPI_ID spi_id, const void *data, uint32_t num)
// \brief       Start receiving data from SPI receiver.
// \param[in]   spi_id: Number of SPI interface
// \param[in]   data:Pointer to buffer for data to receive from SPI receiver
// \param[in]   num: Number of data items to receive
// \param[in]   config: keep CS and qspi option
// \return      Status Error Codes
static volatile const int32_t    DD_SPI_Receive( DD_SPI_ID spi_id, const void *data, uint32_t num, uint8_t config )
{
    int32_t error = DD_DRIVER_ERROR_BUSY;
    Driver_SPI_t *pdriver = &SPI_Driver_List[spi_id];
    printf("......&SPI_Driver_List[spi_id]......\n");
    uint8_t qspi = (( config & DD_SPI_XFER_QSPI_Msk ) != 0 );
    uint8_t pending = (( config & DD_SPI_XFER_PENDING_Msk ) != 0 );
   

    if( pdriver->buzy == false )
    {
        pdriver->buzy = true;
        pdriver->buffer_cmd[CMD_EOT_ID] = SPI_CMD_EOT( SPI_CMD_EOT_EVENT_ENA, pending );
        pdriver->buffer_cmd[CMD_RUN_ID] = SPI_CMD_RX_DATA( num, 0, pdriver->bitsword, qspi, pdriver->lsbfirst );
        pdriver->data_left = num;
        plp_udma_enqueue( UDMA_SPIM_RX_ADDR( spi_id ), (int)data, num << pdriver->sizefactor, pdriver->bytesword);
        plp_udma_enqueue( UDMA_SPIM_CMD_ADDR( spi_id ), (int)&pdriver->buffer_cmd , sizeof(pdriver->buffer_cmd), UDMA_CHANNEL_CFG_SIZE_32);
        error = DD_DRIVER_OK;
    }
   
    printf("......Receive Error: %d......\n", error);
    return error;
}

// \fn          int32_t DD_SPI_Transfer(DD_SPI_ID spi_id, const void *data_out, void *data_in, uint32_t num)
// \brief       Start sending/receiving data to/from SPI transmitter/receiver.
// \param[in]   spi_id: Number of SPI interface
// \param[in]   data_out:  Pointer to buffer with data to send to SPI transmitter
// \param[out]  data_in:   Pointer to buffer for data to receive from SPI receiver
// \param[in]   num:  Number of data items to transfer
// \return      \ref execution_status
static volatile const int32_t    DD_SPI_Transfer( DD_SPI_ID spi_id, const void *data_out, void *data_in, uint32_t num )
{
    int32_t error = DD_DRIVER_ERROR_BUSY;
    Driver_SPI_t *pdriver = &SPI_Driver_List[spi_id];

    if( pdriver->buzy == false )
    {
        pdriver->buzy = true;
        pdriver->buffer_cmd[CMD_EOT_ID] = SPI_CMD_EOT( SPI_CMD_EOT_EVENT_ENA, 0 );
        pdriver->buffer_cmd[CMD_RUN_ID] = SPI_CMD_FUL( num, 0, pdriver->bitsword, pdriver->lsbfirst );
        pdriver->data_left = (num << 1); //num * 2: Rx + Tx
        plp_udma_enqueue( UDMA_SPIM_RX_ADDR( spi_id ), (int)data_in, num << pdriver->sizefactor, pdriver->bytesword);
        plp_udma_enqueue( UDMA_SPIM_TX_ADDR( spi_id ), (int)data_out, num << pdriver->sizefactor, pdriver->bytesword);
        plp_udma_enqueue( UDMA_SPIM_CMD_ADDR( spi_id ), (int)&pdriver->buffer_cmd , sizeof(pdriver->buffer_cmd), UDMA_CHANNEL_CFG_SIZE_32);
        error = DD_DRIVER_OK;
    }
    return error;
}

// \fn          DD_SPI_STATUS DD_SPI_GetStatus( DD_SPI_ID spi_id )
// \brief       Get SPI status
// \param[in]   spi_id: Number of SPI interface
// \return      current SPI interface status
static volatile const DD_SPI_STATUS   DD_SPI_GetStatus( DD_SPI_ID spi_id )
{
    DD_SPI_STATUS status = {0};
    Driver_SPI_t *pdriver = &SPI_Driver_List[spi_id];

    status.busy = pdriver->buzy;
    return status;
}


// \fn          uint32_t DD_SPI_GetDataCount( DD_SPI_ID spi_id )
// \brief       returns the number of currently transferred data items during
//              DD_SPI_Send, DD_SPI_Receive and DD_SPI_Transfer operation.
// \param[in]   spi_id: Number of SPI interface
// \return      number of data items transferred
static volatile const uint32_t    DD_SPI_GetDataCount( DD_SPI_ID spi_id )
{
    Driver_SPI_t *pdriver = &SPI_Driver_List[spi_id];
    int32_t tx_data_left = pulp_read32( UDMA_SPIM_TX_ADDR( spi_id ) + UDMA_CHANNEL_SIZE_OFFSET ) >> pdriver->sizefactor;
    int32_t rx_data_left = pulp_read32( UDMA_SPIM_RX_ADDR( spi_id ) + UDMA_CHANNEL_SIZE_OFFSET ) >> pdriver->sizefactor;
    return ( pdriver->data_left - ( tx_data_left + rx_data_left ) );
}


//===== global functions ======================================================== *


// \fn          DD_DRIVER_SPI *DD_SPI_GetDriver( DD_SPI_ID SpiId )
// \brief       Return the pointer on spi driver
// \param[in]   spi_id: Number of SPI interface
// \return      \ref execution_status
DD_DRIVER_SPI *DD_SPI_GetDriver( DD_SPI_ID SpiId )
{
    DD_DRIVER_SPI *pSpiDriver = NULL;

    if( SpiId < ARCHI_UDMA_NB_SPIM )
    {
        pSpiDriver = &(SPI_Driver_List[ SpiId ].driver);
        //API initialization
        pSpiDriver->Initialize = DD_SPI_InitializeList[ SpiId ];
        pSpiDriver->Uninitialize = DD_SPI_UninitializeList[ SpiId ];
        pSpiDriver->Send = DD_SPI_SendList[ SpiId ];
        pSpiDriver->Receive = DD_SPI_ReceiveList[ SpiId ];
        pSpiDriver->Transfer= DD_SPI_TransferList[ SpiId ];
        pSpiDriver->GetDataCount = DD_SPI_GetDataCountList[ SpiId ];
        pSpiDriver->Control = DD_SPI_ControlList[ SpiId ];
        pSpiDriver->GetStatus= DD_SPI_GetStatusList[ SpiId ];
        pSpiDriver->GetVersion=DD_SPI_GetVersion;
        pSpiDriver->GetCapabilities=DD_SPI_GetCapabilities;
        pSpiDriver->PowerControl=DD_SPI_PowerControl;
    }
    return( pSpiDriver );
}

// \fn          void DD_SPI_IrqCallback( uint32_t evt )
// \brief       Generic callback to call in event interrupt handler c
// \param[in]   evt: event
__attribute__((noinline)) void DD_SPI_IrqCallback( uint32_t evt )
{
    uint8_t spi_id = ( (evt - ARCHI_UDMA_SPIM_EOT_EVT) / UDMA_NB_PERIPH_EVENTS ) - 2;
    Driver_SPI_t *pdriver = &SPI_Driver_List[spi_id];
    if ( pdriver->callback != NULL )
    {
        //vado a impostare un nuovo valore del callback
        pdriver->callback( DD_SPI_EVENT_TRANSFER_COMPLETE );
    }
    pdriver->buzy = false;
}




