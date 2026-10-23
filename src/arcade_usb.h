/*
 * arcade_usb.h
 *
 *  Created on: 2026年8月31日
 *      Author: Admin
 */

#ifndef SRC_ARCADE_USB_H_
#define SRC_ARCADE_USB_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void arcade_usb_init();

int arcade_usb_input(uint8_t* buf, size_t len);

bool arcade_usb_input_busy();

#ifdef __cplusplus
}
#endif

#endif /* SRC_ARCADE_USB_H_ */
