//=====================================================================
//  Trillion Node Engine
//     Platform     : ADI ACCELEROMETER LEAF
//     Processor    : ATmega328P (3.3V /8MHz)
//     Application  : demonstration for ADI
//
//     Leaf configuration
//       (1) 8bit-MCU Leaf
//       (2) ADI ACCELE LEAF
//       (3) LCD LEAF 2018
//       (4) BAT(CR2032) LEAF or BAT(CR2450) LEAF
//
//     Rev.02 2019/04/01  NEXTY Yoshihide NAKAYAMA
//=====================================================================
//動作
//電源投入後、ADXL362のデバイスID等を読み出し
//Normalモードで各軸(XYZ)データ表示を繰り返す
//=====================================================================
//
//=====================================================================
//制限事項
//ADI ACCELE LEAFのINT1とLCD LEAF 2018のSW1はPORTが被っているため
//LCD LEAF 2018のSW1は押下でON(HIGH)ということと
//ADI ACCELE LEAFの回路上、ほぼ常にHIGHとなり、使用不可
//=====================================================================

#include <SPI.h>
//const int SS = 10;
//const int MOSI = 11;
//const int MISO = 12;
//const int SCK  = 13;

#include <Wire.h>
#include <ST7032.h>

//=====================================================================
// definition
//=====================================================================

//LCD LEAF 2018 (ST7032)
#define I2C_EXPANDER_ADDR   0x1A

//ADI ACCELE LEAF(ADXL362)
const int ADXL362_CS = SS;
const int INT1_N = 2; //LCD LEAF 2018とはSWで被っているため使用不可
const int INT2_N = 3;
const int PWR_EN = 4;
const int VOUTGOOD = 5;
const byte ADXL362_DATAX0 = 0x08;
const byte ADXL362_DATAY0 = 0x09;
const byte ADXL362_DATAZ0 = 0x0A;
const byte ADXL362_DATAXL = 0x0E;
const byte ADXL362_DATAXH = 0x0F;
const byte ADXL362_DATAYL = 0x10;
const byte ADXL362_DATAYH = 0x11;
const byte ADXL362_DATAZL = 0x12;
const byte ADXL362_DATAZH = 0x13;
const byte ADXL362_DATATL = 0x14;
const byte ADXL362_DATATH = 0x15;
const byte ADXL362_SFTRST = 0x1F;
const byte ADXL362_THREACTL = 0x20;
const byte ADXL362_THREACTH = 0x21;
const byte ADXL362_THREACTT = 0x22;
const byte ADXL362_THREINACTL = 0x23;
const byte ADXL362_THREINACTH = 0x24;
const byte ADXL362_THREINACTTL = 0x25;
const byte ADXL362_THREINACTTH = 0x26;
const byte ADXL362_ACTINACT_CTL = 0x27;
const byte ADXL362_FIFO_CTL = 0x28;
const byte ADXL362_FIFO_SAM = 0x29;
const byte ADXL362_INTMAP1 = 0x2A;
const byte ADXL362_INTMAP2 = 0x2B;
const byte ADXL362_FLT_CTL = 0x2C;
const byte ADXL362_PWR_CTL = 0x2D;
const byte ADXL362_SLFTST = 0x2E;

const byte WRITE = 0xa;
const byte READ = 0xb;
const byte READ_FIFO = 0xd;

//=====================================================================


ST7032 lcd;

char buf[10];
/**********************************************
* I2C スレーブデバイスに1バイト書き込む
**********************************************/
void i2c_write_byte(int device_address, int reg_address, int write_data){
  Wire.beginTransmission(device_address);
  Wire.write(reg_address);
  Wire.write(write_data);
  Wire.endTransmission();
}
/**********************************************
* I2C スレーブデバイスから1バイト読み込む
**********************************************/
unsigned char i2c_read_byte(int device_address, int reg_address){

  int read_data = 0;

  Wire.beginTransmission(device_address);
  Wire.write(reg_address);
  Wire.endTransmission(false);

  Wire.requestFrom(device_address, 1);
  read_data = Wire.read();

  return read_data;
}

/**********************************************
* SPIでADXL362へ書き込む
**********************************************/
byte regWrite(byte reg, byte val)
{
  digitalWrite(ADXL362_CS, LOW);
  SPI.transfer(WRITE);
  SPI.transfer(reg);
  SPI.transfer(val);
  digitalWrite(ADXL362_CS, HIGH);
}

/**********************************************
* SPIでADXL362から読み込む
**********************************************/
byte regRead(byte reg)
{
  byte ret;
  
  digitalWrite(ADXL362_CS, LOW);
  SPI.transfer(READ);
  SPI.transfer(reg);
  ret = SPI.transfer(0); 
  digitalWrite(ADXL362_CS, HIGH); 

  return ret;
}

