# 🔒 Two-Man Rule Security System
**A Bare-Metal AVR C Implementation on the ATmega2560**

This project is a hardware-level implementation of a "Two-Man Rule" security protocol, designed to secure highly sensitive systems (e.g., vaults, server rooms) by requiring sequential, multi-factor authentication from two independent personnel. 

Built entirely in **bare-metal AVR C** (bypassing standard Arduino libraries), the system utilizes a custom **non-blocking Finite State Machine (FSM)**. By leveraging hardware interrupts (INT0) and internal timers (Timer0/Timer1) for background PWM and millisecond tracking, the CPU maintains strict real-time responsiveness without ever relying on blocking `delay()` functions.

### ⚙️ Core Architecture
* **Phase 1 (Guard 1):** 4x4 Matrix Keypad entry (Matrix Multiplexing)
* **Phase 2 (Guard 2):** Rotary Encoder combination dial (Quadrature Hardware Interrupts)
* **Actuation:** Servo-driven deadbolt (Timer1 Fast PWM)
* **Logic:** Strict sequential FSM with 15-second hardware timeouts

Base Diagram
(./demo-video-and-images/Base-Design.png)

System Locked (Default State)
(./demo-video-and-images/System-Locked.png)

Guard 1 is prompted to enter the keypad pin
(./demo-video-and-images/Guard-1-Input-via-Keypad.png)

Guard 2 is prompted to enter the dial pin
(./demo-video-and-images/Guard-2-Input-via-Dial.png)

Rack is unlocked after both guards entered correct pins
(./demo-video-and-images/Rack-Unlocked.png)

Security breach happened after a guard entered the wrong pin
(./demo-video-and-images/Security-Breach.png)

Pin was not entered within 15 second time limit
(./demo-video-and-images/Timeout.png)

Video demo showcasing how it works:

https://github.com/user-attachments/assets/3c24c7af-2723-46ba-9efd-6db59bd83a3b


