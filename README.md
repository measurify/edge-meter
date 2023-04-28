# Measurify Edge Meter

Edge Measurer is a board based on the [Arduino Nano BLE Sense](https://store.arduino.cc/products/nano-33-ble-sense-rev2?gclid=CjwKCAjwuqiiBhBtEiwATgvixP9YZH3kv_W2rU_MNzxRqpEtBDAPsVifqHgtA1YfRTseUCstFRrMhBoCzdEQAvD_BwE). It acquires values from sensors on the Arduino device and exposes collected data using the on-board BLE connectivity. It provide IMU values (accelerometer, gyroscope and magnetometer) and environmental values (proximity, temperature, humidity, pressure and ambient light) as BLE characteristics. The sampling period can be decided by the connected client, the default value is 250 msec.

The BLE local name is **Mesurify-Meter** and it exposes a single BLE service (UUID: 8e7c2dae-0000-4b0d-b516-f525649c49ca) featuring the following characteristics:

**IMU Characteristic**
- UUID: 8e7c2dae-0001-4b0d-b516-f525649c49ca
- Properties: notify
- Data format: array of 9 float
- Values: acceleration [G], angular velocity [dps], and magnetic field [uT]

**Environment Characteristic**
- UUID: 8e7c2dae-0002-4b0d-b516-f525649c49ca
- Properties: notify
- Data format: 3 float and 5 int
- Values: proximity [0 close, 255 far], temperature [C], humidity [%], pressure [kPA], ambient light [0 dark] and color [RGB]

**Sampling Period Characteristic**
- UUID: 8e7c2dae-0003-4b0d-b516-f525649c49ca
- Properties: read and write
- Data format: 16-bit unsigned integer
- Values: sampling period in msec
