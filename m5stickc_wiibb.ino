//esp32_wiibb_oled.ino 2022/03/01
#include "Wiimote.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//#include <efontEnableJa.h> //プログラム使用率６２％
#include <efontEnableJaMini.h> //プログラム使用率４４％
#include <efont.h>

//#define EFONT_DEBUG　//デバッグ時コメント外す
#define LED        2

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET 5     //I2C NC
#define SCREEN_ADDRESS 0x3c //Address 0x3C
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//ESP_8_BIT_GFX & SSD1306 カラーコード
#define Black   SSD1306_BLACK
#define Blue    SSD1306_WHITE
#define Green   SSD1306_WHITE
#define Cyan    SSD1306_WHITE
#define Red     SSD1306_WHITE
#define Magenta SSD1306_WHITE
#define Yellow  SSD1306_WHITE
#define White   SSD1306_WHITE

Wiimote wiimote;
char w_kg[10];
float tr,br,tl,bl,total;
int button_A=0;
float w_off=0.0;
float wt;
int cal=0;
//**********************************************************************************************
//文字列をビデオに表示する
void disp(int16_t x,int16_t y,int16_t textsize,uint16_t color,String msg){
  oled.setCursor(x, y);
  oled.setTextSize(textsize);
  oled.setTextColor(color);
  oled.print(msg);
}
//efont 文字列をビデオに表示する
//**********************************************************************************************
void printEfont(int16_t x,int16_t y,int16_t txtsize,uint16_t color,uint16_t bgcolor,char *str) {
  int posX = x;
  int posY = y;
  int16_t textsize = txtsize;
  uint16_t textcolor = color;
  uint16_t textbgcolor = bgcolor;
  byte font[32];
  while( *str != 0x00 ){
    // 改行処理
    if( *str == '\n' ){
      // 改行
      posY += 16 * textsize;
      posX += 16 * textsize;
      str++;
      continue;
    }
    // フォント取得
    uint16_t strUTF16;
    str = efontUFT8toUTF16( &strUTF16, str );
    getefontData( font, strUTF16 );
    // 文字横幅
    int width = 16 * textsize;
    if( strUTF16 < 0x0100 ){
      // 半角
      width = 8 * textsize;
    }

#ifdef EFONT_DEBUG
    Serial.printf( "str : U+%04X\n", strUTF16 );
#endif

    // 背景塗りつぶし
    oled.fillRect(posX, posY, width, 16 * textsize, textbgcolor);
    // 取得フォントの確認
    for (uint8_t row = 0; row < 16; row++) {
      word fontdata = font[row*2] * 256 + font[row*2+1];
      for (uint8_t col = 0; col < 16; col++) {

#ifdef EFONT_DEBUG
        Serial.write( ( (0x8000 >> col) & fontdata ) ? "#" : " " );
#endif

        if( (0x8000 >> col) & fontdata ){
          int drawX = posX + col * textsize;
          int drawY = posY + row * textsize;
          if( textsize == 1 ){
            oled.drawPixel(drawX, drawY, textcolor);
          } else {
            oled.fillRect(drawX, drawY, textsize, textsize, textcolor);
          }
        }
      }

#ifdef EFONT_DEBUG
        Serial.write( "\n" );
#endif

    }
    // 描画カーソルを進める
    posX += width;
    // 折返し処理
    if( SCREEN_WIDTH <= posX ){ 
      posX = 0;
      posY += 16 * textsize;
    }
  }
  // カーソルを更新
  oled.setCursor(posX, posY);
}
//**********************************************************************************************
//四角形をビデオに表示する
void Rect(int16_t x1,int16_t y1,int16_t x2,int16_t y2,uint16_t cl){
  oled.drawFastVLine(x1,y1,y2-y1,cl);
  oled.drawFastVLine(x2,y1,y2-y1,cl);
  oled.drawFastHLine(x1,y1,x2-x1,cl);
  oled.drawFastHLine(x1,y2,x2-x1,cl); 
}

//**********************************************************************************************
void setup() {
  pinMode(LED, OUTPUT);
  Serial.begin(115200);
  while ( !Serial ) delay(100);
  for(int i=0;i<5;i++) {
    digitalWrite(LED, HIGH);
    delay(300);
    digitalWrite(LED, LOW);
    delay(300);
  }
  // SSD1306_SWITCHCAPVCC = 3.3Vから内部で表示電圧を生成
  if(!oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); //プログラムを停止します。
  }
  oled.display();//初期表示バッファの内容を画面に表示します
  wiimote.init(wiimote_callback);
 }
