#include <Wire.h>
#include <ICM20948_WE.h>

#define ICM20948_ADDR 0x68
ICM20948_WE myIMU = ICM20948_WE(ICM20948_ADDR);
float activity;

void setupActivitySensor(ICM20948_WE sensor){
  if(!myIMU.init()){
    Serial.println("ICM20948 does not respond");}
  else{
    Serial.println("ICM20948 is connected");}

  Serial.println("Position your ICM20948 flat and don't move it - calibrating...");
  delay(1000);
  myIMU.autoOffsets();
  Serial.println("Done!");

  myIMU.enableAcc(false);

  myIMU.setGyrRange(ICM20948_GYRO_RANGE_250);
  myIMU.setGyrDLPF(ICM20948_DLPF_6);
}

float getActivity(ICM20948_WE sensor){
  sensor.readSensor();
  xyzFloat gyr = sensor.getGyrValues();
  return pow((pow(gyr.x, 2)+pow(gyr.y, 2)+pow(gyr.z, 2)) , 0.5);
}

void setup() {
  Wire.begin();
  Serial.begin(115200);
  while(!Serial) {}
  setupActivitySensor(myIMU);
}

void loop() {
  activity = getActivity(myIMU);
  Serial.println(activity);
  delay(100);
}
