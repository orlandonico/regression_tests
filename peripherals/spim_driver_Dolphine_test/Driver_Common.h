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

#ifndef DRIVER_COMMON_H_
#define DRIVER_COMMON_H_


/* General return codes */
#define DD_DRIVER_OK                 0 ///< Operation succeeded
#define DD_DRIVER_ERROR             -1 ///< Unspecified error
#define DD_DRIVER_ERROR_BUSY        -2 ///< Driver is busy
#define DD_DRIVER_ERROR_TIMEOUT     -3 ///< Timeout occurred
#define DD_DRIVER_ERROR_UNSUPPORTED -4 ///< Operation not supported
#define DD_DRIVER_ERROR_PARAMETER   -5 ///< Parameter error
#define DD_DRIVER_ERROR_SPECIFIC    -6 ///< Start of driver specific error


/**
\brief Driver Version
*/
typedef struct _DD_DRIVER_VERSION {
  uint16_t api;                         ///< API version
  uint16_t drv;                         ///< Driver version
} DD_DRIVER_VERSION;

/**
\brief General power states
*/
typedef enum _DD_POWER_STATE
{
  DD_POWER_OFF,      ///< Power off: no operation possible
  DD_POWER_LOW,      ///< Low Power mode: retain state, detect and signal wake-up events
  DD_POWER_FULL      ///< Power on: full operation at maximum performance
} DD_POWER_STATE;


#define DD_DRIVER_VERSION_MAJOR_MINOR(major, minor)   (((major) << 8) | (minor))

#endif /* DRIVER_COMMON_H_ */

