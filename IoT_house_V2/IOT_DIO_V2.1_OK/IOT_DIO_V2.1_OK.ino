/*智慧大屋20191204_無緣蜂鳴器版
/~~~~~~~~
須更新RFID_ID=>277~312=>兩區塊
                  ~~~~~~~~~~/
QC項目如下：
火焰：打火機感測=>發出警報+亮紅燈
光敏：遮蔽=>LCD顯示黑暗
RFID：白卡=>上鎖+紅燈+LCD顯示=>解鎖+綠燈+LCD顯示
      藍卡=>錯誤警報+LCD顯示
溫溼度：加熱或降溫=>LCD顯示
搖桿：左右撥桿=>LCD換頁
人體動態感測：靠近模組=>LCD顯示

確認蜂鳴器：會正常叫
RGB_LED：亮綠、紅燈
*/
#include <LRemote.h>//APP使用，7697庫?
#include <LiquidCrystal_I2C.h>//LCD2004
LiquidCrystal_I2C lcd_i2c(0x27);//(0x27、0x3F)
//LiquidCrystal_I2C lcd_i2c(0x3F);
#include <DHT.h>//溫溼度
#include <MFRC522.h>//RFID

//#include <Adafruit_NeoPixel.h>//WS2812用Version：1.10.3

//#include <SPI.h>
//#include "Wire.h"
//#include "U8g2lib.h"


DHT dht11_p3(5, DHT11);
MFRC522 rfid(/*SS_PIN*/ 10, /*RST_PIN*/ UINT8_MAX);

//蜂鳴器
#define Do  523
#define Re  587
#define Mi  659
#define Fa  698
#define So  784
#define La  880
#define Si  988
int melody[7] = {Do, Re, Mi, Fa, So, La, Si};
const int buzzer = 3;

int Fire;
int HC_SR501_Data;
int Humidity_Data;
int light_Data;
String RFID_Data;
int Temperature_Data;
int Buzzer_Data;
int key_Data;
int Rocker_Data;
int R_Data;
int page_Data;
int G_Data;

LRemoteLabel Humidity_APP;
LRemoteLabel Temperature_APP;
LRemoteLabel light_APP;
LRemoteLabel PIR_APP;
LRemoteButton Buzzer_APP;
LRemoteLabel Fire_APP;
LRemoteSlider Color_R_APP;
LRemoteSlider Color_G_APP;
/*
RC_ORANGE局
RC_BLUE藍
RC_GREEN綠
RC_PINK粉
RC_GREY輝
RC_YELLOW黃

 */

