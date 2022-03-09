#ifndef _CONFIG_H
#define _CONFIG_H

#define VERSION 3.0

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

#define AUTO_MODE_PAGE_MAX 2
#define MANUAL_MODE_PAGE_MAX 1
#define SET_MODE_PAGE_MAX 1
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
#define WS2812_BLACK 0x0

#define BUZZER_Do 523
#define BUZZER_Re 587
#define BUZZER_Mi 659
#define BUZZER_Fa 698
#define BUZZER_So 784
#define BUZZER_La 880
#define BUZZER_Si 988

/* EEPROM config Start Address */
#define EEPROM_ADDR 0x0

typedef enum
{
    AUTO_MODE,
    MANUAL_MODE,
    SET_MODE,
    MAX_MODE,
} SYS_MODE_E;

typedef enum
{
    WS2812_LEFT,
    WS2812_DOWN,
    WS2812_RIGHT,
    WS2812_UP,
    WS2812_ALL,
    WS2812_CLOSE,
} WS2812_MODE_E;

struct KEY_POLLING_STRUCT {
    uint32_t dwTimeSlot_Polling;
    uint32_t dwTimeSlot_Repeat;
    uint32_t dwRepeatTime;
    uint32_t dwOrgtTime;
    bool     bPressKey;
    bool     bEnableRepeat;
    byte     uActiveLevel;
    byte     uRepeatCount;
    byte     uReserved;
    bool     bFlag;
};     // 22-byte

typedef struct {
    char    mode_tag;
    uint8_t page_max;
} sys_modes_t;

typedef struct {
    char wifi_ssid[30];
    char wifi_pass[30];
    char rfid_uid[8];
    char mqtt_user[30];
    char mqtt_pass[60];
    char device_id[30];
} config_E;     // 188-byte

#endif /* _CONFIG_H */