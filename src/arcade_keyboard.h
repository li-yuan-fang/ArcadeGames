/*
 * arcade_keyboard.h
 *
 *  Created on: 2026年9月1日
 *      Author: Admin
 */

#ifndef SRC_ARCADE_KEYBOARD_H_
#define SRC_ARCADE_KEYBOARD_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*arcade_keyboard_callback_t)(uint8_t data);

#ifndef ARCADE_KEYBOARD_SAMPLE_HZ
#define ARCADE_KEYBOARD_SAMPLE_HZ 8000U
#endif

#ifndef ARCADE_KEYBOARD_DEBOUNCE_US
#define ARCADE_KEYBOARD_DEBOUNCE_US 1000U
#endif

void arcade_keyboard_init(arcade_keyboard_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif /* SRC_ARCADE_KEYBOARD_H_ */
