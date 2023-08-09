#include <Wire.h>

/* 
 * The sensor supports 100KHz and 400KHz. 
 * You hardware setup and pull-ups value will
 * also influence the i2c operation. You can 
 * change this value to 100000 in case of 
 * communication issues.
 */
#define I2C_FREQ_HZ  400000

// 測定間隔(秒) 5~4095を指定すること
#define PERIODIC_MEAS_INTERVAL_IN_SECONDS  5

// キャリブレーション設定
#define CALIB_ABOC_DISABLE  0   // ABOC無効
#define CALIB_ABOC_ENABLE   1   // ABOC有効
#define CALIB_FORCED        2   // 強制補正

#define CALIB_SETTING       CALIB_ABOC_DISABLE  // キャリブレーション設定を指定する

#define I2C_SEND_BUF_LENGTH 10
#define I2C_RECV_BUF_LENGTH 10

uint8_t i2c_sendBuf[I2C_SEND_BUF_LENGTH];
uint8_t i2c_recvBuf[I2C_RECV_BUF_LENGTH];
bool co2_int = false;
bool first_meas = true;

const uint8_t EN3_3V = A2;       // A2
const uint8_t EN12V = A1;        // A1
const uint8_t interruptPin = 2; // D2
const uint8_t CO2_ADDRESS = 0x28u;

/* CO2センサ割り込みハンドラ */
void co2_isr(void){
  noInterrupts();
  Serial.println("CO2 interrupt");

  // 最初の割込みは0ppmしか取れないので、2回目以降に測定結果を取得する
  if(first_meas == true){
    first_meas = false;
  }else{
    co2_int = true;
  }

}

void setup() {
  Serial.begin(115200);
  delay(800);
  Serial.println("serial initialized");

  /* Initialize the i2c interface used by the sensor */
  Wire.begin();
  Wire.setClock(I2C_FREQ_HZ);

  /* ピン設定 */
  pinMode(EN3_3V, OUTPUT);  // EN3_3V
  pinMode(EN12V, OUTPUT);  // EN12V
  pinMode(A0, INPUT);
  pinMode(interruptPin, INPUT); // CO2センサ割込み
  digitalWrite(EN3_3V, LOW);
  digitalWrite(EN12V, LOW);
  
  /* CO2リーフ設定 */
  digitalWrite(EN3_3V, HIGH); // 3.3V ON
  delay(1000);  // sensor_rdy 1sec
  uint8_t id = i2c_read_byte(0x00); // CO2センサID読み込み
  Serial.print("Sensor ID: ");
  Serial.println(id, HEX);
  uint8_t status = i2c_read_byte(0x01); // ステータスレジスタ読み込み
  Serial.print("CO2 STATUS: 0x");
  Serial.println(status, HEX);

  // Idle mode
  i2c_write_byte(0x04, 0x00);
  delay(400);

  /* レジスタ設定0x02~0x04 */
  i2c_sendBuf[0] = (PERIODIC_MEAS_INTERVAL_IN_SECONDS >> 8);  // MEAS_RATE_H
  i2c_sendBuf[1] = (uint8_t)(PERIODIC_MEAS_INTERVAL_IN_SECONDS & 0x00ff);  // MEAS_RATE_L  測定間隔(sec)
  i2c_sendBuf[2] = (CALIB_SETTING << 2) | 0x02;  // MEAS_CFG     Continuous mode
  i2c_write(0x02, 3, i2c_sendBuf);

  delay(1000);  // wait for early notification signal

  /* レジスタアドレス設定0x08 */
  i2c_write_byte(0x08, 0x14);   // INT_CFG  Data ready時にINTを発生

  /* 設定レジスタ値 */
  clearI2CReadbuf();
  i2c_read(0x02, 3, i2c_recvBuf);
  for(int i=0; i<3;i++){
    Serial.print(" [reg 0x");
    Serial.print(i+2, HEX);
    Serial.print("]: 0x");
    Serial.print(i2c_recvBuf[i], HEX);
  }
  Serial.println();

  clearI2CReadbuf();
  i2c_read(0x08, 7, i2c_recvBuf);
  for(int i=0; i<7;i++){
    Serial.print(" [reg 0x");
    Serial.print(i+8, HEX);
    Serial.print("]: 0x");
    Serial.print(i2c_recvBuf[i], HEX);
  }
  Serial.println();

  delay(1);

  /* 測定開始 */
  digitalWrite(EN12V, HIGH); // 12V ON
  delay(1000);

  /* 割込み設定 */
  attachInterrupt(digitalPinToInterrupt(interruptPin), co2_isr, FALLING);
  interrupts();

}

void loop() {

  if(co2_int==true){
    clearI2CReadbuf();
    // read CO2PPM_H, CO2PPM_L, MEAS_STS
    i2c_read(0x05, 3, i2c_recvBuf);
    uint16_t co2_result = ((uint16_t)i2c_recvBuf[0] << 8) | (uint16_t)i2c_recvBuf[1];
    Serial.print("CO2(ppm): ");
    Serial.print(co2_result);
    Serial.print("  STATUS: 0x");
    Serial.println(i2c_recvBuf[2], HEX);

    // clear INT pin status & alarm notification
    i2c_write_byte(0x07, 0x03);  // MEAS_STS
    co2_int = false;

    interrupts();
  }

}

/**********************************************
* I2C Write 1 byte to the slave device
**********************************************/
bool i2c_write_byte(int reg_address, int write_data){
  Wire.beginTransmission(CO2_ADDRESS);
  Wire.write(reg_address);
  Wire.write(write_data);
  if (Wire.endTransmission() != 0)
  {
    return false;
  }
  else
  {
    return true;
  }
}


/**********************************************
* I2C Read 1 byte from the slave device
**********************************************/
unsigned char i2c_read_byte(uint8_t reg_address)
{
  Wire.beginTransmission(CO2_ADDRESS);
  Wire.write(reg_address);
  Wire.endTransmission(false);
  //request 1 byte from slave
  if(Wire.requestFrom(CO2_ADDRESS, 1U, 1U))
  {
    return Wire.read();
  }
  else
  {
    return 0;
  }
}


/**********************************************
* I2C Write multiple bytes to the slave device
**********************************************/
void i2c_write(int reg_address, int length, unsigned char* write_byte){

  Wire.beginTransmission(CO2_ADDRESS);
  Wire.write(reg_address);
  for (int i = 0; i < length; i++){
    Wire.write(write_byte[i]);
  }
  Wire.endTransmission(true);
}


/**********************************************
* I2C Read multiple bytes from the slave device
**********************************************/
void i2c_read(uint8_t reg_address, uint8_t length, unsigned char* read_byte){

  Wire.beginTransmission(CO2_ADDRESS);
  Wire.write(reg_address);
  Wire.endTransmission(false);

  Wire.requestFrom(CO2_ADDRESS, length, (uint8_t)false);
  for (int i = 0; i < length; i++){
    read_byte[i] = Wire.read();
  }

  Wire.endTransmission(true);
}

/**********************************************
* I2C Receive buffer clear
**********************************************/
void clearI2CReadbuf(){
  memset(&i2c_recvBuf[0], 0x00, sizeof(i2c_recvBuf));
}
