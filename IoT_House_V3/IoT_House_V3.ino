#include "config.h"
#include <Adafruit_NeoPixel.h>
#include <DHT.h>
#include <EEPROM.h>
#include <LRemote.h>
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
String   detect_rfid_id = "";
/* #endregion */

/* #region  Page and mode */
uint8_t cur_page = 1;
uint8_t cur_mode = AUTO_MODE;
/* #endregion */

/* #region  Key polling struct */
struct KEY_POLLING_STRUCT m_KeyPolling_JoyX_L_Tag;
struct KEY_POLLING_STRUCT m_KeyPolling_JoyX_R_Tag;
struct KEY_POLLING_STRUCT m_KeyPolling_JoyY_U_Tag;
struct KEY_POLLING_STRUCT m_KeyPolling_JoyY_D_Tag;
struct KEY_POLLING_STRUCT m_KeyPolling_JoyBtn_Tag;
struct KEY_POLLING_STRUCT m_KeyPolling_Pir_Tag;
struct KEY_POLLING_STRUCT m_KeyPolling_Fire_Tag;
/* #endregion */

/* #region  Key polling state */
byte m_uKeyCode_JoyX_L;
byte m_uKeyCode_JoyX_R;
byte m_uKeyCode_JoyY_U;
byte m_uKeyCode_JoyY_D;
byte m_uKeyCode_JoyBtn;
byte m_uKeyCode_Pir;
byte m_uKeyCode_Fire;
/* #endregion */

/* #region  system mode struct */
sys_modes_t sys_modes[3] = {
     {.mode_tag = 'A', .page_max = AUTO_MODE_PAGE_MAX  },
     {.mode_tag = 'M', .page_max = MANUAL_MODE_PAGE_MAX},
     {.mode_tag = 'S', .page_max = SET_MODE_PAGE_MAX   }
};
/* #endregion */

// device config
config_E config;

/* #region  other variable */
bool rfid_detect_flag = 0;
// range 0~1
float    ws2812_brightness = 0.0f;
uint32_t rfid_detect_time  = 0;
/* #endregion */

/* #region  LRemote object */
LRemoteSlider LRSlider_LED_R;
LRemoteSlider LRSlider_LED_G;
LRemoteSlider LRSlider_LED_B;
LRemoteSlider LRSlider_LED_Brightness;
LRemoteSwitch LRSwitch_Buzzer;
LRemoteLabel  LRLable_Temp;
LRemoteLabel  LRLable_Humi;
LRemoteLabel  LRLable_Pir;
LRemoteLabel  LRLable_Light;
LRemoteLabel  LRLable_Fire;
LRemoteLabel  LRLable_RFID;
/* #endregion */

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
    obj_ws2812.setBrightness(50);
    obj_ws2812.clear();
    obj_ws2812.show();

    /* read EEPROM */
    EEPROM.get(EEPROM_ADDR, config);

    Init_Polling_Button(0, &m_KeyPolling_JoyX_L_Tag);
    Init_Polling_Button(0, &m_KeyPolling_JoyX_R_Tag);
    Init_Polling_Button(0, &m_KeyPolling_JoyY_U_Tag);
    Init_Polling_Button(0, &m_KeyPolling_JoyY_D_Tag);
    Init_Polling_Button(0, &m_KeyPolling_JoyBtn_Tag);
    Init_Polling_Button(0, &m_KeyPolling_Pir_Tag);
    Init_Polling_Button(0, &m_KeyPolling_Fire_Tag);

    LRemoteSetting();
    delay(1000);
}

void loop()
{
#if HARDWARE_DEBUG
    hardwareDebug();
#else
    Task_ReadSensorData();
    Task_Joystick();
    Task_RFID();
    Task_LCD();
    Task_WS2812();
    Task_Buzzer();
    Task_LRemote();
    Task_Mode();
#endif
}

