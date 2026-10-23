/*
 * arcade_keyboard.c
 *
 *  Created on: 2026Äê9ÔÂ1ÈÕ
 *      Author: Admin
 */
#include "arcade_keyboard.h"

#include "CH58x_common.h"

#define ARCADE_KEYBOARD_KEYS (GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7)

#if ARCADE_KEYBOARD_SAMPLE_HZ == 0U || ARCADE_KEYBOARD_SAMPLE_HZ > 1000000U
#error "ARCADE_KEYBOARD_SAMPLE_HZ must be between 1 Hz and 1 MHz"
#endif

#define ARCADE_KEYBOARD_SAMPLE_PERIOD_US (1000000U / ARCADE_KEYBOARD_SAMPLE_HZ)
#define ARCADE_KEYBOARD_DEBOUNCE_TICKS \
    ((ARCADE_KEYBOARD_DEBOUNCE_US + ARCADE_KEYBOARD_SAMPLE_PERIOD_US - 1U) / \
     ARCADE_KEYBOARD_SAMPLE_PERIOD_US)

#if ARCADE_KEYBOARD_DEBOUNCE_TICKS > 255U
#error "ARCADE_KEYBOARD_DEBOUNCE_US is too large"
#endif

static const uint16_t keyboard_pins[] = {
    GPIO_Pin_4,
    GPIO_Pin_5,
    GPIO_Pin_6,
    GPIO_Pin_7,
};

static volatile uint16_t stable_state;
static volatile uint16_t candidate_state;
static volatile uint16_t pending_mask;
static volatile uint8_t debounce_ticks[sizeof(keyboard_pins) / sizeof(keyboard_pins[0])];
static volatile uint8_t timer_running;
static uint32_t timer_period;
static arcade_keyboard_callback_t pin_callback;

static inline uint8_t keyboard_pin_index(uint16_t pin) {
    uint8_t index;

    for (index = 0U; index < (sizeof(keyboard_pins) / sizeof(keyboard_pins[0])); ++index) {
        if (keyboard_pins[index] == pin) return index;
    }

    return 0U;
}

static inline void keyboard_set_candidate(uint16_t pin, uint8_t pressed) {
    if (pressed != 0U) {
        candidate_state |= pin;
    } else {
        candidate_state &= (uint16_t)~pin;
    }
}

static inline uint8_t keyboard_pin_pressed(uint16_t port_state, uint16_t pin) {
    return (port_state & pin) == 0U;
}

static inline uint8_t keyboard_pin_report(uint16_t pin) {
    uint8_t index = keyboard_pin_index(pin);
    uint8_t state = (stable_state & pin) != 0U ? 0x1U : 0x3U;

    return (uint8_t)(state << (index * 2U));
}

static void keyboard_start_timer(void) {
    if (timer_running != 0U) return;

    timer_running = 1U;
    TMR0_TimerInit(timer_period);
    TMR0_ClearITFlag(TMR0_3_IT_CYC_END);
    TMR0_ITCfg(ENABLE, TMR0_3_IT_CYC_END);
    PFIC_EnableIRQ(TMR0_IRQn);
}

static void keyboard_stop_timer(void) {
    TMR0_ITCfg(DISABLE, TMR0_3_IT_CYC_END);
    TMR0_Disable();
    TMR0_ClearITFlag(TMR0_3_IT_CYC_END);
    PFIC_DisableIRQ(TMR0_IRQn);
    timer_running = 0U;
}

void arcade_keyboard_init(arcade_keyboard_callback_t callback) {
    uint8_t index;
    uint16_t port_state;

    pin_callback = callback;
    stable_state = 0U;
    candidate_state = 0U;
    pending_mask = 0U;
    timer_running = 0U;
    timer_period = GetSysClock() / ARCADE_KEYBOARD_SAMPLE_HZ;
    if (timer_period == 0U) timer_period = 1U;

    GPIOB_ModeCfg(ARCADE_KEYBOARD_KEYS, GPIO_ModeIN_Floating);
    port_state = GPIOB_ReadPort();
    stable_state = (uint16_t)(~port_state) & ARCADE_KEYBOARD_KEYS;
    candidate_state = stable_state;

    for (index = 0U; index < (sizeof(keyboard_pins) / sizeof(keyboard_pins[0])); ++index) {
        GPIOB_ITModeCfg(
            keyboard_pins[index],
            (stable_state & keyboard_pins[index]) != 0U ? GPIO_ITMode_RiseEdge : GPIO_ITMode_FallEdge);
        debounce_ticks[index] = 0U;
    }

    PFIC_EnableIRQ(GPIO_B_IRQn);
}

__INTERRUPT
__HIGH_CODE
void TMR0_IRQHandler(void) {
    uint8_t index;
    uint8_t data = 0U;
    uint16_t port_state;
    uint16_t pending;

    if (TMR0_GetITFlag(TMR0_3_IT_CYC_END) == 0U) return;

    TMR0_ClearITFlag(TMR0_3_IT_CYC_END);
    port_state = GPIOB_ReadPort();
    pending = pending_mask;

    for (index = 0U; index < (sizeof(keyboard_pins) / sizeof(keyboard_pins[0])); ++index) {
        uint16_t pin = keyboard_pins[index];
        uint8_t pressed;

        if ((pending & pin) == 0U) continue;

        pressed = keyboard_pin_pressed(port_state, pin);
        if (pressed != ((candidate_state & pin) != 0U)) {
            keyboard_set_candidate(pin, pressed);
            debounce_ticks[index] = ARCADE_KEYBOARD_DEBOUNCE_TICKS;
            continue;
        }

        if (debounce_ticks[index] != 0U) --debounce_ticks[index];
        if (debounce_ticks[index] != 0U) continue;

        if ((stable_state & pin) != (candidate_state & pin)) {
            stable_state = (stable_state & (uint16_t)~pin) | (candidate_state & pin);
            data |= keyboard_pin_report(pin);
        }

        pending_mask &= (uint16_t)~pin;
        GPIOB_ITModeCfg(
            pin,
            (candidate_state & pin) != 0U ? GPIO_ITMode_RiseEdge : GPIO_ITMode_FallEdge);
    }

    if (data != 0U && pin_callback != NULL) {
        pin_callback(data);
    }

    if (pending_mask == 0U) keyboard_stop_timer();
}

__INTERRUPT
__HIGH_CODE
void GPIOB_IRQHandler(void) {
    uint8_t index;
    uint16_t port_flag = GPIOB_ReadITFlagPort();
    uint16_t flag = port_flag & ARCADE_KEYBOARD_KEYS;
    uint16_t port_state;

    if (port_flag == 0U) return;

    GPIOB_ClearITFlagBit(port_flag);
    if (flag == 0U) return;

    port_state = GPIOB_ReadPort();

    for (index = 0U; index < (sizeof(keyboard_pins) / sizeof(keyboard_pins[0])); ++index) {
        uint16_t pin = keyboard_pins[index];

        if ((flag & pin) == 0U) continue;

        keyboard_set_candidate(pin, keyboard_pin_pressed(port_state, pin));
        debounce_ticks[index] = ARCADE_KEYBOARD_DEBOUNCE_TICKS;
        pending_mask |= pin;
        R16_PB_INT_EN &= (uint16_t)~pin;
    }

    keyboard_start_timer();
}