void setup(){
  Serial.begin(9600);
  lcd_i2c.begin(20, 4);

//~~~~~~~~~~~~~~~~~~~~  APP  ~~~~~~~~~~~~~~~~~~~~~~~~  
  LRemote.setName(" Wisdom House");
  LRemote.setOrientation(RC_PORTRAIT);
  LRemote.setGrid(3,4);//小數點無效,(1~4,1~8)
  Humidity_APP.setPos(0, 0);
  Humidity_APP.setText("Humidity:");
  Humidity_APP.setSize(1, 1);
  Humidity_APP.setColor(RC_PINK);
  LRemote.addControl(Humidity_APP);

  Temperature_APP.setPos(1, 0);
  Temperature_APP.setText("temperature:");
  Temperature_APP.setSize(1, 1);
  Temperature_APP.setColor(RC_PINK);
  LRemote.addControl(Temperature_APP);

  light_APP.setPos(2, 0);
  light_APP.setText("light");
  light_APP.setSize(1, 1);
  light_APP.setColor(RC_PINK);
  LRemote.addControl(light_APP);
     ////////////////////
  PIR_APP.setPos(0,1 );
  PIR_APP.setText("PIR");
  PIR_APP.setSize(1, 1);
  PIR_APP.setColor(RC_BLUE);
  LRemote.addControl(PIR_APP);

  Buzzer_APP.setPos(1, 1);
  Buzzer_APP.setText("Buzzer");
  Buzzer_APP.setSize(1, 1);
  Buzzer_APP.setColor(RC_BLUE);
  LRemote.addControl(Buzzer_APP);

  Fire_APP.setPos(2, 1);
  Fire_APP.setText("Water");
  Fire_APP.setSize(1, 1);
  Fire_APP.setColor(RC_BLUE);
  LRemote.addControl(Fire_APP);
     ////////////////////
  Color_R_APP.setPos(0, 2);
  Color_R_APP.setText("Color_R_APP");
  Color_R_APP.setSize(3, 1);
  Color_R_APP.setValueRange(0, 255, 0);
  Color_R_APP.setColor(RC_YELLOW);
  LRemote.addControl(Color_R_APP);

  Color_G_APP.setPos(0, 3);
  Color_G_APP.setText("Color_G_APP");
  Color_G_APP.setSize(3, 1); 
  Color_G_APP.setValueRange(0, 255, 0);
  Color_G_APP.setColor(RC_YELLOW);
  LRemote.addControl(Color_G_APP);
     ////////////////////
  LRemote.begin();
  ////////////////////
  
//~~~~~~~~~~~~~~~  LCD  ~~~~~~~~~~~~~~~~~~~~~~  
  lcd_i2c.backlight();
  lcd_i2c.clear();
  lcd_i2c.setCursor(6,0);
  lcd_i2c.print("Welcome");
  lcd_i2c.setCursor(9,1);
  lcd_i2c.print("To");
  lcd_i2c.setCursor(3,2);
  lcd_i2c.print(" Wisdom House");
  lcd_i2c.setCursor(1,3);
  lcd_i2c.print("ICshop");
  lcd_i2c.setCursor(10,3);
  lcd_i2c.print("V_DIO.V2.1");
  delay(2000);
  lcd_i2c.clear();
  HC_SR501_Data = 0;
  Fire = 0;
  pinMode(buzzer, OUTPUT);
  Buzzer_Data = 0;
  Fire = 0;
  lcd_i2c.clear();
  page_Data = 1;
  key_Data = 1;
  dht11_p3.begin();
  SPI.begin();
  rfid.PCD_Init();
  
  pinMode(5, OUTPUT);
  pinMode(4, INPUT);
  pinMode(2, INPUT);
  pinMode(14, OUTPUT);
  pinMode(15, OUTPUT);


  tone(buzzer, 500,100);
  delay(300);
  tone(buzzer, 600,100);
  delay(100);
  tone(buzzer, 1000,100);
  delay(300);

}

void loop(){
  if(LRemote.connected() == true) {
    Serial.println("AUTO");
    APP();
    PC();
  } else {
    Serial.println("ON AUTO");
    PC();
  }
  //delay(100);
}

void APP() {
  LRemote.process();
  
  if(Buzzer_APP.isValueChanged()){
    Serial.println(Buzzer_APP.getValue());
    tone(buzzer, 600,100);
    delay(100);
    tone(buzzer, 1000,100);
    delay(300);
    //digitalWrite(3,HIGH); 
    delay(100);    
  }else{
    //digitalWrite(3,LOW);   
    delay(100); 
  }
  Humidity_APP.updateText(String(String() + "濕度:" + Humidity_Data));
  Temperature_APP.updateText(String(String() + "溫度:" + Temperature_Data));
  light_APP.updateText(String(String() + "light:" + light_Data));
  PIR_APP.updateText(String(String() + "PIR:" + HC_SR501_Data));
  Fire_APP.updateText(String(String() + "Fire:" + Fire));
  
  if(Color_R_APP.isValueChanged()){
    Serial.print("Color_R_APP to new value = ");
    Serial.println(Color_R_APP.getValue());
    analogWrite(17, map(Color_R_APP.getValue(),0,255,255,0));  
  }
  if(Color_G_APP.isValueChanged()){
    Serial.print("Color_G_APP to new value = ");
    Serial.println(Color_G_APP.getValue());
    analogWrite(16, map(Color_G_APP.getValue(),0,255,255,0));  
  }
}

