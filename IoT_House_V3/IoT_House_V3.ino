#include "config.h"
#include <Adafruit_NeoPixel.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Wire.h>

DHT               obj_dht11(DHT11_PIN, DHT11);
Adafruit_NeoPixel obj_ws2812(NUMPIXELS, WS2812_PIN, NEO_GRB + NEO_KHZ800);
MFRC522           obj_rc522(RFID_SS_PIN, 255);
LiquidCrystal_I2C obj_lcd(LCD_I2C_ADDR);

float    m_temperature  = 0.0f;
float    m_humidity     = 0.0f;
uint16_t m_light_raw    = 0;
bool     m_pir          = 0;
uint16_t m_joystick_x   = 0;
uint16_t m_joystick_y   = 0;
bool     m_joystick_btn = 0;
bool     m_fire         = 0;

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
}

void loop()
{
#if HARDWARE_DEBUG
    hardwareDebug();
#else
    readSensorData();
#endif
}

void readSensorData(void)
{
    static uint32_t timer = 0;

    uint32_t cur_millis = millis();
    if (cur_millis > timer) {
        timer = cur_millis + 200;
#if DEBUG_MODE
        M_DEBUG_PRINTLN("Sensor sampling");
#endif
        m_temperature  = obj_dht11.readTemperature(false);
        m_humidity     = obj_dht11.readHumidity(false);
        m_light_raw    = analogRead(LIGHT_PIN);
        m_pir          = digitalRead(PIR_PIN);
        m_joystick_x   = analogRead(JOYSTICK_X_PIN);
        m_joystick_y   = analogRead(JOYSTICK_Y_PIN);
        m_joystick_btn = digitalRead(JOYSTICK_BTN_PIN);
        m_fire         = digitalRead(FIRE_PIN);
    }
}

String getRFIDUID(void)
{
    if (!obj_rc522.PICC_IsNewCardPresent()) {
        return "";
    }
    if (!obj_rc522.PICC_ReadCardSerial()) {
        return "";
    }

    String content = "";
    for (byte i = 0; i < obj_rc522.uid.size; i++) {
        content.concat(String(obj_rc522.uid.uidByte[i], HEX));
    }
    content.toUpperCase();
    return content;
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
    readSensorData();
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
    String card_uid = getRFIDUID();
    if (card_uid != "") {
        M_DEBUG_PRINT(F("UID tag :"));
        M_DEBUG_PRINTLN(card_uid);
    }
#endif
}
#endif
