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

#ifndef DRIVER_SPI_H_
#define DRIVER_SPI_H_

#ifdef  __cplusplus

extern "C"

{

#endif



//===== include files ========================================================== *
#include <stdint.h>
#include <stdbool.h>
#include "Driver_Common.h"

//===== definitions ============================================================ *
#define DD_SPI_API_VERSION DD_DRIVER_VERSION_MAJOR_MINOR(1,0)  /* API version */


#define DD_SPI_CONTROL_Pos              0
#define DD_SPI_CONTROL_Msk             (0xFFUL << DD_SPI_CONTROL_Pos)

// SPI Control Codes: Mode
#define DD_SPI_MODE_INACTIVE           (0x00UL << DD_SPI_CONTROL_Pos)     ///< SPI Inactive
#define DD_SPI_MODE_MASTER             (0x01UL << DD_SPI_CONTROL_Pos)     ///< SPI Master (Output on MOSI, Input on MISO); arg = Bus Speed in bps

// SPI Control Codes: Frame Format
#define DD_SPI_FRAME_FORMAT_Pos         8
#define DD_SPI_FRAME_FORMAT_Msk        (7UL << DD_SPI_FRAME_FORMAT_Pos)
#define DD_SPI_CPOL0_CPHA0             (0UL << DD_SPI_FRAME_FORMAT_Pos)   ///< Clock Polarity 0, Clock Phase 0 -> mode0 (default)
#define DD_SPI_CPOL0_CPHA1             (1UL << DD_SPI_FRAME_FORMAT_Pos)   ///< Clock Polarity 0, Clock Phase 1 -> mode1
#define DD_SPI_CPOL1_CPHA0             (2UL << DD_SPI_FRAME_FORMAT_Pos)   ///< Clock Polarity 1, Clock Phase 0 -> mode2
#define DD_SPI_CPOL1_CPHA1             (3UL << DD_SPI_FRAME_FORMAT_Pos)   ///< Clock Polarity 1, Clock Phase 1 -> mode3

// SPI Control Codes: Data Bits
#define DD_SPI_DATA_BITS_Pos            12
#define DD_SPI_DATA_BITS_Msk           (0x3FUL << DD_SPI_DATA_BITS_Pos)
#define DD_SPI_DATA_BITS(n)            (((n) & 0x3FUL) << DD_SPI_DATA_BITS_Pos) ///< Number of Data bits

// SPI Control Codes: Bit Order
#define DD_SPI_BIT_ORDER_Pos            18
#define DD_SPI_BIT_ORDER_Msk           (1UL << DD_SPI_BIT_ORDER_Pos)
#define DD_SPI_MSB_LSB                 (0UL << DD_SPI_BIT_ORDER_Pos)      ///< SPI Bit order from MSB to LSB (default)
#define DD_SPI_LSB_MSB                 (1UL << DD_SPI_BIT_ORDER_Pos)      ///< SPI Bit order from LSB to MSB

// SPI Control Codes: Miscellaneous Controls
#define DD_SPI_MISCELLANEOUS_Msk       (0x10UL << DD_SPI_CONTROL_Pos)
#define DD_SPI_SET_BUS_SPEED           (0x10UL << DD_SPI_CONTROL_Pos)     ///< Set Bus Speed in bps; arg = value
#define DD_SPI_GET_BUS_SPEED           (0x11UL << DD_SPI_CONTROL_Pos)     ///< Get Bus Speed in bps
#define DD_SPI_SET_DEFAULT_TX_VALUE    (0x12UL << DD_SPI_CONTROL_Pos)     ///< Set default Transmit value; arg = value
#define DD_SPI_ABORT_TRANSFER          (0x14UL << DD_SPI_CONTROL_Pos)     ///< Abort current data transfer

// SPI specific error codes
#define DD_SPI_ERROR_MODE              (DD_DRIVER_ERROR_SPECIFIC - 1)     ///< Specified Mode not supported
#define DD_SPI_ERROR_FRAME_FORMAT      (DD_DRIVER_ERROR_SPECIFIC - 2)     ///< Specified Frame Format not supported
#define DD_SPI_ERROR_DATA_BITS         (DD_DRIVER_ERROR_SPECIFIC - 3)     ///< Specified number of Data bits not supported
#define DD_SPI_ERROR_BIT_ORDER         (DD_DRIVER_ERROR_SPECIFIC - 4)     ///< Specified Bit order not supported
#define DD_SPI_ERROR_SS_MODE           (DD_DRIVER_ERROR_SPECIFIC - 5)     ///< Specified Slave Select Mode not supported



// SPI transmit options
#define DD_SPI_XFER_PENDING_Pos         0
#define DD_SPI_XFER_PENDING_Msk        (0x01 << DD_SPI_XFER_PENDING_Pos)
#define DD_SPI_XFER_PENDING_ENABLE     (0x01 << DD_SPI_XFER_PENDING_Pos)
#define DD_SPI_XFER_PENDING_DISABLE    (0x00 << DD_SPI_XFER_PENDING_Pos)

#define DD_SPI_XFER_QSPI_Pos           1
#define DD_SPI_XFER_QSPI_Msk           (0x01 << DD_SPI_XFER_QSPI_Pos)
#define DD_SPI_XFER_QSPI_ENABLE        (0x01 << DD_SPI_XFER_QSPI_Pos)
#define DD_SPI_XFER_QSPI_DISABLE       (0x00 << DD_SPI_XFER_QSPI_Pos)




/****** SPI Event *****/
#define DD_SPI_EVENT_TRANSFER_COMPLETE (1UL << 0)  ///< Data Transfer completed

/**
\brief SPI interface numberStatus
*/
typedef enum
{
    SPIM0 = 0,
    SPIM1 = 1,
    SPIM2 = 2,
    SPIM3 = 3,
}DD_SPI_ID;

/**
\brief SPI Driver Capabilities.
*/
typedef struct _DD_SPI_CAPABILITIES
{
    uint32_t simplex          : 1;        ///< supports Simplex Mode (Master and Slave) @deprecated Reserved (must be zero)
    uint32_t ti_ssi           : 1;        ///< supports TI Synchronous Serial Interface
    uint32_t microwire        : 1;        ///< supports Microwire Interface
    uint32_t event_mode_fault : 1;        ///< Signal Mode Fault event: \ref DD_SPI_EVENT_MODE_FAULT
    uint32_t reserved         : 28;       ///< Reserved (must be zero)
} DD_SPI_CAPABILITIES;


/**
\brief SPI Status
*/

typedef struct _DD_SPI_STATUS {
    uint32_t busy       : 1;              ///< Transmitter/Receiver busy flag
    uint32_t free       : 31;
} DD_SPI_STATUS;


typedef void (*DD_SPI_SignalEvent_t) (uint32_t event);  ///< Pointer to \ref DD_SPI_SignalEvent : Signal SPI Event.


// Function documentation
/**
  \fn          int32_t DD_SPI_Initialize (DD_SPI_SignalEvent_t cb_event)
  \brief       Initialize SPI Interface.
  \param[in]   cb_event  Pointer to \ref DD_SPI_SignalEvent
  \return      execution_status

  \fn          int32_t DD_SPI_Uninitialize (void)
  \brief       De-initialize SPI Interface.
  \return      execution_status

  \fn          int32_t DD_SPI_Send (const void *data, uint32_t num)
  \brief       Start sending data to SPI transmitter.
  \param[in]   data  Pointer to buffer with data to send to SPI transmitter
  \param[in]   num   Number of data items to send
  \return      execution_status

  \fn          int32_t DD_SPI_Receive (void *data, uint32_t num)
  \brief       Start receiving data from SPI receiver.
  \param[out]  data  Pointer to buffer for data to receive from SPI receiver
  \param[in]   num   Number of data items to receive
  \return      execution_status

  \fn          int32_t DD_SPI_Transfer (const void *data_out, void *data_in, uint32_t    num)
  \brief       Start sending/receiving data to/from SPI transmitter/receiver.
  \param[in]   data_out  Pointer to buffer with data to send to SPI transmitter
  \param[out]  data_in   Pointer to buffer for data to receive from SPI receiver
  \param[in]   num       Number of data items to transfer
  \return      execution_status

  \fn          uint32_t DD_SPI_GetDataCount (void)
  \brief       Get transferred data count.
  \return      number of data items transferred

  \fn          int32_t DD_SPI_Control (uint32_t control, uint32_t arg)
  \brief       Control SPI Interface.
  \param[in]   control  Operation
  \param[in]   arg      Argument of operation (optional)
  \return      spi_execution_status

  \fn          DD_SPI_STATUS GetStatus (void)
  \brief       Get SPI status.
  \return      SPI status

  \fn          void DD_SPI_SignalEvent (uint32_t event)
  \brief       Signal SPI Events.
  \param[in]   event SPI_events notification mask
  \return      none

*/


typedef struct _DD_DRIVER_SPI {
    const int32_t              (*Initialize)      (DD_SPI_SignalEvent_t cb_event);   ///< Pointer to \ref DD_SPI_Initialize : Initialize SPI Interface.
    const int32_t              (*Uninitialize)    (void);                             ///< Pointer to \ref DD_SPI_Uninitialize : De-initialize SPI Interface.
    const int32_t              (*Send)            (const void *data, uint32_t num, uint8_t cfg );   ///< Pointer to \ref DD_SPI_Send : Start sending data to SPI Interface.
    const int32_t              (*Receive)         (const void *data, uint32_t num, uint8_t cfg );   ///< Pointer to \ref DD_SPI_Receive : Start receiving data from SPI Interface.
    const int32_t              (*Transfer)        (const void *data_out, void *data_in,  uint32_t num); ///< Pointer to \ref DD_SPI_Transfer : Start sending/receiving data to/from SPI.
    const uint32_t             (*GetDataCount)    (void);                             ///< Pointer to \ref DD_SPI_GetDataCount : Get transferred data count.
    const int32_t              (*Control)         (uint32_t control, uint32_t arg);   ///< Pointer to \ref DD_SPI_Control : Control SPI Interface.
    const DD_SPI_STATUS       (*GetStatus)       (void);                             ///< Pointer to \ref DD_SPI_GetStatus : Get SPI status.
    const DD_DRIVER_VERSION   (*GetVersion)      (void);                             ///< Pointer to \ref DD_SPI_GetVersion : Get SPI version.
    const DD_SPI_CAPABILITIES (*GetCapabilities) (void);                             ///< Pointer to \ref DD_SPI_GetCapabilities : Get SPI capabilities.
    const int32_t              (*PowerControl)    (DD_POWER_STATE state);            ///< Pointer to \ref DD_SPI_PowerControl : Control SPI Interface Power.

} DD_DRIVER_SPI;


DD_DRIVER_SPI *DD_SPI_GetDriver( DD_SPI_ID spi_id );



#ifdef  __cplusplus
}
#endif


#endif /* DRIVER_SPI_H_ */
