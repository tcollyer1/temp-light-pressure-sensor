# Embedded C++ Temperature, Light & Pressure Sensor and Monitoring System
This is a system created in Mbed Studio with C++, using a [NUCLEO-F429ZI](https://os.mbed.com/platforms/ST-Nucleo-F429ZI/) development board with a small microcontroller connected to it. This was a piece of university coursework, with the purpose of being an **environmental sensor** designed for the continuous monitoring and tracking of temperature, light and pressure levels, with internet connectivity via Microsoft Azure.

## About the Hardware
The Nucleo board used features a [STM32F429ZI](https://www.st.com/en/microcontrollers-microprocessors/stm32f429zi.html) microcontroller, based on an Arm Cortex-M4 processor. The board 

## Project Features
- The program periodically samples and buffers sensor data, using a FIFO buffer, at a fixed rate of every **10 seconds** and additionally writes buffer data to an inserted SD card each minute for sending to Azure
- The program is **multi-threaded**, including producer and consumer threads for reading from/writing to the buffer containing the sensor data (during sample collection/SD card writes), a thread to listen for user input to prevent logged system error messages from displaying, and threads for handling sending sensor data to Azure
- Use of classes and templates with **virtual functions** to demonstrate consideration for reusability and avoiding platform-specific functionality
- Error handling and communication of these errors to the user (including console messages and triggering LED lights) in cases such as the buffer being full, or pressure/light/temperature readings being outside accepted thresholds
- Remote modification of acceptable temperature/pressure/light levels via remote function calls from Azure IoT centre