void Task_Mode(void)
{
    switch (cur_mode) {
    case AUTO_MODE: {
        switch (cur_page) {
        case 1:
        case 2: {
            if (m_uKeyCode_JoyX_L == _KEYCODE_F_EDGE) {
                cur_page--;
            }
            if (m_uKeyCode_JoyX_R == _KEYCODE_F_EDGE) {
                cur_page++;
            }
            if (m_uKeyCode_JoyBtn == _KEYCODE_F_EDGE) {
                next_mode();
            }
        } break;
        default:
            break;
        }
    } break;
    case MANUAL_MODE: {
        if (m_uKeyCode_JoyBtn == _KEYCODE_F_EDGE) {
            next_mode();
        }
    } break;
    case SET_MODE: {
        switch (cur_page) {
        case 1: {
            if (m_uKeyCode_JoyBtn == _KEYCODE_REPEAT && m_KeyPolling_JoyBtn_Tag.uRepeatCount >= 3) {
                change_mode(AUTO_MODE);
            }
        } break;
        default:
            break;
        }
    } break;
    default:
        break;
    }

    // check page range
    if (cur_page > sys_modes[cur_mode].page_max) {
        cur_page = sys_modes[cur_mode].page_max;
    } else if (cur_page < 1) {
        cur_page = 1;
    }
}

void Task_LCD(void)
{
    static uint32_t timer    = 0;
    static uint8_t  pre_page = 0, pre_mode = 0;
    static String   show_rfid = ID_NULL;

    if (pre_page != cur_page || pre_mode != cur_mode) {
        pre_page  = cur_page;
        pre_mode  = cur_mode;
        show_rfid = ID_NULL;
        obj_lcd.clear();
    }

    if (millis() > timer) {
        timer         = millis() + 150;
        char buff[20] = {0};

        sprintf(buff, "   IoT House V%.1f  %c", VERSION, sys_modes[cur_mode].mode_tag);
        lcd_print(0, 0, buff);
        switch (cur_mode) {
        case AUTO_MODE: {
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
                sprintf(buff, "   RFID :%8s", show_rfid.c_str());
                lcd_print(0, 3, buff);
            } break;
            default:
                break;
            }
            // All page functions in automatic mode are implemented here
            if (millis() > rfid_detect_time + 5000) {
                show_rfid = ID_NULL;
            } else {
                show_rfid = detect_rfid_id;
            }
        } break;
        case MANUAL_MODE: {
            lcd_print(0, 2, (char *)" Open Linkit Remote ");
        } break;
        case SET_MODE: {
            switch (cur_page) {
            case 1: {
                lcd_print(0, 1, (char *)"Input New RFID Card:");
                sprintf(buff, "New Card :%8s", show_rfid.c_str());
                lcd_print(0, 3, buff);
            } break;
            default:
                break;
            }
            if (rfid_detect_flag) {
                show_rfid = detect_rfid_id;
            }
        } break;
        default:
            break;
        }
    }
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

void Task_RFID(void)
{
    static uint32_t timer = 0;

    rfid_detect_flag    = false;
    uint32_t cur_millis = millis();
    if (cur_millis > timer) {
        timer = cur_millis + 200;
        if (!obj_rc522.PICC_IsNewCardPresent()) {
            return;
        }
        if (!obj_rc522.PICC_ReadCardSerial()) {
            return;
        }
        rfid_detect_flag = true;

        String content = "";
        for (byte i = 0; i < obj_rc522.uid.size; i++) {
            content.concat(String(obj_rc522.uid.uidByte[i], HEX));
        }
        content.toUpperCase();
        rfid_detect_time = millis();

        switch (cur_mode) {
        case AUTO_MODE: {
        case MANUAL_MODE:
            detect_rfid_id = content;
            break;
        case SET_MODE:
            detect_rfid_id = content;
            /* write EEPROM */
            strcpy(config.rfid_uid, content.c_str());
            EEPROM.put(EEPROM_ADDR, config);
            break;
        default:
            break;
        }
        }
    }
}

