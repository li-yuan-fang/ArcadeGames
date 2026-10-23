/*
 * arcade_usb_mux.h
 *
 *  Created on: 2026年9月10日
 *      Author: Admin
 */

#ifndef SRC_ARCADE_USB_MUX_H_
#define SRC_ARCADE_USB_MUX_H_

#if ARCADE_USB_HS_MODE
#include "arcade_usb_hs.h"

#define ARCADE_USB_INIT arcade_usb_hs_init()
#define ARCADE_USB_IN_BUSY arcade_usb_hs_input_busy
#define ARCADE_USB_IN arcade_usb_hs_input
#else // ARCADE_USB_HS_MODE
#include "arcade_usb.h"

#define ARCADE_USB_INIT arcade_usb_init()
#define ARCADE_USB_IN_BUSY arcade_usb_input_busy
#define ARCADE_USB_IN arcade_usb_input
#endif // ARCADE_USB_HS_MODE

#endif /* SRC_ARCADE_USB_MUX_H_ */
