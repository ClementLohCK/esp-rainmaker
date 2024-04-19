# Building-Health Monitoring using Internet of Things (IoT)

## What this project is about

Currently, the maintenance of buildings is a manual process.

This project aims to develop a self-monitoring solution of the structural integrity of buildings, through the use of Internet of Things (IoT).

The entire project consists of 4 nodes connected on a common network. The sensors are -- (1) Buzzer w Flashing LED, (2) Tilt Sensor, (3) Rain Sensor, (4) Flex Sensor.

This repository will cover (1) Buzzer w Flashing LED, (3) Rain Sensor, (4) Flex Sensor.

It uses the ESP32-C6 board, which supports up to WiFi6 in 2.4GHsz band. For Serial Communications, I2C is used.

## Build, Flash firmware, and Visualize

1. Wire the components according to our report ./Report-Wireless_Health_Monitoring_of_Buildings.pdf
2. The different pin numbers can be changed in app_priv.h for each of the respective sensors.
3. Connect your device with a USB-Type C Cable. Choose the Right COM Port. Build the Project. Flash it unto your ESP Device.
4. Once Flashed, Open Serial Monitor, and use the ESP Rainmaker App to scan the QR Code. This will perform provisioning of WiFi.
5. Repeat for all of the nodes.
6. Once Provisioned, you will see 3 devices on your ESP Rainmaker App, one for each sensor.

## What to expect here?

- This project uses a buzzer and the inbuilt RGB LED as an alarm. Also uses a HW-028 rain sensor to detect amount of water, as well as flex sensor to detect amount of bending.
- The sensor value is sent every REPORTING_PERIOD number of seconds. Settings can be changed in app_priv.h.
- Each node will consistently send analog data of its sensor wirelessly, to be visualized in ESP Rainmaker App.
- You can check the state of the alarm, precipitation percent and amount of bending in the ESP Rainmaker phone app.

## Hardware

This project is designed for (1) Buzzer with Flashing LED, (3) Rain Sensor, (4) Flex Sensor, to be indepedently housed with its portable power source, 9V Battery, Bulk Converter and a Junction Box Casing.

These components can be changed to suit the design needs.

## Reset to Factory

Press and hold the BOOT button for more than 3 seconds to reset the board to factory defaults. You will have to provision the board again to use it.