void Task_WS2812(void)
{
    static uint32_t timer         = 0;
    static bool     pre_rfid_flag = false;
    static uint8_t  pre_mode      = 0;

    if (pre_mode != cur_mode) {
        pre_mode = cur_mode;
        ws2812SetShow(WS2812_ALL, WS2812_BLACK);
    }

    switch (cur_mode) {
    case AUTO_MODE:
    case SET_MODE: {
        uint32_t cur_millis;
        bool     colse_flag = false, rfid_flag = false;

        if (m_uKeyCode_JoyX_L == _KEYCODE_F_EDGE) {
            ws2812SetShow(WS2812_LEFT, WS2812_WHITE);
        } else if (m_uKeyCode_JoyX_R == _KEYCODE_F_EDGE) {
            ws2812SetShow(WS2812_RIGHT, WS2812_WHITE);
        } else if (m_uKeyCode_JoyY_U == _KEYCODE_F_EDGE) {
            ws2812SetShow(WS2812_UP, WS2812_WHITE);
        } else if (m_uKeyCode_JoyY_D == _KEYCODE_F_EDGE) {
            ws2812SetShow(WS2812_DOWN, WS2812_WHITE);
        } else if (m_uKeyCode_JoyBtn == _KEYCODE_F_EDGE) {
            ws2812SetShow(WS2812_ALL, WS2812_WHITE);
        }

        if (millis() < rfid_detect_time + 5000) {
            if (rfid_detect_flag) {
                rfid_flag = false;
                if (strcmp(config.rfid_uid, detect_rfid_id.c_str()) == 0) {
                    ws2812SetShow(WS2812_ALL, WS2812_GREEN);
                } else if (strcmp(config.rfid_uid, detect_rfid_id.c_str()) != 0) {
                    ws2812SetShow(WS2812_ALL, WS2812_RED);
                }
            }
        } else {
            rfid_flag = true;
        }

        colse_flag |= (m_uKeyCode_JoyX_L == _KEYCODE_R_EDGE);
        colse_flag |= (m_uKeyCode_JoyX_R == _KEYCODE_R_EDGE);
        colse_flag |= (m_uKeyCode_JoyY_U == _KEYCODE_R_EDGE);
        colse_flag |= (m_uKeyCode_JoyY_D == _KEYCODE_R_EDGE);
        colse_flag |= (m_uKeyCode_JoyBtn == _KEYCODE_R_EDGE);

        if (pre_rfid_flag != rfid_flag) {
            pre_rfid_flag = rfid_flag;
            colse_flag |= rfid_flag;
        }

        if (colse_flag) {
            ws2812SetShow(WS2812_ALL, WS2812_BLACK);
        }

        cur_millis = millis();
        if (cur_millis > timer) {
            timer                  = cur_millis + 200;
            uint8_t brightness_x10 = map(m_light_raw, 0, 3500, 1, 10);
            ws2812_brightness      = brightness_x10 * 0.1;
        }
    } break;
    case MANUAL_MODE: {
        uint8_t r              = (uint8_t)LRSlider_LED_R.getValue();
        uint8_t g              = (uint8_t)LRSlider_LED_G.getValue();
        uint8_t b              = (uint8_t)LRSlider_LED_B.getValue();
        uint8_t brightness_x10 = (uint8_t)LRSlider_LED_Brightness.getValue();
        ws2812_brightness      = brightness_x10 * 0.1;
        ws2812SetShow(WS2812_ALL, r, g, b);
    } break;
    default:
        break;
    }
}

