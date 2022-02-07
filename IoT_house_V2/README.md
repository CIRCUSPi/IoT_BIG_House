# IoT_BIG_House
智慧大屋

IOT智慧大屋使用說明

1. 資料夾檔名library：此資料夾內的程式庫需複製到使用者自己安裝好的ARDUINO的library資料夾內。

2. 資料夾檔名IOT_DIO_V2.1_OK：為主程式，開啟後須增加RFID的ID(白卡上有寫ID)才可使用自己的白卡。

3. 更改位置兩處：
於407行的if判斷內部
RFID_Data == "c7d61053(ID)" or //範本 此行的下一行新增即可。

於450行的if判斷內部
RFID_Data == "c7d61053(ID)" or //範本 此行的下一行新增即可