void PC() {
  Rocker();
  Buzzer_Block();
  HC_SR501();
  DHC_11();
  Fire2();
  light();
  RFID();
  Key();
}

void Buzzer_X3(){
  for (int count = 0; count < 3; count++) {
      tone(buzzer, 500,500);
      delay(500);
      tone(buzzer, 1000,500);
      delay(500);
    }
}

void Buzzer_Block() {//錯誤警報
  if (Buzzer_Data == 1) {
    Buzzer_X3();
  } else {
    noTone(buzzer);
    delay(500);
  }
}

void Fire2() {
  pinMode(2, INPUT);
  Fire = digitalRead(2);
  Fire = 1 - Fire;
  if (Fire == 0) {
    Buzzer_Data = 0;
    if (page_Data == 2) {
      lcd_i2c.setCursor(0,1);
      lcd_i2c.print("Fira = ");
      lcd_i2c.print("Fety       ");
    }
  } else {
    Buzzer_Data = 1;
    if (page_Data == 2) {
      lcd_i2c.setCursor(0,1);
      lcd_i2c.print("Fira = ");
      lcd_i2c.print("--Danger--");
    }
  }
}

void HC_SR501() {
  pinMode(16, INPUT);
  HC_SR501_Data= digitalRead(16);
  if (HC_SR501_Data == 0) {
    if (page_Data == 1) {
      lcd_i2c.setCursor(0,1);
      lcd_i2c.print("PIR = ");
      lcd_i2c.setCursor(12,1);
      lcd_i2c.print("        ");
      lcd_i2c.setCursor(6,1);
      lcd_i2c.print("Safety");
    }
    Buzzer_Data = 0;
  } else {
    if (page_Data == 1) {
      lcd_i2c.setCursor(0,1);
      lcd_i2c.print("PIR = ");
      lcd_i2c.print("Someone");
    }
    Buzzer_Data = 1;
  }
}

void DHC_11() {                                                                                                  
  Humidity_Data = dht11_p3.readHumidity();
  Temperature_Data = dht11_p3.readTemperature();
  if (page_Data == 1) {
    lcd_i2c.setCursor(0,2);
    lcd_i2c.print("Humidity = ");
    lcd_i2c.print(Humidity_Data);
    lcd_i2c.print("%");
    lcd_i2c.setCursor(0,3);
    lcd_i2c.print("Temperature = ");
    lcd_i2c.print(Temperature_Data);
    lcd_i2c.print("c");
  }
}

void light() {
  //light_Data = map(4100 - analogRead(A0),0,4100,0,100);
  pinMode(4, INPUT);
  light_Data = digitalRead(4);

  
  if (light_Data == 0) {
    if (page_Data == 2) {
      lcd_i2c.setCursor(0,2);
      lcd_i2c.print("Light = ");
      lcd_i2c.print("Bright");//亮
    }
  } else {
    if (page_Data == 2) {
      lcd_i2c.setCursor(0,2);
      lcd_i2c.print("Light = ");
      lcd_i2c.print("dark   ");//按
    }
  }
}

void Rocker() {
  lcd_i2c.setCursor(2,0);
  lcd_i2c.print(" Wisdom House");
  Rocker_Data = analogRead(A3);
  if (Rocker_Data < 2000) {
    page_Data = page_Data - 1;
    lcd_i2c.clear();
    if (page_Data <= 1) {
      page_Data = 1;
    }
  } else if (Rocker_Data > 3000) {
    page_Data = page_Data + 1;
    lcd_i2c.clear();
    if (page_Data >= 3) {
      page_Data = 2;
    }
  } else {
  }
  lcd_i2c.setCursor(18,0);
  lcd_i2c.print("P");
  lcd_i2c.print(page_Data);
}

