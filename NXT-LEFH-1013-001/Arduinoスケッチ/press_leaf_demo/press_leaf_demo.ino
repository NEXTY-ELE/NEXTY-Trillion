//=====================================================================
//  Trillion Node Engine
//     Platform     : IFX PRESSURE SENSOR LEAF
//     Processor    : ATmega328P (3.3V /8MHz)
//     Application  : test for IFX Pressure sensor
//
//     Leaf configuration
//       (1) 8bit-MCU Leaf
//       (2) IFX PRESSURE SENSOR LEAF
//       (3) LCD LEAF 2018
//       (4) BAT(CR2032) LEAF or BAT(CR2450) LEAF
//=====================================================================
// [Operation]
// (1)Display barometric pressure data on the LCD. 
// (2)Display temperature data on LCD. 
//  Repeat (1) and (2). 
//=====================================================================
#include <Dps310.h>
#include <Wire.h>
#include <ST7032.h>

Dps310 Dps310PressureSensor = Dps310();
ST7032 lcd;

#define DPS310_address 0x77 /*DPS310 I2C address*/
#define PRD_ID 0x0D
#define COEF_SRCE 0x28
#define LCD_address   0x1A



void setup()
{
/*LCD LEAF setup*/
  pinMode(2, INPUT);
  Wire.begin();
    
/*IO Expander Initialization*/ 
  Wire.beginTransmission(LCD_address);
  Wire.write(0x03);
  Wire.write(0xFE);
  Wire.endTransmission();

  Wire.beginTransmission(LCD_address);
  Wire.write(0x01);
  Wire.write(0x01);
  Wire.endTransmission();
 
  lcd.begin(8, 2);
  lcd.setContrast(30);
  lcd.clear();

/*First Message*/
  lcd.setCursor(0, 0);
  lcd.print(" Hello!");
  lcd.setCursor(0, 1);
  lcd.print(" wait!!");
  delay(1000);
  Wire.beginTransmission(LCD_address);
  Wire.write(0x01);
  Wire.write(0x00);
  Wire.endTransmission();
  delay(3000);
  
  Wire.beginTransmission(LCD_address);
  Wire.write(0x01);
  Wire.write(0x01);
  Wire.endTransmission();
  lcd.begin(8, 2);
  lcd.setContrast(30);
  delay(100);

  Serial.begin(9600);
  while (!Serial);


/* Call begin to initialize Dps310PressureSensor
   The parameter 0x76 is the bus address. The default address is 0x77 and does not need to be given.
   Dps310PressureSensor.begin(Wire, 0x76);
   Use the commented line below instead of the one above to use the default I2C address.
   if you are using the Pressure 3 click Board, you need 0x76 */

  Dps310PressureSensor.begin(Wire);

  Serial.println("Init complete!");

  int read_data, tmp00, val;
  Wire.beginTransmission(DPS310_address);
  Wire.write(PRD_ID);
  Wire.endTransmission();
  Wire.requestFrom(DPS310_address,1);
  read_data = Wire.read();
  delay(100);
  Serial.print("IDreg 0d");
  Serial.println(read_data);
  tmp00 = (read_data >> 4) & 0x0F;
  Serial.print("REV_ID ");Serial.println(tmp00);
  tmp00 = read_data & 0x0F;
  Serial.print("PROD_ID ");Serial.println(tmp00);
  delay(100);
  Wire.beginTransmission(DPS310_address);
  Wire.write(COEF_SRCE);
  Wire.endTransmission();
  Wire.requestFrom(DPS310_address,1);
  read_data = Wire.read();
  tmp00 = (read_data >> 7) & 0x01;
  
  if (tmp00 == 0){
    Serial.println("Internal temperature sensor ASIC");
  }else{
    Serial.println("External temperature sensor MEMS");
  }

}



void loop()
{
  float temperature;
  float pressure;
  uint8_t oversampling = 7;
  int16_t ret;
  Serial.println();
  lcd.clear();

/*lets the Dps310 perform a Single temperature measurement with the last (or standard) configuration
  The result will be written to the paramerter temperature
  ret = Dps310PressureSensor.measureTempOnce(temperature);
  the commented line below does exactly the same as the one above, but you can also config the precision
  oversampling can be a value from 0 to 7
  the Dps 310 will perform 2^oversampling internal temperature measurements and combine them to one result with higher precision
  measurements with higher precision take more time, consult datasheet for more information */

  ret = Dps310PressureSensor.measureTempOnce(temperature, oversampling);

  lcd.setCursor(0, 0);
  lcd.print("Temp[dC]");
  lcd.setCursor(0, 1);
  
  if (ret != 0)
  {
    /*Something went wrong.
      Look at the library code for more information about return codes*/
    Serial.print("FAIL! ret = ");
    Serial.println(ret);
    lcd.print(" FAIL!  ");
  }
  else
  {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" [degC]");
    lcd.print(temperature);
  }

    delay(1000);

/* Pressure measurement behaves like temperature measurement */
  //ret = Dps310PressureSensor.measurePressureOnce(pressure);
  ret = Dps310PressureSensor.measurePressureOnce(pressure, oversampling);

  lcd.setCursor(0, 0);
  lcd.print("Prs[hPa]");
  lcd.setCursor(0, 1);
  
if (ret != 0)
  {
    /*Something went wrong.
      Look at the library code for more information about return codes */
    Serial.print("FAIL! ret = ");
    Serial.println(ret);
    lcd.print(" FAIL!  ");
  }
  else
  {
    Serial.print("Pressure: ");
    Serial.print(pressure/100);
    Serial.println(" [hPa]");
    lcd.print(pressure/100);
  }

    delay(1000);

  /*Wait some time*/
  delay(500);
}