/**********************************************
*各軸の加速度値を読み込む
**********************************************/
//X軸
double  accele_Xval_get() {
  short val;
  double ret;
  val = (regRead(ADXL362_DATAXH) << 8) | regRead(ADXL362_DATAXL);
  ret = val * 0.00390625; // 0.00390625 = ±8g * 2 / 2 ^ 12bit
  return ret;
}
//Y軸
double  accele_Yval_get() {
  short val;
  double ret;
  val = (regRead(ADXL362_DATAYH) << 8) | regRead(ADXL362_DATAYL);
  ret = val * 0.00390625; // 0.00390625 = ±8g * 2 / 2 ^ 12bit
  return ret;
}
//Z軸
double  accele_Zval_get() {
  short val;
  double ret;
  val = (regRead(ADXL362_DATAZH) << 8) | regRead(ADXL362_DATAZL);
  ret = val * 0.00390625; // 0.00390625 = ±8g * 2 / 2 ^ 12bit
  return ret;
}

void setup() {

//LCD LEAF setup
  pinMode(2, INPUT);
  Wire.begin();
// IO Expander 初期化
 i2c_write_byte(I2C_EXPANDER_ADDR, 0x03, 0xFE);
 i2c_write_byte(I2C_EXPANDER_ADDR, 0x01, 0x01);
  
  lcd.begin(8, 2);
  lcd.setContrast(30);
  lcd.clear();

// First Message表示
  lcd.print(" Hello!");
  lcd.setCursor(0, 1);
  lcd.print(" wait!!");
  delay(1000);
  i2c_write_byte(I2C_EXPANDER_ADDR, 0x01, 0x00); 
  delay(3000);
  i2c_write_byte(I2C_EXPANDER_ADDR, 0x01, 0x01);
  lcd.begin(8, 2);
  lcd.setContrast(30);
  delay(100);

//ADI ACCELE LEAFを起ち上げる
  pinMode(PWR_EN,OUTPUT);
  digitalWrite(PWR_EN,LOW);
  delay(500);
  digitalWrite(PWR_EN,HIGH); //ADI ACCELE LEAF Power ON
  pinMode(VOUTGOOD,INPUT);
  delay(500);
  lcd.clear();
 //ADI ACCELE LEAF 電源確認
char  val;
  val = digitalRead(VOUTGOOD); //DC/DCコンバータのVOUTGOOD(POUTGOOD)読み込み
  lcd.setCursor(0, 0);
  lcd.print("ADI LEAF");
  if (val == 1) { 
  lcd.setCursor(0, 1);
    lcd.print("DRIVE OK"); //正常に立ち上がっている
  }else{
  lcd.setCursor(0, 1);
    lcd.print("DRIVE NG"); //正常に立ち上がっていない
ADI_DRIVE_NG:
    goto ADI_DRIVE_NG; //無限ループでこれ以上進まないようにする
  }

  delay(3000);
  lcd.clear();
  
//SPI setup
  pinMode(SS, OUTPUT); 
  digitalWrite(SS, HIGH);

  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV8); // 8MHz/8 = 1MHz
//SPI通信にてADXL362のID情報を読み込み、LCDに表示。
  lcd.setCursor(0, 0);
  lcd.print("DEV. ID");
  lcd.setCursor(0, 1);
  lcd.print(regRead(0x00), HEX);
  delay(500);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MEMS ID");
  lcd.setCursor(0, 1);
  lcd.print(regRead(0x01), HEX);
  delay(500);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PART ID");
  lcd.setCursor(0, 1);
  lcd.print(regRead(0x02), HEX);
  delay(500);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("REV. ID");
  lcd.setCursor(0, 1);
  lcd.print(regRead(0x03), HEX);
  delay(500);
  lcd.clear();

//ADXL362のレジスタ設定
  regWrite(ADXL362_FLT_CTL, 0x93); // +/-8g range, 100Hz (b10010011)
  regWrite(ADXL362_PWR_CTL, 0x02); // normal operation, measurement mode (b00000010)

//セットアップ完了。Mainプログラムへの準備ができたことをLCDに表示。
  lcd.setCursor(0, 0);
  lcd.print(" DEMO   ");
  lcd.setCursor(0, 1);
  lcd.print(" READY! ");
  
  delay(3000);
  lcd.clear();
}

void loop() {
double X, Y, Z;
int   i;

//Normal,Measurementモード,測定レンジ+/-8g,測定周波数100Hzにて連続測定
  regWrite(ADXL362_PWR_CTL, 0x02); // normal operation, measurement mode (b00000010)
  delay(100);
  i = 0;
  for (i=0 ; i<10 ;i++) {

    X = accele_Xval_get();
    Y = accele_Yval_get();
    Z = accele_Zval_get();
            
    lcd.setCursor(0, 0);
    lcd.print("X-Normal");
    lcd.setCursor(0, 1);
    lcd.print(X);
    delay(1000);
    lcd.setCursor(0, 0);
    lcd.print("Y");
    lcd.setCursor(0, 1);
    lcd.print(Y);
    delay(1000);
    lcd.setCursor(0, 0);
    lcd.print("Z");
    lcd.setCursor(0, 1);
    lcd.print(Z);
    delay(1000);
    lcd.clear();
  }
}