//**********************************************************************************************
void loop() {
  wiimote.handle();
  if (button_A) {
    w_off=total;
    cal=1;
  }
  wt=total-w_off;
  if (wt<0.5) wt=0.0;
  sprintf(w_kg,"%2.1f",wt);
  oled.clearDisplay();  //画面をクリア
  disp(1,2,1,Green,(String)"SSD1306 OLED & WiiBB");
  printEfont(  8,15,3,Cyan,Black,(char *)w_kg);
  printEfont(110,15,1,Cyan,Black,(char *)"kg");
  if(!cal) printEfont(80,30,2,Cyan,Black,(char *)"CAL");
  Rect(0,0,127,63,Yellow);//枠表示 
  oled.display();
  delay(10);
}

void wiimote_callback(wiimote_event_type_t event_type, uint16_t handle, uint8_t *data, size_t len) {
  static int connection_count = 0;
  printf("wiimote handle=%04X len=%d ", handle, len);
  if(event_type == WIIMOTE_EVENT_DATA){
    if(data[1]==0x32){
      for (int i = 0; i < 4; i++) {
        printf("%02X ", data[i]);
      }
      // http://wiibrew.org/wiki/Wiimote/Extension_Controllers/Nunchuck
      uint8_t* ext = data+4;
      printf(" ... Nunchuk: sx=%3d sy=%3d c=%d z=%d\n",
        ext[0],
        ext[1],
        0==(ext[5]&0x02),
        0==(ext[5]&0x01)
      );
    }else if(data[1]==0x34){
      for (int i = 0; i < 4; i++) {
        printf("%02X ", data[i]);
      }
      // https://wiibrew.org/wiki/Wii_Balance_Board#Data_Format
      uint8_t* ext = data+4;
      /*printf(" ... Wii Balance Board: TopRight=%d BottomRight=%d TopLeft=%d BottomLeft=%d Temperature=%d BatteryLevel=0x%02x\n",
        ext[0] * 256 + ext[1],
        ext[2] * 256 + ext[3],
        ext[4] * 256 + ext[5],
        ext[6] * 256 + ext[7],
        ext[8],
        ext[10]
      );*/
      
      float weight[4];
      wiimote.get_balance_weight(data, weight);
      tr=weight[BALANCE_POSITION_TOP_RIGHT];
      br=weight[BALANCE_POSITION_BOTTOM_RIGHT];
      tl=weight[BALANCE_POSITION_TOP_LEFT];
      bl=weight[BALANCE_POSITION_BOTTOM_LEFT];
      total=tr+br+tl+bl;
      digitalWrite(LED, HIGH);
      printf(" ... Wii Balance Board: TopRight=%2.1f BottomRight=%2.1f TopLeft=%2.1f BottomLeft=%2.1f total=%3.2f\n",tr,br,tl,bl,total);
    }else{
      for (int i = 0; i < len; i++) {
        printf("%02X ", data[i]);
      }
      printf("\n");
    }

    bool wiimote_button_down  = (data[2] & 0x01) != 0;
    bool wiimote_button_up    = (data[2] & 0x02) != 0;
    bool wiimote_button_right = (data[2] & 0x04) != 0;
    bool wiimote_button_left  = (data[2] & 0x08) != 0;
    bool wiimote_button_plus  = (data[2] & 0x10) != 0;
    bool wiimote_button_2     = (data[3] & 0x01) != 0;
    bool wiimote_button_1     = (data[3] & 0x02) != 0;
    bool wiimote_button_B     = (data[3] & 0x04) != 0;
    bool wiimote_button_A     = (data[3] & 0x08) != 0;
    bool wiimote_button_minus = (data[3] & 0x10) != 0;
    bool wiimote_button_home  = (data[3] & 0x80) != 0;
    if (wiimote_button_A) button_A=1; else button_A=0;
    static bool rumble = false;
    if(wiimote_button_plus && !rumble){
      wiimote.set_rumble(handle, true);
      rumble = true;
    }
    if(wiimote_button_minus && rumble){
      wiimote.set_rumble(handle, false);
      rumble = false;
    }
  }else if(event_type == WIIMOTE_EVENT_INITIALIZE){
    printf("  event_type=WIIMOTE_EVENT_INITIALIZE\n");
    wiimote.scan(true);
  }else if(event_type == WIIMOTE_EVENT_SCAN_START){
    printf("  event_type=WIIMOTE_EVENT_SCAN_START\n");
  }else if(event_type == WIIMOTE_EVENT_SCAN_STOP){
    printf("  event_type=WIIMOTE_EVENT_SCAN_STOP\n");
    if(connection_count==0){
      wiimote.scan(true);
    }
  }else if(event_type == WIIMOTE_EVENT_CONNECT){
    printf("  event_type=WIIMOTE_EVENT_CONNECT\n");
    wiimote.set_led(handle, 1<<connection_count);
    connection_count++;
  }else if(event_type == WIIMOTE_EVENT_DISCONNECT){
    printf("  event_type=WIIMOTE_EVENT_DISCONNECT\n");
    connection_count--;
    wiimote.scan(true);
  }else{
    printf("  event_type=%d\n", event_type);
  }
  delay(100);
}
