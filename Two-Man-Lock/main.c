#define F_CPU 16000000UL // Tell the compiler the Mega runs at 16 MHz

#include <avr/io.h>        // Standard AVR register definitions
#include <avr/interrupt.h> // Required for ISRs and sei()
#include <util/delay.h>    // Built-in delay functions
#include <stdlib.h>        // Required for itoa()

// --- GLOBAL VARIABLES FOR ENCODER ---
volatile int dial_position = 0;   
volatile uint8_t dial_moved = 0;  // A flag to tell the main loop "I changed!"

// --- TIMEOUT CLOCK (TIMER0) ---
volatile uint32_t system_millis = 0; // Our custom millisecond counter

void timer0_init(void) {
    // 1. Configure Timer0 for CTC Mode (Clear Timer on Compare Match)
    TCCR0A = (1 << WGM01); 
    
    // 2. Set Prescaler to 64
    TCCR0B = (1 << CS01) | (1 << CS00);
    
    // 3. Set the target count for 1ms
    // 16MHz clock / 64 prescaler = 250,000 ticks per second. 
    // 250 ticks = 1 millisecond. (250 - 1 = 249)
    OCR0A = 249; 
    
    // 4. Enable the Compare Match A Interrupt
    TIMSK0 |= (1 << OCIE0A);
}

// Every 1ms, the CPU jumps here, adds 1 to our counter, and goes back to work
ISR(TIMER0_COMPA_vect) {
    system_millis++;
}

// --- ENCODER HARDWARE INTERRUPT DRIVER ---
void encoder_init(void) {
    // 1. Configure the External Interrupt Control Register A (EICRA)
    // We want INT0 (Pin 21) to trigger an interrupt on the FALLING EDGE
    EICRA |= (1 << ISC01);
    EICRA &= ~(1 << ISC00);

    // 2. Enable the INT0 interrupt in the Mask Register (EIMSK)
    EIMSK |= (1 << INT0);

    // 3. Enable Global Interrupts (Sets the 'I' bit in the Status Register)
    sei(); 
}

ISR(INT0_vect) {
    // INT0 (PD0) just went LOW. Check INT1 (PD1) for direction.
    if (PIND & (1 << PORTD1)) {
        dial_position++; // Clockwise
    } else {
        dial_position--; // Counter-Clockwise
    }
    dial_moved = 1; // Flag that a movement occurred
}

// --- BARE-METAL LCD DRIVER (PORTA) ---
void lcd_pulse(void) {
    PORTA |= (1 << PA1); // Enable HIGH
    _delay_us(1);        
    PORTA &= ~(1 << PA1); // Enable LOW
    _delay_us(100);      
}

void lcd_write(uint8_t data, uint8_t is_data) {
    PORTA &= 0xC0; // Clear RS, E, and D4-D7
    
    if (is_data) {
        PORTA |= (1 << PA0); // RS = 1 for text
    }

    PORTA |= ((data >> 2) & 0x3C); // Send HIGH nibble
    lcd_pulse();

    PORTA &= ~(0x3C);              // Clear data pins
    PORTA |= ((data << 2) & 0x3C); // Send LOW nibble
    lcd_pulse();
    
    if (!is_data) _delay_ms(2); 
}

void lcd_init(void) {
    _delay_ms(50); 
    PORTA &= 0xC0;
    PORTA |= (0x03 << 2); lcd_pulse(); _delay_ms(5);
    lcd_pulse(); _delay_us(150);
    lcd_pulse();
    
    PORTA &= 0xC0;
    PORTA |= (0x02 << 2); lcd_pulse(); // Set 4-bit mode
    
    lcd_write(0x28, 0); // 4-bit, 2 lines, 5x8 font
    lcd_write(0x0C, 0); // Display ON, Cursor OFF
    lcd_write(0x01, 0); // Clear display
    lcd_write(0x06, 0); // Auto-increment cursor
}

void lcd_print(const char* str) {
    while (*str) {
        lcd_write(*str++, 1);
    }
}

void lcd_clear(void) {
    lcd_write(0x01, 0); 
}

