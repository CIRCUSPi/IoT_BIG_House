#ifndef _CONFIG_H
#define _CONFIG_H

#define HARDWARE_DEBUG false
#define HARDWARE_DEBUG_SENSOR true
#define HARDWARE_DEBUG_BUZZER true
#define HARDWARE_DEBUG_WS2812 true
#define HARDWARE_DEBUG_LCD true
#define HARDWARE_DEBUG_RFID true

#define DEBUG_MODE true
#if DEBUG_MODE
#define debugSerial Serial
#define M_DEBUG_PRINT(x) debugSerial.print(x)
#define M_DEBUG_PRINTLN(x) debugSerial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif
// 7697  /  EZDIO
#define DHT11_PIN 16           // IO2
#define LIGHT_PIN 17           // IO3
#define PIR_PIN 3              // IO9
#define JOYSTICK_X_PIN 14      // IO0
#define JOYSTICK_Y_PIN 15      // IO1
#define JOYSTICK_BTN_PIN 2     // IO8
#define WS2812_PIN 7           // IO15
#define RFID_SS_PIN 10         // IO7
#define FIRE_PIN 5             // IO13
#define BUZZER_PIN 4           // IO16

#define NUMPIXELS 4
#define LCD_I2C_ADDR 0x27

#define TRIG_STR "TRIG"
#define IDLE_STR "IDLE"

#define PAGE_MAX 2
#define ID_NULL "--------"

#define _TIME_KEY_POLLING_uS 50000UL
#define _TIME_KEY_REPEAT_START_uS 1000000UL
#define _TIME_KEY_REPEAT_WORK_uS 300000UL

#define _KEYCODE_NOKEY 0x00
#define _KEYCODE_F_EDGE 0x01
#define _KEYCODE_REPEAT 0x02
#define _KEYCODE_PRESSED 0x03
#define _KEYCODE_R_EDGE 0x04

#define WS2812_WHITE 0xFFFFFF
#define WS2812_RED 0xFF0000
#define WS2812_GREEN 0x00FF00
#define WS2812_BLUE 0x0000FF

typedef enum
{
    AUTO   = 65,     // A
    MANUAL = 77,     // M
} SYS_MODE_E;

typedef enum
{
    JOY_LEFT,
    JOY_DOWN,
    JOY_RIGHT,
    JOY_UP,
    JOY_IDLE,
} JOYSTICK_DIR_E;

struct KEY_POLLING_STRUCT {
    uint32_t dwTimeSlot_Polling;
    uint32_t dwTimeSlot_Repeat;
    uint32_t dwRepeatTime;
    uint32_t dwOrgtTime;
    bool     bPressKey;
    bool     bEnableRepeat;
    uint16_t uAnalogLevel;
    bool     uLess_operator;
    byte     uPin;
    byte     uActiveLevel;
    byte     uRepeatCount;
    byte     uReserved;
};     // 22-byte

#endif /* _CONFIG_H */