void Task_Buzzer(void)
{
    static uint32_t timer = 0;

    switch (cur_mode) {
    case AUTO_MODE: {
        if (m_uKeyCode_Pir == _KEYCODE_F_EDGE) {
            tone(BUZZER_PIN, BUZZER_Si);
            timer = millis() + 1000;
        } else if (m_uKeyCode_Pir == _KEYCODE_R_EDGE) {
            noTone(BUZZER_PIN);
        }

        if (m_uKeyCode_Fire == _KEYCODE_F_EDGE) {
            tone(BUZZER_PIN, BUZZER_Do);
            timer = millis() + 1000;
        } else if (m_uKeyCode_Fire == _KEYCODE_R_EDGE) {
            noTone(BUZZER_PIN);
        }

        if (rfid_detect_flag || m_uKeyCode_JoyBtn == _KEYCODE_F_EDGE) {
            tone(BUZZER_PIN, BUZZER_Fa);
            timer = millis() + 100;
        }

        if (m_uKeyCode_JoyBtn == _KEYCODE_REPEAT && m_KeyPolling_JoyBtn_Tag.uRepeatCount >= 3) {
            tone(BUZZER_PIN, BUZZER_La);
            timer = millis() + 100;
        }
    } break;
    case MANUAL_MODE: {
        if (rfid_detect_flag || m_uKeyCode_JoyBtn == _KEYCODE_F_EDGE) {
            tone(BUZZER_PIN, BUZZER_Fa);
            timer = millis() + 100;
        }

        if (m_uKeyCode_JoyBtn == _KEYCODE_REPEAT && m_KeyPolling_JoyBtn_Tag.uRepeatCount >= 3) {
            tone(BUZZER_PIN, BUZZER_La);
            timer = millis() + 100;
        }
        bool btn_state = (bool)LRSwitch_Buzzer.getValue();
        if (btn_state) {
            tone(BUZZER_PIN, BUZZER_Si);
            timer = millis() + 200;
        }
    } break;
    case SET_MODE: {
        if (rfid_detect_flag || m_uKeyCode_JoyBtn == _KEYCODE_F_EDGE) {
            tone(BUZZER_PIN, BUZZER_Fa);
            timer = millis() + 100;
        }

        if (m_uKeyCode_JoyBtn == _KEYCODE_REPEAT && m_KeyPolling_JoyBtn_Tag.uRepeatCount >= 3) {
            tone(BUZZER_PIN, BUZZER_La);
            timer = millis() + 100;
        }
    } break;
    default:
        break;
    }

    if (millis() > timer) {
        noTone(BUZZER_PIN);
    }
}

