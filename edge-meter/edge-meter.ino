#define NANO_V1 //NANO_V2

#include <Arduino_APDS9960.h>
#include <Arduino_LPS22HB.h>

#ifdef NANO_V1
#include <Arduino_HTS221.h>
#include <Arduino_LSM9DS1.h>
#endif

#ifdef NANO_V2
#include <Arduino_HS300x.h>
#include <Arduino_BMI270_BMM150.h>
#endif 

#include <MadgwickAHRS.h>
#include <ArduinoBLE.h>

String name = "Mesurify-Meter";

Madgwick filter;

const char* service_uuid = "8e7c2dae-0000-4b0d-b516-f525649c49ca";
const char* sampling_period_uuid = "8e7c2dae-0001-4b0d-b516-f525649c49ca";
const char* imu_uuid = "8e7c2dae-0002-4b0d-b516-f525649c49ca";
const char* environment_uuid = "8e7c2dae-0003-4b0d-b516-f525649c49ca";
const char* orientation_uuid = "8e7c2dae-0004-4b0d-b516-f525649c49ca";

BLEService service(service_uuid);

// Loop delay
int sampling_period = 250;
int heartbit_period = 5000;
unsigned long sampling_previousMillis = 0;
unsigned long heartbit_previousMillis = 0;
BLECharacteristic samplingPeriodCharacteristic(sampling_period_uuid,  BLEWrite | BLERead, sizeof(int));

// IMU: 9 floats G, degress per second, uT
float acceleration[3];
float angular_speed[3];
float magnetic_field[3];
BLECharacteristic imuCharacteristic(imu_uuid, BLENotify, 9 * sizeof(float)); 

// Environment: proximity, temperature, humidity, pressure, light, r, g, b  
int proximity; 
float temperature; 
float humidity;
float pressure;
int light;
int red, green, blue;
BLECharacteristic environmentCharacteristic(environment_uuid, BLENotify, 3 * sizeof(float) + 5 * sizeof(int));

// Orientation: 3 floats degress
float heading;
float pitch;
float roll;
BLECharacteristic orientationCharacteristic(orientation_uuid, BLENotify, 3 * sizeof(float)); 

void init_sensors(void){
  while(!APDS.begin()) { Serial.println("Error, APDS"); delay(500); };

  #ifdef NANO_V1
  while(!HTS.begin()) { Serial.println("Error, HTS"); delay(500); };
  #endif
  
  #ifdef NANO_V2
  while(!HS300x.begin()) { Serial.println("Error, HTS"); delay(500); };
  #endif
  
  while(!BARO.begin()) { Serial.println("Error, BARO"); delay(500); };
  while(!IMU.begin()) { Serial.println("Error, IMU"); delay(500); };
}

void init_BLE(){
  while(!BLE.begin()) { Serial.println("Error, BLE"); delay(500); };

  String address = BLE.address();

  BLE.setLocalName(name.c_str());
  BLE.setDeviceName(name.c_str());
  BLE.setAdvertisedService(service);
  
  service.addCharacteristic(imuCharacteristic);
  service.addCharacteristic(environmentCharacteristic);
  service.addCharacteristic(orientationCharacteristic);

  samplingPeriodCharacteristic.setEventHandler(BLEWritten, onSamplingPeriodCharacteristicWrite);
  service.addCharacteristic(samplingPeriodCharacteristic);
  
  BLE.addService(service);
  BLE.advertise();
}
void initFilter(){
  filter.begin(1000/sampling_period);
}

void setup() {
  Serial.begin(9600);

  Serial.println("Init sensors...");  
  init_sensors();
  
  Serial.println("Init BLE...");  
  init_BLE();

  Serial.println("Init Filter...");  
  initFilter();

  Serial.println("Start loop...");  
}

void manageRawValues() {
  if (IMU.accelerationAvailable()) {
    float x, y, z;
    IMU.readAcceleration(x, y, z);
    acceleration[0] = x;
    acceleration[1] = y;
    acceleration[2] = z;
  }
  
  if (IMU.gyroscopeAvailable()) {
    float x, y, z;
    IMU.readGyroscope(x, y, z);
    angular_speed[0] = x;
    angular_speed[1] = y;
    angular_speed[2] = z;
  }

  if (IMU.magneticFieldAvailable()) {
    float x, y, z;
    IMU.readMagneticField(x, y, z);
    magnetic_field[0] = x;
    magnetic_field[1] = y;
    magnetic_field[2] = z;
  }
}

void manageIMU() {
  float imu[9] = { acceleration[0], acceleration[1], acceleration[2], 
                   angular_speed[0], angular_speed[1], angular_speed[2],
                   magnetic_field[0], magnetic_field[1], magnetic_field[2] 
                 };
  imuCharacteristic.writeValue(imu, sizeof(imu));
} 

void manageOrientation(){
  filter.update(angular_speed[0], angular_speed[1], angular_speed[2],
                acceleration[0], acceleration[1], acceleration[2],
                magnetic_field[0], magnetic_field[1], magnetic_field[2]);

  heading = filter.getYawRadians();
  pitch = filter.getPitchRadians();
  roll = filter.getRollRadians();

  float orientation[3] = { heading, pitch, roll };
  orientationCharacteristic.writeValue(orientation, sizeof(orientation));
}

void manageEnvironment() {
  if (APDS.proximityAvailable()) { proximity = APDS.readProximity(); }

  #ifdef NANO_V1
  temperature = HTS.readTemperature(); 
  humidity = HTS.readHumidity(); 
  #endif

  #ifdef NANO_V2
  temperature = HS300x.readTemperature();
  humidity = HS300x.readHumidity();
  #endif

  pressure = BARO.readPressure();
  if (APDS.colorAvailable()) { APDS.readColor(red, green, blue, light); }

  float environment[8] = { proximity, temperature, humidity, pressure, light, red, green, blue };
  environmentCharacteristic.writeValue(environment, sizeof(environment));
} 

void onSamplingPeriodCharacteristicWrite(BLEDevice central, BLECharacteristic characteristic) {
  sampling_period = word(samplingPeriodCharacteristic[0], samplingPeriodCharacteristic[1]);
  Serial.print("Sampling period change: "); 
  Serial.print(sampling_period);
  Serial.println(" ms"); 
}

void heartbit() {
  if (millis() - heartbit_previousMillis >= heartbit_period) {
      if (BLE.connected()) {
        Serial.print("BLE client connected (");
        Serial.print(BLE.address());
        Serial.println(")...");
        Serial.print(" - Sampling period: ");
        Serial.println(sampling_period);
        if (imuCharacteristic.subscribed()) Serial.println(" - IMU subscribed");
        if (environmentCharacteristic.subscribed()) Serial.println(" - Environment subscribed");
        if (orientationCharacteristic.subscribed()) Serial.println(" - Orientation subscribed");
      }
      else {
        Serial.println("No BLE client connected...");
      }
      heartbit_previousMillis = millis();
    }
}

void loop() {
  heartbit();
  if (BLE.connected()) {
    if (millis() - sampling_previousMillis >= sampling_period) {
      if (imuCharacteristic.subscribed() || orientationCharacteristic.subscribed()) { manageRawValues(); }
      if (imuCharacteristic.subscribed()) { manageIMU(); }
      if (environmentCharacteristic.subscribed()) { manageEnvironment(); }
      if (orientationCharacteristic.subscribed()) { manageOrientation(); }
      sampling_previousMillis = millis();
    }
  } 
}