void Key() {
  //lcd_i2c.setCursor(18,1);
  //lcd_i2c.print(key_Data);
  if (key_Data == -1) {
    R_Data = 1;
    G_Data = 0;
  } else if (key_Data == 1) {
    R_Data = 0;
    G_Data = 1;
  }
  if(LRemote.connected() != true){
      if (R_Data == 1) {
      digitalWrite(14, LOW);
    } else {
      digitalWrite(14, HIGH);
    }
    if (G_Data == 1) {
      digitalWrite(15, LOW);
    } else {
      digitalWrite(15, HIGH);
    }  
  }
  
}

String mfrc522_readID(){
  String ret;
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()){
    MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
    for (byte i = 0; i < rfid.uid.size; i++) {
      ret += (rfid.uid.uidByte[i] < 0x10 ? "0" : "");
      ret += String(rfid.uid.uidByte[i], HEX);
    }
  }
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  return ret;
}

void RFID() {
  RFID_Data = mfrc522_readID();
  Serial.print("RFID_Data = ");
  Serial.println(RFID_Data);
  if (page_Data == 2) {
    lcd_i2c.setCursor(0,3);
    lcd_i2c.print("RFID=");
    if (RFID_Data != "") {
      
      lcd_i2c.setCursor(6,3);
      lcd_i2c.print(RFID_Data);
      if (
          RFID_Data == "c7d61053" or //範本 

          RFID_Data == "63fe1108"or
          RFID_Data == "63238e08"or
          RFID_Data == "d31eca08"or
          RFID_Data == "b3ac8e0a"or
          RFID_Data == "23caa908"or

          RFID_Data == "63c5590a"or
          RFID_Data == "736d170a"or
          RFID_Data == "5388fc07"or
          RFID_Data == "437b160a"or
          RFID_Data == "c76b383e"or
          RFID_Data == "c0dfef1b"or
          RFID_Data == "80d6921b"
          
          ){
        digitalWrite(14, HIGH);
        lcd_i2c.print(" OK");
        key_Data = key_Data * -1;
        tone(buzzer, 1000,100);
        digitalWrite(15, HIGH);
        delay(100);
        digitalWrite(15, LOW);
        delay(100);
      } else {
        digitalWrite(15, HIGH);
        lcd_i2c.print(" Error");
        for (int count2 = 0; count2 < 3; count2++) {
          tone(buzzer, 1000,100);
          digitalWrite(14, HIGH);
          delay(100);
          digitalWrite(14, LOW);
          delay(100);
        }
        delay(1000);
      }
    } else {
      lcd_i2c.print("               ");
    }
  } else {
    if (RFID_Data != "") {
      if (
          RFID_Data == "c7d61053" or //範本 

          RFID_Data == "63fe1108"or
          RFID_Data == "63238e08"or
          RFID_Data == "d31eca08"or
          RFID_Data == "b3ac8e0a"or
          RFID_Data == "23caa908"or

          RFID_Data == "63c5590a"or
          RFID_Data == "736d170a"or
          RFID_Data == "5388fc07"or
          RFID_Data == "437b160a"or
          RFID_Data == "c76b383e"or
          RFID_Data == "c0dfef1b"or
          RFID_Data == "80d6921b"
          ){
      digitalWrite(14, HIGH);
        key_Data = key_Data * -1;
        tone(buzzer, 1000,100);
        digitalWrite(15, HIGH);
        delay(100);
        digitalWrite(15, LOW);
        delay(100);
      } else {
        digitalWrite(15, HIGH);
        for (int count3 = 0; count3 < 3; count3++) {
          tone(buzzer, 1000,100);
          digitalWrite(14, HIGH);
          delay(100);
          digitalWrite(14, LOW);
          delay(100);
        }
        delay(500);
      }
    }
  }
}
