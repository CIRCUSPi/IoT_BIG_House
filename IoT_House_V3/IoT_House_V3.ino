#include "config.h"
#include <Adafruit_NeoPixel.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Wire.h>

/* #region  Object */
DHT               obj_dht11(DHT11_PIN, DHT11);
Adafruit_NeoPixel obj_ws2812(NUMPIXELS, WS2812_PIN, NEO_GRB + NEO_KHZ800);
MFRC522           obj_rc522(RFID_SS_PIN, 255);
LiquidCrystal_I2C obj_lcd(LCD_I2C_ADDR);
/* #endregion */

/* #region  Sensor value */
float    m_temperature  = 0.0f;
float    m_humidity     = 0.0f;
uint16_t m_light_raw    = 0;
bool     m_pir          = 0;
uint16_t m_joystick_x   = 0;
uint16_t m_joystick_y   = 0;
bool     m_joystick_btn = 0;
bool     m_fire         = 0;
String   card_uid       = ID_NULL;
/* #endregion */

uint8_t cur_page     = 1;
uint8_t mode         = AUTO;
uint8_t joystick_dir = JOY_IDLE;

struct KEY_POLLING_STRUCT m_KeyPolling_JoyX_L_Tag;
struct KEY_POLLING_STRUCT m_KeyPolling_JoyX_R_Tag;
struct KEY_POLLING_STRUCT m_KeyPolling_JoyY_U_Tag;
struct KEY_POLLING_STRUCT m_KeyPolling_JoyY_D_Tag;
struct KEY_POLLING_STRUCT m_KeyPolling_JoyBtn_Tag;
struct KEY_POLLING_STRUCT m_KeyPolling_Pir_Tag;
struct KEY_POLLING_STRUCT m_KeyPolling_Fire_Tag;

byte m_uKeyCode_JoyX_L;
byte m_uKeyCode_JoyX_R;
byte m_uKeyCode_JoyY_U;
byte m_uKeyCode_JoyY_D;
byte m_uKeyCode_JoyBtn;
byte m_uKeyCode_Pir;
byte m_uKeyCode_Fire;

