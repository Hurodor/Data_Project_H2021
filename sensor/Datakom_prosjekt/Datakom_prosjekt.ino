#include "Wire.h"
#include "math.h"

const int MPU_ADDR = 0x68;
int16_t temp;
int16_t accl_x, accl_y, accl_z;

uint16_t activity;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
}

void loop() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 4*2, true);

  accl_x = Wire.read()<<8 | Wire.read();
  accl_y = Wire.read()<<8 | Wire.read();
  accl_z = Wire.read()<<8 | Wire.read();
  activity = getActivity(accl_x, accl_y, accl_z);
  Serial.print("Activity: ");
  Serial.print(activity);

  temp = Wire.read()<<8 | Wire.read();
  temp = temp/340 + 36.53;
  Serial.print(" | Temperature: ");
  Serial.println(temp);
  delay(100);
}

uint16_t getActivity(int16_t x, int16_t y, int16_t z) {
  double act = sqrt(pow((double) x, 2) + pow((double) y, 2) + pow((double) z, 2));
  return (uint16_t) act;
}
