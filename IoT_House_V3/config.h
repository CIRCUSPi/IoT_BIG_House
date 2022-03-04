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

#endif /* _CONFIG_H */