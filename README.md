# Spatial Mapping Using Time-of-Flight (2DX3 Project)

Embedded spatial mapping system using a **Time-of-Flight (ToF) sensor and stepper motor** to reconstruct a 3-D representation of an indoor environment.

This project was developed for **McMaster University's 2DX3 Embedded Systems course** and demonstrates the integration of **embedded control, sensor acquisition, and data visualization**.

The system rotates a ToF sensor through **360°** using a stepper motor to capture **distance measurements in a vertical plane**, while the device is manually displaced along another axis to construct a **3-D spatial map**.

---

# Project Overview

The goal of the system is to create a **low-cost alternative to LiDAR-style spatial mapping systems** by integrating:

- Microcontroller-based control
- Stepper motor rotation
- Time-of-Flight distance sensing
- Data acquisition and storage
- Serial communication with a PC
- Visualization of the scanned environment

Distance samples are collected across a **360° sweep**, producing a **2-D slice of the environment**, and repeated measurements at different positions allow reconstruction of a **3-D model**.

---

# Hardware

## Microcontroller
- **TI MSP432E401Y / TM4C1294-based LaunchPad**
- Used for:
  - GPIO
  - I²C communication
  - UART transmission
  - Stepper motor control

## Sensor
- **VL53L1X Time-of-Flight sensor**
- Measures distance using laser time-of-flight technology
- Communicates with the MCU via **I²C**

## Motor System
- **28BYJ-48 stepper motor**
- Driven via **ULN2003 driver**
- Rotates the sensor for **360° spatial scans**

## Inputs

Four pushbuttons control the system:

| Button | Function |
|------|------|
| Button 0 | Start / Stop motor |
| Button 1 | Change rotation direction |
| Button 2 | Toggle measurement angle (11.25° or 45°) |
| Button 3 | Return to home position |

## Status LEDs

| LED | Function |
|----|----|
| LED0 | Motor running status |
| LED1 | Direction indicator |
| LED2 | Angle mode indicator |
| LED3 | Step indicator |

---

# System Architecture

The system follows a typical **embedded data acquisition pipeline**:

Sensor (ToF)
↓
I²C Communication
↓Flashing is done through the **MSP432 debug interface**.

---

# Visualization

Distance data will be transmitted over **UART** to a PC application where it can be:

- Displayed in a terminal
- Parsed by a Python script
- Converted into a **3-D point cloud**

Potential tools:

- Python + Matplotlib
- MATLAB
- 3-D viewer software

---

# Learning Objectives

This project demonstrates key embedded systems concepts:

- Embedded C development
- Microcontroller peripheral configuration
- Stepper motor control
- Sensor interfacing (I²C)
- Serial communication (UART)
- Data acquisition and visualization
- System integration and debugging

---

# Future Improvements

Possible extensions include:

- Continuous linear translation (automatic 3-D scanning)
- Real-time visualization
- Multi-sensor scanning
- SLAM-style mapping
- Integration with robotics navigation systems
Microcontroller Processing
↓
Distance Storage
↓
UART Transmission
↓
PC Visualization

The microcontroller performs:

- Motor control
- Sensor communication
- Distance sampling
- Serial transmission to the PC

---

# Deliverable 1 – Early Integration

Status: **Completed**

Deliverable 1 focuses on **motor control and system interaction**.

### Features Implemented

- Start / Stop motor control
- Motor direction control
- Angle resolution toggle
  - 11.25°
  - 45°
- Step indicator LED
- Homing function
- Automatic stop after one full rotation

### Concepts Demonstrated

- GPIO configuration
- Pushbutton input handling
- LED status outputs
- Stepper motor control
- State-machine design

---

# Deliverable 2 – Final System (In Progress)

The final system integrates **distance sensing and 3-D mapping**.

### Planned Features

- VL53L1X Time-of-Flight integration
- Distance measurement during rotation
- I²C communication with sensor
- Data storage in MCU memory
- UART transmission to PC
- Python / MATLAB visualization
- 3-D reconstruction of scanned environment

The final demonstration will include scanning a **real indoor location and generating a spatial model**.

---

# Build and Flash

Compile using **ARM GCC**.

Example:

make
./flash.sh

Flashing is done through the **MSP432 debug interface**.

---

# Visualization

Distance data will be transmitted over **UART** to a PC application where it can be:

- Displayed in a terminal
- Parsed by a Python script
- Converted into a **3-D point cloud**

Potential tools:

- Python + Matplotlib
- MATLAB
- 3-D viewer software

---

# Learning Objectives

This project demonstrates key embedded systems concepts:

- Embedded C development
- Microcontroller peripheral configuration
- Stepper motor control
- Sensor interfacing (I²C)
- Serial communication (UART)
- Data acquisition and visualization
- System integration and debugging

---

# Future Improvements

Possible extensions include:

- Continuous linear translation (automatic 3-D scanning)
- Real-time visualization
- Multi-sensor scanning
- SLAM-style mapping
- Integration with robotics navigation systems