void Task_LRemote(void)
{
    static uint32_t timer = 0;

    if (millis() > timer) {
        timer = millis() + 200;
        LRemote.process();
        switch (cur_mode) {
        case AUTO_MODE: {
        } break;
        case MANUAL_MODE: {
            LRLable_Temp.updateText("T:" + String(m_temperature));
            LRLable_Humi.updateText("H:" + String(m_humidity));
            LRLable_Pir.updateText("P:" + String(m_pir));
            LRLable_Fire.updateText("F:" + String(m_fire));
            LRLable_Light.updateText("L:" + String(m_light_raw));
            LRLable_RFID.updateText("R:" + detect_rfid_id);
        } break;
        case SET_MODE: {
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

void ws2812SetShow(uint16_t ws2812_mode, uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t color = obj_ws2812.Color(r, g, b);
    ws2812SetShow(ws2812_mode, color);
}

void ws2812SetShow(uint16_t ws2812_mode, uint32_t color)
{
    uint8_t r, g, b;
    r = (color >> 16) & 0xFF;
    g = (color >> 8) & 0xFF;
    b = color & 0xFF;

    // set Brightness
    r *= ws2812_brightness;
    g *= ws2812_brightness;
    b *= ws2812_brightness;
    color = obj_ws2812.Color(r, g, b);

    obj_ws2812.clear();
    switch (ws2812_mode) {
    case WS2812_LEFT:
    case WS2812_DOWN:
    case WS2812_RIGHT:
    case WS2812_UP:
        obj_ws2812.setPixelColor(ws2812_mode, color);
        break;
    case WS2812_ALL:
        for (uint8_t i = 0; i < NUMPIXELS; i++) {
            obj_ws2812.setPixelColor(i, color);
        }
        break;
    default:
        break;
    }
    obj_ws2812.show();
    obj_ws2812.show();
}

void change_mode(uint8_t new_mode)
{
    if (new_mode >= MAX_MODE) {
        DEBUG_PRINTLN(F("out of mode range!"));
        return;
    }
    cur_page = 1;
    cur_mode = new_mode;
}

void next_mode(void)
{
    cur_mode++;
    if (cur_mode >= MAX_MODE) {
        cur_mode = 0;
    }
    cur_page = 1;
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

void LRemoteSetting(void)
{
    char buff[16] = {0};
    sprintf(buff, "IoT House V%.1f", VERSION);
    LRemote.setName(buff);
    LRemote.setOrientation(RC_PORTRAIT);
    LRemote.setGrid(2, 8);

    LRLable_Temp.setPos(0, 0);
    LRLable_Temp.setSize(1, 1);
    LRLable_Temp.setText(String("Temperature"));

    LRLable_Humi.setPos(1, 0);
    LRLable_Humi.setSize(1, 1);
    LRLable_Humi.setText(String("Humidity"));

    LRLable_Pir.setPos(0, 1);
    LRLable_Pir.setSize(1, 1);
    LRLable_Pir.setText(String("Pir"));

    LRLable_Fire.setPos(1, 1);
    LRLable_Fire.setSize(1, 1);
    LRLable_Fire.setText(String("Fire"));

    LRLable_Light.setPos(0, 2);
    LRLable_Light.setSize(1, 1);
    LRLable_Light.setText(String("Light"));

    LRLable_RFID.setPos(1, 2);
    LRLable_RFID.setSize(1, 1);
    LRLable_RFID.setText(String("RFID"));

    LRSlider_LED_R.setPos(0, 3);
    LRSlider_LED_R.setSize(2, 1);
    LRSlider_LED_R.setText(String("LED R"));
    LRSlider_LED_R.setColor(RC_PINK);
    LRSlider_LED_R.setValueRange(0, 255, 0);

    LRSlider_LED_G.setPos(0, 4);
    LRSlider_LED_G.setSize(2, 1);
    LRSlider_LED_G.setText(String("LED G"));
    LRSlider_LED_G.setColor(RC_GREEN);
    LRSlider_LED_G.setValueRange(0, 255, 0);

    LRSlider_LED_B.setPos(0, 5);
    LRSlider_LED_B.setSize(2, 1);
    LRSlider_LED_B.setText(String("LED B"));
    LRSlider_LED_B.setColor(RC_BLUE);
    LRSlider_LED_B.setValueRange(0, 255, 0);

    LRSlider_LED_Brightness.setPos(0, 6);
    LRSlider_LED_Brightness.setSize(2, 1);
    LRSlider_LED_Brightness.setText(String("LED Brightness"));
    LRSlider_LED_Brightness.setColor(RC_YELLOW);
    LRSlider_LED_Brightness.setValueRange(1, 10, 1);

    LRSwitch_Buzzer.setPos(0, 7);
    LRSwitch_Buzzer.setSize(1, 1);
    LRSwitch_Buzzer.setText(String("Buzzer"));
    LRSwitch_Buzzer.setColor(RC_ORANGE);

    LRemote.addControl(LRLable_Temp);
    LRemote.addControl(LRLable_Humi);
    LRemote.addControl(LRLable_Pir);
    LRemote.addControl(LRLable_Fire);
    LRemote.addControl(LRLable_Light);
    LRemote.addControl(LRLable_RFID);
    LRemote.addControl(LRSlider_LED_R);
    LRemote.addControl(LRSlider_LED_G);
    LRemote.addControl(LRSlider_LED_B);
    LRemote.addControl(LRSlider_LED_Brightness);
    LRemote.addControl(LRSwitch_Buzzer);

    LRemote.begin();
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
    Task_RFID();
    if (detect_rfid_id != ID_NULL) {
        M_DEBUG_PRINT(F("UID tag :"));
        M_DEBUG_PRINTLN(detect_rfid_id);
    }
#endif
}
#endif