// --- BARE-METAL KEYPAD DRIVER (PORTC) ---
const char key_map[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

char scan_keypad() {
    for (uint8_t row = 0; row < 4; row++) {
        PORTC = ~(1 << row) & 0x0F; // Drive one row LOW
        PORTC |= 0xF0;              // Keep pull-ups on columns
        _delay_ms(5);               // Debounce
        
        uint8_t cols = PINC & 0xF0; 
        
        if (cols != 0xF0) { // If a button is pressed
            while((PINC & 0xF0) != 0xF0); // Wait for release
            
            if (!(cols & (1 << PC4))) return key_map[row][0];
            if (!(cols & (1 << PC5))) return key_map[row][1];
            if (!(cols & (1 << PC6))) return key_map[row][2];
            if (!(cols & (1 << PC7))) return key_map[row][3];
        }
    }
    return '\0'; 
}

// --- FINITE STATE MACHINE SYSTEM ---
typedef enum {
    STATE_IDLE,
    STATE_AUTH_1,
    STATE_AUTH_2,
    STATE_UNLOCKED,
    STATE_LOCKED_OUT
} SystemState;

int main(void) {
    // 1. CONFIGURE KEYPAD (PORTC)
    DDRC = 0x0F;
    PORTC = 0xF0;

    // 2. CONFIGURE LCD (PORTA)
    DDRA = 0x3F; 

    // 3. CONFIGURE ENCODER PULL-UPS (PORTD)
    PORTD |= (1 << PORTD0) | (1 << PORTD1);

    // 4. CONFIGURE OUTPUTS (SERVO on PB5, BUZZER on PE3)
    DDRB |= (1 << PB5); 
    DDRE |= (1 << PE3); 

    // 5. CONFIGURE TIMER1 FOR SERVO PWM (50Hz / 20ms period)
    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11) | (1 << CS10);
    ICR1 = 4999; 
    OCR1A = 250; // Set to locked position (0 degrees)

    // 6. INITIALIZE SYSTEMS
    lcd_init();               
    encoder_init();           
    timer0_init(); // Start our 1ms system clock!
    
    lcd_print("SYSTEM LOCKED"); 
    
    // FSM Variables
    SystemState current_state = STATE_IDLE;
    char target_pin[] = "1234"; 
    char entered_pin[5];        
    uint8_t pin_index = 0;      
    char key; 
    
    // Timer Variables
    uint32_t auth1_start_time = 0; 
    uint32_t auth2_start_time = 0; // Added for Guard 2
    uint32_t elapsed_time;
    int seconds_left;

    // --- MAIN FSM LOOP ---
    while (1) {
        switch (current_state) {
            
            case STATE_IDLE:
                key = scan_keypad(); 
                if (key == '*') { 
                    current_state = STATE_AUTH_1;
                    pin_index = 0; 
                    
                    auth1_start_time = system_millis; // START THE 15-SECOND CLOCK
                    
                    lcd_clear();
                    lcd_print("AUTH 1 REQ:");
                }
                break;

            case STATE_AUTH_1:
                elapsed_time = system_millis - auth1_start_time;
                seconds_left = 15 - (elapsed_time / 1000);
                
                { 
                    static int last_second_printed = -1; 
                    
                    // --- THE LIVE COUNTDOWN TIMER ---
                    if (seconds_left != last_second_printed && seconds_left >= 0) {
                        last_second_printed = seconds_left;
                        lcd_write(0xC0 + 9, 0); 
                        char time_buffer[8];
                        lcd_print("T-");
                        itoa(seconds_left, time_buffer, 10);
                        lcd_print(time_buffer);
                        lcd_print("s "); 
                    }

                    // --- THE TIMEOUT CHECK ---
                    if (elapsed_time >= 15000) {
                        lcd_clear();
                        lcd_print("AUTH 1 TIMEOUT!");
                        PORTE |= (1 << PE3); _delay_ms(500); PORTE &= ~(1 << PE3);
                        _delay_ms(1500); 
                        lcd_clear();
                        lcd_print("SYSTEM LOCKED");
                        current_state = STATE_IDLE; 
                        last_second_printed = -1; 
                        break; 
                    }

                    key = scan_keypad();
                    if (key != '\0' && key != '*') { 
                        entered_pin[pin_index] = key;
                        lcd_write(0x80 + 11 + pin_index, 0); 
                        lcd_write(key, 1); 
                        pin_index++;
                        
                        if (pin_index == 4) {
                            entered_pin[4] = '\0'; 
                            
                            uint8_t is_match = 1;
                            for(int i=0; i<4; i++){
                                if(entered_pin[i] != target_pin[i]) is_match = 0;
                            }

                            last_second_printed = -1; 

                            if (is_match) {
                                current_state = STATE_AUTH_2; 
                                dial_position = 0; 
                                auth2_start_time = system_millis; // START GUARD 2'S CLOCK
                                lcd_clear();
                                lcd_print("G2 DIAL: 0");
                            } else {
                                current_state = STATE_LOCKED_OUT; 
                            }
                        }
                    }
                } 
                break;

            case STATE_AUTH_2:
                elapsed_time = system_millis - auth2_start_time;
                seconds_left = 15 - (elapsed_time / 1000);

                {
                    static int last_second_printed_g2 = -1; 

                    // --- THE LIVE COUNTDOWN TIMER FOR GUARD 2 ---
                    if (seconds_left != last_second_printed_g2 && seconds_left >= 0) {
                        last_second_printed_g2 = seconds_left;
                        lcd_write(0xC0 + 9, 0); 
                        char time_buffer[8];
                        lcd_print("T-");
                        itoa(seconds_left, time_buffer, 10);
                        lcd_print(time_buffer);
                        lcd_print("s "); 
                    }

                    // --- THE TIMEOUT CHECK FOR GUARD 2 ---
                    if (elapsed_time >= 15000) {
                        lcd_clear();
                        lcd_print("AUTH 2 TIMEOUT!");
                        PORTE |= (1 << PE3); _delay_ms(500); PORTE &= ~(1 << PE3);
                        _delay_ms(1500); 
                        lcd_clear();
                        lcd_print("SYSTEM LOCKED");
                        current_state = STATE_IDLE; 
                        last_second_printed_g2 = -1; 
                        break; 
                    }

                    // --- ENCODER LOGIC (NO LCD CLEARING) ---
                    if (dial_moved) {
                        dial_moved = 0; 
                        // Move cursor to top row, character 9 (right after "G2 DIAL: ")
                        lcd_write(0x80 + 9, 0); 
                        
                        char buffer[6];
                        itoa((int)dial_position, buffer, 10);
                        lcd_print(buffer);
                        lcd_print("   "); // Clear trailing numbers if going backward
                    }

                    key = scan_keypad();
                    if (key == '#') { 
                        if (dial_position == 15) { 
                            lcd_clear();
                            lcd_print("ACCESS GRANTED");
                            current_state = STATE_UNLOCKED;
                        } else {
                            current_state = STATE_LOCKED_OUT;
                        }
                        last_second_printed_g2 = -1; // Reset for next time
                    }
                }
                break;

            case STATE_UNLOCKED:
                lcd_clear();
                lcd_print("RACK UNLOCKED");
                lcd_write(0xC0, 0); 
                lcd_print("Closing in 5s...");
                
                OCR1A = 500; // Open Servo
                
                _delay_ms(5000); 
                
                // Warning beep
                PORTE |= (1 << PE3); _delay_ms(200); PORTE &= ~(1 << PE3);
                
                lcd_clear();
                lcd_print("SYSTEM LOCKED");
                
                OCR1A = 250; // Relock Servo
                current_state = STATE_IDLE; 
                break;

            case STATE_LOCKED_OUT:
                lcd_clear();
                lcd_print("SECURITY BREACH!");
                lcd_write(0xC0, 0); 
                lcd_print("SYSTEM LOCKOUT");
                
                // Alarm siren
                for (int i = 0; i < 3000; i++) {
                    PORTE |= (1 << PE3);  
                    _delay_us(500);
                    PORTE &= ~(1 << PE3); 
                    _delay_us(500);
                }
                
                lcd_clear();
                lcd_print("PENALTY WAIT...");
                _delay_ms(5000); 
                
                lcd_clear();
                lcd_print("SYSTEM LOCKED");
                current_state = STATE_IDLE; 
                break;
        }
    }
    return 0;
}