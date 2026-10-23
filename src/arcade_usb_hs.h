/*
 * arcade_usb_hs.h
 *
 *  Created on: 2026年9月10日
 *      Author: Admin
 */

#ifndef SRC_ARCADE_USB_HS_H_
#define SRC_ARCADE_USB_HS_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum arcade_usb_hs_transfer_mode {
    ARCADE_USB_HS_TRANSFER_DMA = 0,
    ARCADE_USB_HS_TRANSFER_CPY = 1
};

void arcade_usb_hs_init();

void arcade_usb_hs_deinit();

int arcade_usb_hs_input(uint8_t* buf, size_t len);

int arcade_usb_hs_endp_input(uint8_t endp, uint8_t* pbuf, size_t len, enum arcade_usb_hs_transfer_mode mode);

bool arcade_usb_hs_input_busy();

#ifdef __cplusplus
}
#endif


#endif /* SRC_ARCADE_USB_HS_H_ */
