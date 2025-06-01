# Attendance-Monitor

Attendance-Monitor is a microcontroller-based student attendance management system, designed and implemented for the ATmega32 platform. The project includes everything needed to simulate and demonstrate the system in Proteus, with precompiled binaries and example Proteus project files for quick setup.

## Project Overview

This system allows educators or administrators to:
- Record student attendance using a keypad and graphical LCD (GLCD).
- Store attendance data in EEPROM for persistence.
- Retrieve, search, and manage attendance records.
- Monitor temperature and basic classroom traffic using sensors.
- Interact with a menu-driven interface for different management tasks.

![image_2025-06-01_04-16-20](https://github.com/user-attachments/assets/3847aca8-517a-4c11-b703-3fcfa40abadc)

The project showcases embedded programming for real-world applications and demonstrates how microcontrollers can manage structured data and interact with multiple hardware modules.

## Key Technologies

### Microcontroller Platform
- **ATmega32**: 8-bit AVR RISC microcontroller, used as the core of the system.

### Core Libraries (AVR Toolchain)
- `<avr/io.h>`: Low-level device hardware access.
- `<avr/interrupt.h>`: Interrupt support for real-time and event-driven features.
- `<avr/eeprom.h>`: Built-in EEPROM read/write routines for persistent storage.
- `<util/delay.h>`, `<util/twi.h>`: Timing and TWI/I2C support.
- `<stdio.h>`, `<string.h>`, `<stdint.h>`: Standard C libraries for formatting, string handling, and portable data types.

### Custom Libraries & Drivers
- **GLCD Driver (KS0108)**
  - `KS0108.h`, `KS0108.c`, `KS0108_Settings.h`, `Font5x8.h`
  - Custom driver for controlling a KS0108-based graphical LCD.
  - Provides text and graphics output, font rendering, and memory management functions.
- **IO_Macros.h**: Abstraction of low-level digital read/write operations (used by GLCD driver).

### Hardware Modules & Peripherals
- **Graphical LCD (GLCD)**: Main user interface for menus and data display.
- **Keypad Matrix**: 4x3 keypad for user input and navigation.
- **EEPROM**: Used for storing student attendance records permanently.
- **USART (Serial Communication)**: For exporting attendance data and debugging.
- **Buzzer**: Audio feedback for user actions and system events.
- **Temperature Sensor (e.g., LM35 on ADC0)**: Monitors environmental temperature.
- **Ultrasonic Sensor**: Used for basic traffic (people) monitoring.
- **RTC (Real-Time Clock) via I2C/TWI**: Timekeeping for timestamps (code includes I2C/TWI setup and typical RTC address/definitions).

### System Features
- **Menu-Driven UI**: Modular software menus for attendance, management, monitoring, and data retrieval.
- **Persistent Storage**: EEPROM operations for saving/loading attendance.
- **Sensor Integration**: ADC for temperature, digital IO for ultrasonic and buzzer.
- **Real-Time Feedback**: GLCD updates and buzzer tones for immediate user feedback.
- **Student Management:** Search, view, and remove present students from the list.
- **Retrieve Data:** Export attendance data over USART for logging.
- 
### Simulation & Tooling
- **Proteus**: For simulating the full hardware system including microcontroller, GLCD, sensors, and user input.
- **Precompiled HEX Binaries**: For rapid simulation or hardware flashing.
- **AVR-GCC Toolchain**: The entire project is written in C and built for the AVR platform.

**Custom Library Highlight:**
- The project uses a custom GLCD library (`KS0108` driver) for low-level control of graphical LCDs compatible with the KS0108 chipset. This library is not standard and is essential for driving the visual feedback and UI of the system.

## Getting Started

### Requirements

- **Hardware (Simulation):**
  - Proteus (for simulation)
  - ATmega32 microcontroller (simulated in Proteus)
  - GLCD, Keypad, Buzzer, Temperature sensor, Ultrasonic sensor

- **Files Included:**
  - Source code for ATmega32 (in `/Src`)
  - Precompiled `.hex` binaries for immediate use
  - Proteus project file(s) to run the simulation

### Quick Simulation Guide

1. **Open the Proteus project file** provided in the repository.
2. **Update the Program Path:**
   - Double-click the ATmega32 chip in the Proteus schematic.
   - Click the '...' button next to the Program File field.
   - Browse and select the precompiled `.hex` file provided (`/bin` or relevant directory).
3. **Run the Simulation:**
   - Start the simulation in Proteus.
   - Interact using the keypad and observe output on the GLCD.
   - Use the menu to access attendance, student management, and monitoring features.

## File Structure

- `/Src`: C source code for ATmega32, including main logic, EEPROM operations, menu handling, and hardware abstraction.
- `/bin` or equivalent: Precompiled `.hex` firmware for quick flashing or simulation.
- `/Proteus`: Proteus project and schematic files for simulation.
- `README.md`: Project overview and instructions.

## How It Works (High-Level)

- The system starts with a menu on the GLCD, navigated via keypad.
- When recording attendance, students enter their unique ID numbers. These are validated, timestamped, and stored.
- Administrators can search for, view, and remove student entries.
- Attendance and additional sensor data can be retrieved or monitored in real time.
- All attendance records are saved in EEPROM, ensuring data is retained after resets or power loss.

## Customization

- To use your own compiled binaries, recompile the source in `/Src` and update the `.hex` file path in Proteus.
- The code is modular, allowing easy extension for more sensors or features.

## Credits

Developed and maintained by [Kebabist](https://github.com/Kebabist).