void setup()
{
#if DEBUG_MODE
    debugSerial.begin(115200);
#endif
    SPI.begin();
    pinMode(LIGHT_PIN, INPUT);
    pinMode(PIR_PIN, INPUT);
    pinMode(JOYSTICK_X_PIN, INPUT);
    pinMode(JOYSTICK_Y_PIN, INPUT);
    pinMode(JOYSTICK_BTN_PIN, INPUT_PULLUP);
    pinMode(FIRE_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    obj_dht11.begin();
    obj_ws2812.begin();
    obj_lcd.begin(20, 4);
    obj_rc522.PCD_Init();
    obj_ws2812.setBrightness(30);
    obj_ws2812.clear();
    obj_ws2812.show();

    Init_Polling_Button(0, &m_KeyPolling_JoyX_L_Tag);
    Init_Polling_Button(0, &m_KeyPolling_JoyX_R_Tag);
    Init_Polling_Button(0, &m_KeyPolling_JoyY_U_Tag);
    Init_Polling_Button(0, &m_KeyPolling_JoyY_D_Tag);
    Init_Polling_Button(0, &m_KeyPolling_JoyBtn_Tag);
    Init_Polling_Button(0, &m_KeyPolling_Pir_Tag);
    Init_Polling_Button(0, &m_KeyPolling_Fire_Tag);
}

void loop()
{
#if HARDWARE_DEBUG
    hardwareDebug();
#else
    // Get Sensor Data
    Task_ReadSensorData();
    Task_GetRFIDUID();
    Task_LCD();
    Task_Joystick();
    Task_WS2812();
    // Task_Buzzer();
#endif
}

void Task_LCD(void)
{
    static uint32_t timer    = 0;
    static uint8_t  pre_page = 0;

    if (pre_page != cur_page) {
        pre_page = cur_page;
        obj_lcd.clear();
    }

    if (millis() > timer) {
        timer         = millis() + 150;
        char buff[20] = {0};

        sprintf(buff, "   %s  %c", "IoT House V3.0", mode);
        lcd_print(0, 0, buff);
        switch (cur_page) {
        case 1: {
            sprintf(buff, " Temp:%.0f  PIR :%s ", m_temperature, m_pir ? TRIG_STR : IDLE_STR);
            lcd_print(0, 1, buff);
            sprintf(buff, " Humi:%.0f  Fire:%s ", m_humidity, m_fire ? TRIG_STR : IDLE_STR);
            lcd_print(0, 3, buff);
        } break;
        case 2: {
            sprintf(buff, "   Light:%04d       ", m_light_raw);
            lcd_print(0, 1, buff);
            sprintf(buff, "   RFID :%8s", card_uid.c_str());
            lcd_print(0, 3, buff);
        } break;
        case 3: {

        } break;
        default:
            break;
        }
    }
}

void lcd_print(uint8_t col, uint8_t row, char *data)
{
    obj_lcd.setCursor(col, row);
    obj_lcd.print(data);
}

void Task_Joystick()
{
    // default pullup
    m_KeyPolling_JoyX_L_Tag.bFlag = !(analogRead(JOYSTICK_X_PIN) < 1500);
    m_KeyPolling_JoyX_R_Tag.bFlag = !(analogRead(JOYSTICK_X_PIN) > 3500);
    m_KeyPolling_JoyY_U_Tag.bFlag = !(analogRead(JOYSTICK_Y_PIN) < 1500);
    m_KeyPolling_JoyY_D_Tag.bFlag = !(analogRead(JOYSTICK_Y_PIN) > 3500);
    m_KeyPolling_JoyBtn_Tag.bFlag = m_joystick_btn;

    m_uKeyCode_JoyX_L = Polling_Button_Repeat(&m_KeyPolling_JoyX_L_Tag);
    m_uKeyCode_JoyX_R = Polling_Button_Repeat(&m_KeyPolling_JoyX_R_Tag);
    m_uKeyCode_JoyY_U = Polling_Button_Repeat(&m_KeyPolling_JoyY_U_Tag);
    m_uKeyCode_JoyY_D = Polling_Button_Repeat(&m_KeyPolling_JoyY_D_Tag);
    m_uKeyCode_JoyBtn = Polling_Button_Repeat(&m_KeyPolling_JoyBtn_Tag);

    if (m_uKeyCode_JoyX_L == _KEYCODE_F_EDGE) {
        cur_page--;
    }
    if (m_uKeyCode_JoyX_R == _KEYCODE_F_EDGE) {
        cur_page++;
    }
    // check page range
    if (cur_page > PAGE_MAX) {
        cur_page = PAGE_MAX;
    } else if (cur_page < 1) {
        cur_page = 1;
    }
}

void Task_ReadSensorData(void)
{
    static uint32_t timer = 0;

    uint32_t cur_millis = millis();
    if (cur_millis > timer) {
        timer = cur_millis + 200;
#if HARDWARE_DEBUG
        M_DEBUG_PRINTLN("Sensor sampling");
#endif
        m_temperature = obj_dht11.readTemperature(false);
        m_humidity    = obj_dht11.readHumidity(false);
    }
    m_light_raw                 = analogRead(LIGHT_PIN);
    m_joystick_btn              = digitalRead(JOYSTICK_BTN_PIN);
    m_pir                       = !digitalRead(PIR_PIN);
    m_fire                      = !digitalRead(FIRE_PIN);
    m_KeyPolling_Pir_Tag.bFlag  = !m_pir;
    m_KeyPolling_Fire_Tag.bFlag = !m_fire;
    m_uKeyCode_Pir              = Polling_Button_Repeat(&m_KeyPolling_Pir_Tag);
    m_uKeyCode_Fire             = Polling_Button_Repeat(&m_KeyPolling_Fire_Tag);
}

void Task_GetRFIDUID(void)
{
    static uint32_t timer = 0, timer2 = 0;

    uint32_t cur_millis = millis();
    if (cur_millis > timer2) {
        if (card_uid != ID_NULL) {
            card_uid = ID_NULL;
        }
    }

    if (cur_millis > timer) {
        timer = cur_millis + 200;
        if (!obj_rc522.PICC_IsNewCardPresent()) {
            return;
        }
        if (!obj_rc522.PICC_ReadCardSerial()) {
            return;
        }

        String content = "";
        for (byte i = 0; i < obj_rc522.uid.size; i++) {
            content.concat(String(obj_rc522.uid.uidByte[i], HEX));
        }
        content.toUpperCase();
        card_uid = content;
        timer2   = cur_millis + 5000;
    }
}

void Task_WS2812(void)
{
    static uint32_t timer = 0;

    if (m_uKeyCode_JoyX_L == _KEYCODE_F_EDGE) {
        ws2812SetShow(JOY_LEFT, WS2812_WHITE);
    } else if (m_uKeyCode_JoyX_R == _KEYCODE_F_EDGE) {
        ws2812SetShow(JOY_RIGHT, WS2812_WHITE);
    } else if (m_uKeyCode_JoyY_U == _KEYCODE_F_EDGE) {
        ws2812SetShow(JOY_UP, WS2812_WHITE);
    } else if (m_uKeyCode_JoyY_D == _KEYCODE_F_EDGE) {
        ws2812SetShow(JOY_DOWN, WS2812_WHITE);
    }
    if (m_uKeyCode_JoyX_L == _KEYCODE_R_EDGE || m_uKeyCode_JoyX_R == _KEYCODE_R_EDGE || m_uKeyCode_JoyY_U == _KEYCODE_R_EDGE || m_uKeyCode_JoyY_D == _KEYCODE_R_EDGE) {
        obj_ws2812.clear();
        obj_ws2812.show();
    }

    uint32_t cur_millis = millis();
    if (cur_millis > timer) {
        timer              = cur_millis + 200;
        uint8_t brightness = map(m_light_raw, 0, 3500, 2, 100);
        obj_ws2812.setBrightness(brightness);
    }
}

void ws2812SetShow(uint16_t idx, uint32_t color)
{
    obj_ws2812.setPixelColor(idx, color);
    obj_ws2812.show();
}

void Task_Buzzer(void)
{
    static uint32_t timer = 0;
    if (m_uKeyCode_Pir == _KEYCODE_F_EDGE) {
        tone(BUZZER_PIN, BUZZER_Si);
        timer = millis() + 2000;
    } else if (m_uKeyCode_Pir == _KEYCODE_R_EDGE || millis() > timer) {
        noTone(BUZZER_PIN);
    }

    if (m_uKeyCode_Fire == _KEYCODE_F_EDGE) {
        tone(BUZZER_PIN, BUZZER_Do);
        timer = millis() + 2000;
    } else if (m_uKeyCode_Fire == _KEYCODE_R_EDGE || millis() > timer) {
        noTone(BUZZER_PIN);
    }
}

//--------------------------------------------------------
//	Return Code:
//		1. 0x00: No-Key, key is not pressed.
//		2. 0x01: Front-Edge Trigger.
//		3. 0x02: Repeat Trigger.
//		4. 0x03: Key still Pressed.
//		5. 0x04: Rear-Edge Trigger.
//--------------------------------------------------------
byte Polling_Button_Repeat(struct KEY_POLLING_STRUCT *pTag)
{
    // Check if it is time to do Polling-Key
    uint32_t dwTime = micros();
    if ((dwTime - pTag->dwTimeSlot_Polling) < _TIME_KEY_POLLING_uS)
        return 0x00;     // No-Key, time is not up

    // Process Polling Key
    pTag->dwTimeSlot_Polling = dwTime;

    byte uPinLevel = (pTag->bFlag ^ pTag->uActiveLevel) & 0x01;
    if (uPinLevel > 0) {     // Key is not pressed
        pTag->bPressKey = false;

        if (pTag->bEnableRepeat) {
            pTag->bEnableRepeat = false;
            return 0x04;     // Rear-Edge Trigger.
        } else {
            return 0x00;     // No-Key, key is not pressed.
        }
    } else {     // Key was pressed
        pTag->bPressKey = true;
        if (!pTag->bEnableRepeat) {
            pTag->dwTimeSlot_Repeat = dwTime;
            pTag->dwOrgtTime        = dwTime;
            pTag->dwRepeatTime      = _TIME_KEY_REPEAT_START_uS;
            pTag->bEnableRepeat     = true;
            pTag->uRepeatCount      = 0;
            return 0x01;     // Front-Edge Trigger.
        } else {
            if ((dwTime - pTag->dwTimeSlot_Repeat) > pTag->dwRepeatTime) {
                pTag->dwTimeSlot_Repeat = dwTime;
                pTag->dwRepeatTime      = _TIME_KEY_REPEAT_WORK_uS;
                pTag->uRepeatCount++;
                return 0x02;     // Repeat Trigger.
            } else {
                return 0x03;     // Key still Pressed.
            }
        }
    }
}

void FillBytes(byte *pDst, byte fillByte, byte uCount)
{
    byte i;
    for (i = 0; i < uCount; i++) {
        pDst[i] = fillByte;
    }
}

void Init_Polling_Button(byte uActiveLevel, struct KEY_POLLING_STRUCT *pTag)
{
    FillBytes((byte *)pTag, 0x00, sizeof(KEY_POLLING_STRUCT));
    pTag->uActiveLevel = uActiveLevel;
}

#if HARDWARE_DEBUG
void hardwareDebug(void)
{
    static uint32_t timer1 = 0, timer2 = 0, timer3 = 0;
    static uint32_t cur_sec     = 0;
    static bool     buzzer_flag = false;
    static uint8_t  idx         = 0;

#if HARDWARE_DEBUG_SENSOR
    // Debug All Sensor
    Task_ReadSensorData();
    M_DEBUG_PRINT("DHT11 Temp: ");
    M_DEBUG_PRINT(m_temperature);
    M_DEBUG_PRINT(" , ");
    M_DEBUG_PRINT("Humi: ");
    M_DEBUG_PRINT(m_humidity);
    M_DEBUG_PRINT(" , ");
    M_DEBUG_PRINT("Light: ");
    M_DEBUG_PRINT(m_light_raw);
    M_DEBUG_PRINT(" , ");
    M_DEBUG_PRINT("Pir: ");
    M_DEBUG_PRINT(m_pir);
    M_DEBUG_PRINT(" , ");
    M_DEBUG_PRINT("joy_x: ");
    M_DEBUG_PRINT(m_joystick_x);
    M_DEBUG_PRINT(" , ");
    M_DEBUG_PRINT("joy_y: ");
    M_DEBUG_PRINT(m_joystick_y);
    M_DEBUG_PRINT(" , ");
    M_DEBUG_PRINT("joy_btn: ");
    M_DEBUG_PRINT(m_joystick_btn);
    M_DEBUG_PRINT(" , ");
    M_DEBUG_PRINT("fire: ");
    M_DEBUG_PRINT(m_fire);
    M_DEBUG_PRINTLN();
#endif

#if HARDWARE_DEBUG_BUZZER
    // Debug Buzzer
    if (millis() > timer1) {
        buzzer_flag = !buzzer_flag;
        if (buzzer_flag) {
            timer1 = millis() + 50;
            tone(BUZZER_PIN, 500);
        } else {
            noTone(BUZZER_PIN);
            timer1 = millis() + 5000;
        }
    }
#endif

#if HARDWARE_DEBUG_WS2812
    // Debug WS2812
    if (millis() > timer2) {
        timer2 = millis() + 500;
        obj_ws2812.clear();
        obj_ws2812.setPixelColor(idx++, 0xFFFFFF);
        if (idx >= NUMPIXELS) {
            idx = 0;
        }
        obj_ws2812.show();
    }
#endif

#if HARDWARE_DEBUG_LCD
    // Debug LCD
    if (millis() > timer3) {
        timer3 = millis() + 1000;
        obj_lcd.setCursor(0, 0);
        obj_lcd.print(F("   Hello World !!   "));
        obj_lcd.setCursor(0, 1);
        obj_lcd.print(cur_sec++);
    }
#endif

#if HARDWARE_DEBUG_RFID
    // Debug RFID
    Task_GetRFIDUID();
    if (card_uid != ID_NULL) {
        M_DEBUG_PRINT(F("UID tag :"));
        M_DEBUG_PRINTLN(card_uid);
    }
#endif
}
#endif
