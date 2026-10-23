/*
 * Main.c
 *
 *  Created on: 2026年8月31日
 *      Author: Admin
 */

#include <stdlib.h>
#include <stdint.h>

#include "CH58x_common.h"
#include "arcade_keyboard.h"
#include "arcade_queue.h"
#include "arcade_usb_mux.h"

static arcade_queue_t input_queue;

static inline void DebugInit(void) {
    GPIOA_ModeCfg(GPIO_Pin_4, GPIO_ModeIN_PU);
    GPIOA_ModeCfg(GPIO_Pin_5, GPIO_ModeOut_PP_5mA);
    UART3_DefInit();
}

#if ARCADE_NATIVE_GAMEPAD
    static uint8_t gamepad_buttons = 0x0;
#endif // ARCADE_NATIVE_GAMEPAD

static void keyboard_callback(uint8_t data) {
    // 按下0x1 抬起0x3
    // Low Bit表示触发事件
    // High Bit表示按下/抬起

#if ARCADE_NATIVE_GAMEPAD
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t btn = data >> (i << 1);
        if ((btn & 0x1) == 0U) continue;

        uint8_t btn_mask = 1 << i;
        if ((btn & 0x2) == 0U) {
            // 按下
            gamepad_buttons |= btn_mask;
        } else {
            // 抬起
            gamepad_buttons &= ~btn_mask;
        }
    }

    int ret = arcade_queue_enqueue(&input_queue, &gamepad_buttons);
    if (ret != 0) {
        printf("Key - QUEUE FAIL: %d\n", ret);
    }
#else // ARCADE_NATIVE_GAMEPAD
    uint8_t* buf = (uint8_t*) malloc(1);
    if (buf == NULL) {
        printf("Key - NOMEM\n");
        return;
    }

    *buf = data;

    int ret = arcade_queue_enqueue(&input_queue, buf);
    if (ret != 0) {
        printf("Key - QUEUE FAIL: %d\n", ret);
    }
#endif // ARCADE_NATIVE_GAMEPAD
}

static void usb_input_loop() {
    if (ARCADE_USB_IN_BUSY()) return;

    uint8_t* data = arcade_queue_dequeue(&input_queue);
    if (data == NULL) return;

    int ret = ARCADE_USB_IN(data, 1U);
    if (ret != 0) {
        printf("USB - INPUT FAIL: %d\n", ret);
    }

#if !ARCADE_NATIVE_GAMEPAD
    free(data);
#endif // ARCADE_NATIVE_GAMEPAD
}

int main() {
    // uint8_t data = 0x00;

    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(SYSCLK_FREQ);

    DebugInit();
    printf("start\n");

    // LED
    GPIOA_ModeCfg(GPIO_Pin_1 | GPIO_Pin_2, GPIO_ModeOut_PP_5mA);
    GPIOA_SetBits(GPIO_Pin_1 | GPIO_Pin_2);

    arcade_queue_init(&input_queue);
    arcade_keyboard_init(keyboard_callback);

    ARCADE_USB_INIT;

    while(1) {
        usb_input_loop();

        // mDelaymS(1000U);
        // if (arcade_usb_hs_input_busy()) continue;

        // int ret = arcade_usb_hs_input(&data, 1U);
        // if (ret != 0) {
        //     printf("USB - INPUT FAIL: %d\n", ret);
        //     continue;
        // }

        // data++;
    }
}
