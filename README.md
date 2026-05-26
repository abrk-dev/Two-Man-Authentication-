# 🔒 Two-Man Rule Security System
**A Bare-Metal AVR C Implementation on the ATmega2560**

This project is a hardware-level implementation of a "Two-Man Rule" security protocol, designed to secure highly sensitive systems (e.g., vaults, server rooms) by requiring sequential, multi-factor authentication from two independent personnel. 

Built entirely in **bare-metal AVR C** (bypassing standard Arduino libraries), the system utilizes a custom **non-blocking Finite State Machine (FSM)**. By leveraging hardware interrupts (INT0) and internal timers (Timer0/Timer1) for background PWM and millisecond tracking, the CPU maintains strict real-time responsiveness without ever relying on blocking `delay()` functions.

### ⚙️ Core Architecture
* **Phase 1 (Guard 1):** 4x4 Matrix Keypad entry (Matrix Multiplexing)
* **Phase 2 (Guard 2):** Rotary Encoder combination dial (Quadrature Hardware Interrupts)
* **Actuation:** Servo-driven deadbolt (Timer1 Fast PWM)
* **Logic:** Strict sequential FSM with 15-second hardware timeouts

[![Test in Wokwi](https://img.shields.io/badge/Wokwi-Simulate_Project-blue?style=for-the-badge&logo=wokwi)](https://wokwi.com/projects/461112925248720897)

Base Diagram

<img width="880" height="440" alt="Base-Design" src="https://github.com/user-attachments/assets/d1f13805-fbb9-4863-bf9d-8001f62fc40c" />



System Locked (Default State)

<img width="931" height="473" alt="System-Locked" src="https://github.com/user-attachments/assets/5b9164f0-8ba5-4171-96b9-3a3a77182e06" />



Guard 1 is prompted to enter the keypad pin

<img width="938" height="479" alt="Guard-1-Input-via-Keypad" src="https://github.com/user-attachments/assets/ea48f367-0dcb-4f7a-803f-fc7f554eb22c" />



Guard 2 is prompted to enter the dial pin

<img width="946" height="471" alt="Guard-2-Input-via-Dial" src="https://github.com/user-attachments/assets/3a450b61-858c-4c26-a9b3-66e332268245" />



Rack is unlocked after both guards entered correct pins

<img width="923" height="474" alt="Rack-Unlocked" src="https://github.com/user-attachments/assets/4f0f7d9f-dade-4578-825d-8765851b5847" />



Security breach warning after a guard entered the wrong pin

<img width="941" height="456" alt="Security-Breach" src="https://github.com/user-attachments/assets/594c2e2d-4b61-4d7c-be8a-e8a13edd1ed0" />



Pin was not entered within 15 second time limit

<img width="939" height="486" alt="Timeout" src="https://github.com/user-attachments/assets/14b9ec2b-75df-4c30-84fe-e2b246e401b6" />



Video demo showcasing how it works:

https://github.com/user-attachments/assets/3c24c7af-2723-46ba-9efd-6db59bd83a3b


