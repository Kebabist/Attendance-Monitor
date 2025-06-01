    #ifndef F_CPU
    #define F_CPU 16000000UL
    #endif

    #ifndef GLCD_CONF_H
    #define GLCD_CONF_H

    // Device configuration
    #define GLCD_FONT_HEIGHT   8  // Tahoma11x13 font height
    #define GLCD_LINE_SPACING  2   // Additional spacing between lines

    #endif
    
    #include <avr/io.h>
    #include <avr/interrupt.h>
    #include <avr/eeprom.h>
    #include <util/delay.h>
    #include <util/twi.h>
    #include <stdio.h>
    #include <string.h>
    #include <stdint.h>
    #include "KS0108.h"
    #include "KS0108_Settings.h"   
    #include "Font5x8.h"

    // GLCD specific settings
    #define GLCD_WIDTH      128
    #define GLCD_HEIGHT     64

    // ----------------- USART Configuration -----------------
    #define BAUD 9600
    #define MYUBRR ((F_CPU / 16 / BAUD) - 1)

    // ----------------- Keypad Configuration -----------------
    #define KEYPAD_PORT PORTA
    #define KEYPAD_DDR  DDRA
    #define KEYPAD_PIN  PINA
    #define ROWS 4
    #define COLS 3

    #define ROW_START_PIN 1  // PA1
    #define COL_START_PIN 5  // PA5
    #define ROW_MASK 0x1E    // 0b00011110 (PA1-PA4)
    #define COL_MASK 0xE0    // 0b11100000 (PA5-PA7)

    // ----------------- Ultrasonic Sensor Configuration -----------------
    #define US_PORT   PORTB
    #define US_DDR    DDRB
    #define US_PIN    PINB
    #define US_TRIG   PB0
    #define US_ECHO   PB1
    #define US_ERROR         -1
    #define US_NO_OBSTACLE   -2

    // ----------------- Temperature Sensor Configuration -----------------
    #define TEMP_ADC_CHANNEL 0 // Using PA0/ADC0

    // ----------------- Buzzer Configuration -----------------
    #define BUZZER_PORT PORTD
    #define BUZZER_DDR  DDRD
    #define BUZZER_PIN  PD7


    //-------------------- RTC Definitions --------------------
#define RTC_ADDRESS 0x68
#define RTC_SDA_PIN PD2    // Using D2 for SDA
#define RTC_SCL_PIN PD3  
    // Structure for time/date
    typedef struct {
        uint8_t seconds;
        uint8_t minutes;
        uint8_t hours;
        uint8_t day;
        uint8_t date;
        uint8_t month;
        uint8_t year;
    } RTCDateTime;

    // ----------------- Constants -----------------
    #define MAX_STUDENTS       20
    #define STUDENT_ID_LENGTH  8
    #define EEPROM_START_ADDR  0x00
    #define ATTENDANCE_TIME_LIMIT 10
    #define BUFFER_SIZE 16

    // ------------------ Keypad matrix definition ------------------
    const char keypadMatrix[ROWS][COLS] = {
        {'1', '2', '3'},
        {'4', '5', '6'},
        {'7', '8', '9'},
        {'*', '0', '#'}
    };

    // ----------------- Menu States -----------------
    typedef enum {
        MENU_MAIN,
        MENU_ATTENDANCE,
        MENU_STUDENT_MGMT,
        MENU_VIEW_PRESENT,
        MENU_TEMP_MONITOR,
        MENU_RETRIEVE_DATA,
        MENU_TRAFFIC
    } MenuState;
    
    volatile MenuState currentMenu = MENU_MAIN;
    volatile MenuState previousMenu = MENU_MAIN;


    // -------------------Global Variables -------------------
    volatile uint32_t systemTime = 0;
    volatile uint32_t inputStartTime = 0;
    volatile uint8_t timeoutOccurred = 0;

    uint8_t studentCount = 0;
    uint16_t attendanceStartTime = 0;
    uint8_t attendanceActive = 0;

    typedef struct {
        char id[STUDENT_ID_LENGTH + 1];
        uint32_t timestamp;  // Timestamp in seconds since system start
    } StudentRecord;
    StudentRecord presentStudents[MAX_STUDENTS];  

    // ----------------- Function Declarations -----------------
    void initSystem(void);
    void initUSART(void);
    void initGLCD(void);
    void initKeypad(void);
    void initADC(void);
    void initUltrasonic(void);
    void initTimer0(void);

    void displayMenu(void);
    void handleKeypad(void);
    char getKeypadInput(void);

    void startupBeep(void);

    void startAttendance(void);
    uint8_t validateStudentID(const char* id);
    void submitStudentCode(void);
    void searchStudent(void);
    void viewPresentStudents(void);
    void removeStudent(void) ;
    void monitorTemperature(void);
    void retrieveStudentData(void);
    uint8_t checkAttendanceTimeLimit(void);
    void startAttendance(void);
    void monitorTraffic(void);

    void USART_Transmit(unsigned char data);
    void USART_TransmitString(const char *str);

    uint16_t readTemperature(void);
    uint16_t measureDistance(void);

    void buzzerBeep(void);
    void buzzerQuickBeep(void);
    void buzzerSuccessBeep(void);

    void saveToEEPROM(void);
    void loadFromEEPROM(void);

    // ----------------- System Initialization -----------------
    void initSystem(void) {
        initUSART();
        initGLCD();
        initKeypad();
        initADC();
        initUltrasonic();
        initTimer0();
        BUZZER_DDR |= (1 << BUZZER_PIN);
        loadFromEEPROM();   
        startupBeep();
    }

    void startupBeep(void) {
    // Play three short beeps
    for (uint8_t i = 0; i < 2; i++) {
        BUZZER_PORT |= (1 << BUZZER_PIN); // Turn buzzer on
        _delay_ms(200);                    // Delay for 100 ms
        BUZZER_PORT &= ~(1 << BUZZER_PIN); // Turn buzzer off
        _delay_ms(200);                    // Delay for 100 ms between beeps
    }
    
    // Play one long beep
    BUZZER_PORT |= (1 << BUZZER_PIN);    // Turn buzzer on
    _delay_ms(500);                       // Delay for 500 ms
    BUZZER_PORT &= ~(1 << BUZZER_PIN);   // Turn buzzer off
}

    // ----------------- USART -----------------
void initUSART(void) {
    // Set baud rate
    UBRRL = (unsigned char)MYUBRR;
    UBRRH = (unsigned char)(MYUBRR >> 8);
    
    // Enable transmitter and receiver
    UCSRB = (1 << RXEN) | (1 << TXEN);
    
    // Set frame format: 8 data bits, 1 stop bit
     UCSRC = (1 << UCSZ1) | (1 << UCSZ0);
}

void USART_Transmit(unsigned char data) {
    // Wait for empty transmit buffer
    while (!(UCSRA & (1 << UDRE)));
    
    // Put data into buffer
    UDR = data;
}

void USART_TransmitString(const char *str) {   
    if (!str) return;
    
    while (*str) {
        USART_Transmit(*str++);
        _delay_ms(50); // Small delay between characters
    }
    // Send newline
    USART_Transmit('\r');
    USART_Transmit('\n');
}

    // ----------------- LCD -----------------
void initGLCD(void) {
    // Power-up delay
    _delay_ms(100);
    
    // Setup
    GLCD_Setup();
    _delay_ms(100);
    GLCD_Clear();

    GLCD_SetFont(Font5x8, 5, 8, GLCD_Merge);
    GLCD_GotoXY(1, 1);
    GLCD_Render();
}

    // ----------------- Keypad -----------------

void initKeypad(void) {
    // Rows as outputs, set high
    KEYPAD_DDR |= ROW_MASK;
    KEYPAD_PORT |= ROW_MASK;
    
    // Columns as inputs, enable pull-ups
    KEYPAD_DDR &= ~COL_MASK;
    KEYPAD_PORT |= COL_MASK;
}

// Function to read a key from the buffer
char getKeypadInput(void) {
    for (uint8_t row = 0; row < ROWS; row++) {
        // Reset all rows high
        KEYPAD_PORT |= ROW_MASK;
        
        // Set current row low
        KEYPAD_PORT &= ~(1 << (row + ROW_START_PIN));
        
        // Brief delay for Proteus
        _delay_ms(100);
        
        // Direct pin read
        uint8_t cols = KEYPAD_PIN;
        
        // Mask and shift columns
        cols = (cols & COL_MASK) >> COL_START_PIN;
        
        // Check if any key pressed in this row
        if (cols != 0x07) {  // 0x07 is no-key-pressed state
            for (uint8_t col = 0; col < COLS; col++) {
                if (!(cols & (1 << col))) {
                    _delay_ms(50);  // Small verification delay
                    // Verify key is still pressed
                    if (!((KEYPAD_PIN & COL_MASK) >> COL_START_PIN & (1 << col))) {
                        return keypadMatrix[row][col];
                    }
                }
            }
        }
    }
    return 0;  // No key pressed
}
    // ----------------- ADC (Temperature) -----------------
    void initADC(void) {
        // Set AVCC as reference voltage
        ADMUX = (1 << REFS0);  // Using AVCC with external capacitor at AREF pin
        
        // Enable ADC and set prescaler to 128 for accurate readings
        ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
        
        // Do a dummy read to initialize ADC
        ADCSRA |= (1 << ADSC);
        while(ADCSRA & (1 << ADSC));
        (void)ADC;
        _delay_ms(200);  // Increased delay for ADC stabilization
    }

    uint16_t readTemperature(void) {
        // Start conversion
        ADCSRA |= (1 << ADSC);
        while (ADCSRA & (1 << ADSC));  // Wait for conversion
        
        // For LM35: Output is 10mV/°C
        // With 5V reference and 10-bit ADC:
        // Temperature = (ADC_value * 5000mV) / (1024 * 10mV/°C)
        uint16_t voltage = (ADC * 5000.0) / 1024.0;  // Convert to millivolts
        return voltage / 10.0;  // Convert to Celsius
    }

    // ----------------- Ultrasonic -----------------
    void initUltrasonic(void) {
        US_DDR |= (1 << US_TRIG);   // Trigger pin output
        US_DDR &= ~(1 << US_ECHO);  // Echo pin input
    }

uint16_t measureDistance(void) {
    uint32_t i, result;

    // Trigger pulse
    US_PORT |= (1 << US_TRIG);
    _delay_us(150);
    US_PORT &= ~(1 << US_TRIG);

    // Wait for rising edge
    for (i = 0; i < 600000; i++) {
        if (US_PIN & (1 << US_ECHO)) break;
    }
    if (i == 600000) return US_ERROR;

    // Start Timer1 with prescaler 8
    TCCR1A = 0x00;
    TCCR1B = (1 << CS11);
    TCNT1 = 0;

    // Wait for falling edge
    for (i = 0; i < 600000; i++) {
        if (!(US_PIN & (1 << US_ECHO))) break;
        if (TCNT1 > 60000) return US_NO_OBSTACLE;
    }
    result = TCNT1;
    TCCR1B = 0x00;

    if (result > 60000) return US_NO_OBSTACLE;

    // Convert to centimeters
    // Timer ticks / (58μs × 2 ticks/μs) = distance in cm
    return (uint16_t)(result / 116);
}

    // ----------------- Buzzer -----------------
    void buzzerBeep(void) {
        BUZZER_PORT |= (1 << BUZZER_PIN);
        _delay_ms(750);
        BUZZER_PORT &= ~(1 << BUZZER_PIN);
    }

    void buzzerQuickBeep(void) {
    BUZZER_PORT |= (1 << BUZZER_PIN);
    _delay_ms(250);
    BUZZER_PORT &= ~(1 << BUZZER_PIN);
    }

void buzzerSuccessBeep(void) {
    for(uint8_t i = 0; i < 2; i++) {
        BUZZER_PORT |= (1 << BUZZER_PIN);
        _delay_ms(250);
        BUZZER_PORT &= ~(1 << BUZZER_PIN);
        _delay_ms(250);
        }
    }

    // ----------------- EEPROM Operations -----------------
    void saveToEEPROM(void) {
        uint16_t addr = EEPROM_START_ADDR;
        eeprom_write_byte((uint8_t*)addr++, studentCount);
        for(uint8_t i = 0; i < studentCount; i++) {
            // Valid data check
            if(!validateStudentID(presentStudents[i].id)) {
                    presentStudents[i].id[0] = '\0';
                    presentStudents[i].timestamp = 0;
            }

            for(uint8_t j = 0; j < STUDENT_ID_LENGTH; j++) {
                eeprom_write_byte((uint8_t*)addr++, presentStudents[i].id[j]);
            }
            eeprom_write_dword((uint32_t*)addr, presentStudents[i].timestamp);
            addr += sizeof(uint32_t);
        }
    }

void loadFromEEPROM(void) {
    // First clear all records
    for(uint8_t i = 0; i < MAX_STUDENTS; i++) {
        presentStudents[i].id[0] = '\0';
        presentStudents[i].timestamp = 0;
    }

    uint16_t addr = EEPROM_START_ADDR;
    studentCount = eeprom_read_byte((const uint8_t*)addr++);
    if (studentCount > MAX_STUDENTS) studentCount = 0;

    for(uint8_t i = 0; i < studentCount; i++) {
        for(uint8_t j = 0; j < STUDENT_ID_LENGTH; j++) {
            presentStudents[i].id[j] = eeprom_read_byte((const uint8_t*)addr++);
        }
        presentStudents[i].id[STUDENT_ID_LENGTH] = '\0';
        presentStudents[i].timestamp = eeprom_read_dword((const uint32_t*)addr);
        addr += sizeof(uint32_t);
    }
}

    // ----------------- Menu & Logic -----------------
    void displayMenu(void) {
        GLCD_Clear();
        switch(currentMenu) {
            case MENU_MAIN:
                GLCD_GotoXY(1, 1);
                GLCD_PrintString("1:Attendance");
                GLCD_GotoXY(1, 9);
                GLCD_PrintString("2:Student Mgmt");
                GLCD_GotoXY(1, 17);
                GLCD_PrintString("3:Temperature");
                GLCD_GotoXY(1, 25);
                GLCD_PrintString("4:Retrieve Data");
                GLCD_GotoXY(1, 33);
                GLCD_PrintString("5:Traffic Monitor");
                break;
                
            case MENU_ATTENDANCE:
                GLCD_GotoXY(0, 0);
                GLCD_PrintString("Enter ID:");
                GLCD_GotoXY(0, 8);
                GLCD_PrintString("*:Back #:Submit");
                break;
                
            case MENU_STUDENT_MGMT:
                GLCD_GotoXY(0, 0);
                GLCD_PrintString("1:Search");
                GLCD_GotoXY(0, 8);
                GLCD_PrintString("2:ViewPresent");
                GLCD_GotoXY(0,16);
                GLCD_PrintString("3:Remove Student");
                GLCD_GotoXY(0,24);
                GLCD_PrintString("*:Back");
                break;
                
            case MENU_VIEW_PRESENT:
                GLCD_GotoXY(0, 0);
                GLCD_PrintString("Present Students");
                break;
                
            case MENU_TEMP_MONITOR:
                GLCD_GotoXY(0, 0);
                GLCD_PrintString("Temp Monitor...");
                break;
                
            case MENU_RETRIEVE_DATA:
                GLCD_GotoXY(0, 0);
                GLCD_PrintString("Retrieving Data");
                break;
                
            case MENU_TRAFFIC:
                GLCD_GotoXY(0, 0);
                GLCD_PrintString("Traffic Monitor...");
                break;
        }
        GLCD_Render(); 
        previousMenu = currentMenu;
    }

void handleKeypad(void) {
    char key = getKeypadInput();
    if(!key) return;

    switch(currentMenu) {
        case MENU_MAIN:
            if(key == '1') {
                currentMenu = MENU_ATTENDANCE;
                displayMenu();
                submitStudentCode(); 
            }
            else if(key == '2') {
                currentMenu = MENU_STUDENT_MGMT;
                displayMenu();
            }
            else if(key == '3') {
                currentMenu = MENU_TEMP_MONITOR;
                displayMenu();
                monitorTemperature(); 
            }
            else if(key == '4') {
                currentMenu = MENU_RETRIEVE_DATA;
                displayMenu();
                retrieveStudentData(); 
            }
            else if(key == '5') {
                currentMenu = MENU_TRAFFIC;
                displayMenu();
                monitorTraffic();
            }
            break;

            case MENU_STUDENT_MGMT:

            if(key == '*') {
                currentMenu = MENU_MAIN;
                displayMenu();
            } 
            else if(key == '1') {
                searchStudent();
            } 
            else if(key == '2') {      
                viewPresentStudents();  
            }
            else if(key == '3') {
                removeStudent();
            }
            break;

        default:
            if(key == '*') {
                currentMenu = MENU_MAIN;
                displayMenu();
            }
            break;
    }
}

    void initTimer0(void) {
        // Setup Timer0 for 1ms interrupts
        TCCR0 = (1 << WGM01) | (1 << CS02);  // CTC mode, prescaler 256
        OCR0 = 62;  // For 1ms interrupt at 16MHz
        TIMSK |= (1 << OCIE0);  // Enable compare match interrupt
        sei();  // Enable global interrupts
    }

    // Timer0 interrupt for system time
    ISR(TIMER0_COMP_vect) {
        static uint16_t ms_counter = 0;
        ms_counter++;
        if (ms_counter >= 1000) {  // 1 second passed
            systemTime++;
            ms_counter = 0;
        }
    }

void searchStudent(void) {
    char searchID[STUDENT_ID_LENGTH + 1] = {0};
    uint8_t idIndex = 0;
    char key;
    uint8_t found = 0;

    GLCD_Clear();
    GLCD_GotoXY(1, 1);
    GLCD_PrintString("Enter ID:");
    GLCD_GotoXY(1, 16);
    GLCD_PrintString("*:Back #:Submit");
    GLCD_Render();

    while(1) {
        key = getKeypadInput();
        if(key) {
            if(key == '*') {
                currentMenu = MENU_STUDENT_MGMT;
                return;
            }
            if((key >= '0' && key <= '9') && idIndex < STUDENT_ID_LENGTH) {
                searchID[idIndex++] = key;
                searchID[idIndex] = '\0';
                GLCD_GotoXY(1, 8);
                GLCD_GotoXY(1, 8);  // Position after "Enter ID:"
                GLCD_PrintString(searchID);
                // Add cursor position indicator
                if(idIndex < STUDENT_ID_LENGTH) {
                    GLCD_PrintString("_");
                }
                GLCD_Render();
            }
            if(key == '#' && idIndex == STUDENT_ID_LENGTH) {
                for(uint8_t i = 0; i < studentCount; i++) {
                    if(strncmp(searchID, presentStudents[i].id, STUDENT_ID_LENGTH) == 0) {
                        GLCD_Clear();
                        GLCD_GotoXY(1, 1);
                        GLCD_PrintString("Found:");
                        GLCD_GotoXY(1, 9);
                        GLCD_PrintString(presentStudents[i].id);
                        GLCD_GotoXY(1, 17);
                        uint8_t hours = (presentStudents[i].timestamp / 3600) % 24;
                        uint8_t minutes = (presentStudents[i].timestamp / 60) % 60;
                        char timeStr[BUFFER_SIZE];
                        snprintf(timeStr, sizeof(timeStr), "Time: %02u:%02u", hours, minutes);
                        GLCD_PrintString(timeStr);
                        GLCD_Render();
                        found = 1;
                        break;
                    }
                }
                if(!found) {
                    GLCD_Clear();
                    GLCD_GotoXY(1, 1);
                    GLCD_PrintString("No Record");
                    GLCD_GotoXY(1, 9);
                    GLCD_PrintString("Exists!");
                    GLCD_Render();
                }
                _delay_ms(2000);
                currentMenu = MENU_STUDENT_MGMT;
                return;
            }
        }
        _delay_ms(500);
    }
}


void viewPresentStudents(void) {
    char buffer[BUFFER_SIZE];

    if(studentCount == 0) {
        GLCD_Clear();
        GLCD_GotoXY(1, 1);
        GLCD_PrintString_P("No Students Present");
        GLCD_Render();
        buzzerQuickBeep();
        _delay_ms(1000);
        return;
    }

    for(uint8_t i = 0; i < studentCount; i++) {
        // Valid data check
        if(!validateStudentID(presentStudents[i].id)) {
            continue;
        }

        GLCD_Clear();
        GLCD_GotoXY(0, 0);
        GLCD_PrintString("ID:");
        GLCD_PrintString(presentStudents[i].id);

        uint8_t hours = (presentStudents[i].timestamp / 3600) % 24;
        uint8_t minutes = (presentStudents[i].timestamp / 60) % 60;
        snprintf(buffer, sizeof(buffer), "%02u:%02u", hours, minutes);

        GLCD_GotoXY(0, 16);
        GLCD_PrintString("Time:");
        GLCD_PrintString(buffer);
        GLCD_Render();

        _delay_ms(500);
    }
}

void removeStudent(void) {
    char removeID[STUDENT_ID_LENGTH + 1] = {0};
    uint8_t idIndex = 0;
    char key;
    uint8_t removed = 0;

    GLCD_Clear();
    GLCD_GotoXY(1, 1);
    GLCD_PrintString("Remove Student");
    GLCD_GotoXY(1, 16);
    GLCD_PrintString("*:Back #:Submit");
    GLCD_Render();

    while(1) {
        key = getKeypadInput();
        if(key) {
            if(key == '*') {
                currentMenu = MENU_STUDENT_MGMT;
                return;
            }
            if(key >= '0' && key <= '9' && idIndex < STUDENT_ID_LENGTH) {
                removeID[idIndex++] = key;
                removeID[idIndex] = '\0';
                GLCD_GotoXY(1, 8);  // Position after "Enter ID:"
                GLCD_PrintString(removeID);
                // Add cursor position indicator
                if(idIndex < STUDENT_ID_LENGTH) {
                    GLCD_PrintString("_");
                }
                GLCD_Render();
            }
            if(key == '#' && idIndex == STUDENT_ID_LENGTH) {
                for(uint8_t i = 0; i < studentCount; i++) {
                    if(strcmp(removeID, presentStudents[i].id) == 0) {
                        // Shift array left
                        for(uint8_t j = i; j < studentCount - 1; j++) {
                            presentStudents[j] = presentStudents[j + 1];
                        }
                        studentCount--;
                        saveToEEPROM();
                        removed = 1;
                        GLCD_Clear();
                        GLCD_GotoXY(1, 1);
                        GLCD_PrintString("Student");
                        GLCD_GotoXY(1, 9);
                        GLCD_PrintString("Removed!");
                        GLCD_Render();
                        _delay_ms(3000);
                        break;
                    }
                }
                if(!removed) {
                    GLCD_Clear();
                    GLCD_GotoXY(1, 1);
                    GLCD_PrintString("ID not found!");
                    GLCD_Render();
                    _delay_ms(3000);
                }
                currentMenu = MENU_STUDENT_MGMT;
                return;
            }
        }
        _delay_ms(500);
    }
}

void monitorTemperature(void) {
        char key;
        uint16_t raw_adc;
        uint16_t temp;
        char tempStr[BUFFER_SIZE];
        
        // Select ADC channel 0 with left adjust result
        ADMUX = (1 << REFS0) | (0 & 0x07);
        
        // Initial display setup
        GLCD_Clear();
        GLCD_GotoXY(1, 1);
        GLCD_PrintString("Temperature:");
        GLCD_GotoXY(1, 25);
        GLCD_PrintString("*:Back");
        GLCD_Render();

        while(1) {
            GLCD_PrintString("Temperature:");
            GLCD_GotoXY(1, 25);
            GLCD_PrintString("*:Back");
            key = getKeypadInput();
            if(key == '*') {
                currentMenu = MENU_MAIN;
                return;
            }

            // Start conversion
            ADCSRA |= (1 << ADSC);
            while (ADCSRA & (1 << ADSC)); // Wait for conversion
            raw_adc = ADC;
            
            // Convert to temperature (LM35: 10mV/°C)
            temp = (raw_adc * 5000.0) / 1024.0;  // Convert to millivolts
            temp = temp / 10.0;  // Convert to Celsius
            
            // Clear previous readings
            GLCD_Clear();
            // Display readings
            snprintf(tempStr, sizeof(tempStr), "ADC:%u", raw_adc);
            GLCD_GotoXY(1, 9);
            GLCD_PrintString(tempStr);
            
            snprintf(tempStr, sizeof(tempStr), "Temp:%d.%dC", 
                    (int)temp, (int)((temp - (int)temp) * 10));
            GLCD_GotoXY(1, 17);
            GLCD_PrintString(tempStr);
            
            GLCD_Render();
            _delay_ms(500);
        }
}

    void retrieveStudentData(void) {
        char buffer[BUFFER_SIZE];
        uint8_t i;

        // Show we're starting
        GLCD_Clear();
        GLCD_GotoXY(1, 1);
        GLCD_PrintString("Sending...");
        GLCD_Render();
        _delay_ms(1000);  // Give time for display

        // Test USART first
        USART_TransmitString("=== START ===");
        _delay_ms(1000);

        // Display student count
        snprintf(buffer, sizeof(buffer), "Students: %d", studentCount);
        GLCD_Clear();
        GLCD_GotoXY(1, 1);
        GLCD_PrintString(buffer);
        GLCD_Render();
        USART_TransmitString(buffer);
        _delay_ms(1000);

        // Send each student's data
        for(i = 0; i < studentCount; i++) {
            // Valid data check
            if(!validateStudentID(presentStudents[i].id)) {
                continue;
            }

            // Clear previous
            GLCD_Clear();

                        // Send to USART
            snprintf(buffer, sizeof(buffer), "ID:%s", 
                    presentStudents[i].id);
            USART_TransmitString(buffer);
            
            // Format student data
            uint8_t hours = (presentStudents[i].timestamp / 3600) % 24;
            uint8_t minutes = (presentStudents[i].timestamp / 60) % 60;
            
            // Show on LCD
            GLCD_GotoXY(1, 1);
            GLCD_PrintString(presentStudents[i].id);
            snprintf(buffer, sizeof(buffer), "Time: %02u:%02u", hours, minutes);
            GLCD_GotoXY(1, 9);
            GLCD_PrintString(buffer);
            GLCD_Render();
            USART_TransmitString(buffer);
            
            _delay_ms(1000);  // Show each record for 1 second
        }

        // Show completion
        GLCD_Clear();
        GLCD_GotoXY(1, 1);
        GLCD_PrintString("Data Sent!");
        GLCD_GotoXY(1, 9);
        GLCD_PrintString("Records:");
        snprintf(buffer, sizeof(buffer), "%d", studentCount);
        GLCD_GotoXY(1, 17);
        GLCD_PrintString(buffer);
        GLCD_Render();
        
        USART_TransmitString("=== END ===");
        _delay_ms(2000);
        
        currentMenu = MENU_MAIN;
    }

    uint8_t checkAttendanceTimeLimit(void) {
        if (!attendanceActive) return 1;

        uint32_t elapsedTime = systemTime - inputStartTime;
        
        if (elapsedTime >= ATTENDANCE_TIME_LIMIT) {
            GLCD_Clear();
            GLCD_GotoXY(1, 1);
            GLCD_PrintString("Time Limit");
            GLCD_GotoXY(1, 9);
            GLCD_PrintString("Exceeded!");
            GLCD_Render();
            buzzerBeep();
            _delay_ms(2000);
            timeoutOccurred = 1;
            return 0;
        }
        return 1;
    }

    void startAttendance(void) {
        attendanceActive = 1;
        attendanceStartTime = 0;  // Current time
        GLCD_Clear();
        GLCD_GotoXY(1, 1);
        GLCD_PrintString("Attendance");
        GLCD_GotoXY(1, 9);
        GLCD_PrintString("Started!");
        GLCD_Render();
        _delay_ms(1000);
    }

void monitorTraffic(void) {
    uint16_t dist;
    char distStr[BUFFER_SIZE];
    uint8_t peopleCount = 0;
    char key;
    
    GLCD_Clear();
    GLCD_GotoXY(0, 0);
    GLCD_PrintString("Traffic Monitor");
    GLCD_GotoXY(0, 8);
    GLCD_PrintString("*:Back");
    GLCD_Render();

    while(1) {
        key = getKeypadInput();
        if(key == '*') {
            currentMenu = MENU_MAIN;
            return;
        }

        dist = measureDistance();
        
        // Only count when person moves away (to avoid multiple counts)
        if(dist <= 3) {
            peopleCount++;
            buzzerQuickBeep();
        }

        GLCD_Clear();
        GLCD_GotoXY(0, 0);
        GLCD_PrintString("Traffic Monitor");
        
        if(dist != (uint16_t)US_ERROR && dist != (uint16_t)US_NO_OBSTACLE) {
            snprintf(distStr, sizeof(distStr), "Dist:%u m", dist);
            GLCD_GotoXY(0, 16);
            GLCD_PrintString(distStr);
        } else {
            GLCD_GotoXY(0, 16);
            GLCD_PrintString("No Obstacle");
        }

        snprintf(distStr, sizeof(distStr), "Count:%u", peopleCount);
        GLCD_GotoXY(0, 24);
        GLCD_PrintString(distStr);
        
        GLCD_GotoXY(0, 32);
        GLCD_PrintString("*:Back");
        GLCD_Render();

        _delay_ms(2000); // 2 second delay between readings
    }
}
    uint8_t validateStudentID(const char* id) {
        // Validate year (20-23)
        if (id[0] != '2') return 0;
        if (id[1] < '0' || id[1] > '3') return 0;
        
            // Validate department (001-999)
            if (id[2] < '0' || id[2] > '9') return 0;
            if (id[3] < '0' || id[3] > '9') return 0;
            if (id[4] < '0' || id[4] > '9') return 0;
            if (id[2] == '0' && id[3] == '0' && id[4] == '0') return 0;
            
            // Validate student number (001-999)
            if (id[5] < '0' || id[5] > '9') return 0;
            if (id[6] < '0' || id[6] > '9') return 0;
            if (id[7] < '0' || id[7] > '9') return 0;
            if (id[5] == '0' && id[6] == '0' && id[7] == '0') return 0;
            
        return 1;
    }

    void submitStudentCode(void) {
        char studentID[STUDENT_ID_LENGTH + 1] = {0};
        uint8_t idIndex = 0;
        char key;
        uint8_t updateDisplay = 1;
        static uint32_t lastTimeUpdate = 0;

        // Start timing
        attendanceActive = 1;
        inputStartTime = systemTime;
        timeoutOccurred = 0;
        char timeStr[16];
        
        
        // Clear screen and show initial prompt
        GLCD_Clear();
        GLCD_GotoXY(1, 1);
        GLCD_PrintString("Enter ID:");
        GLCD_Render();

        while(1) {

            // Check for timeout
            if (!checkAttendanceTimeLimit()) {
                currentMenu = MENU_MAIN;
                return;
            }
            key = getKeypadInput();
            
            // Update display if needed
            if(updateDisplay) {
                if(systemTime != lastTimeUpdate) {
                    lastTimeUpdate = systemTime;
                    
                    // Calculate and display remaining time
                    uint8_t remainingTime = ATTENDANCE_TIME_LIMIT - (systemTime - inputStartTime);
                    if(remainingTime > ATTENDANCE_TIME_LIMIT) {
                        remainingTime = 0; // Handle overflow
                    }
                    snprintf(timeStr, sizeof(timeStr), "Time Limit: %d s", remainingTime);
                }
                GLCD_Clear();
                GLCD_GotoXY(1, 1);
                GLCD_PrintString("Enter ID:");
                GLCD_Render();
                GLCD_GotoXY(1, 8);  // Position after "Enter ID:"
                GLCD_PrintString(studentID);
                // Add cursor position indicator
                if(idIndex < STUDENT_ID_LENGTH) {
                    GLCD_PrintString("_");
                }
                // Display remaining time
                // GLCD_GotoXY(1, 17);
                // GLCD_ClearLine(2);
                // GLCD_PrintString(timeStr);
                
                GLCD_Render();
                updateDisplay = 0;
            }

            if(key) {
                // Reset timeout counter on valid input
                inputStartTime = systemTime;
                lastTimeUpdate = systemTime - 1; // Force immediate time update
                updateDisplay = 1;
                // Handle backspace (*) - clear last character
                if(key == '*') {
                    attendanceActive = 0;
                // currentMenu = MENU_MAIN;
                    if(idIndex > 0) {
                        studentID[--idIndex] = '\0';
                        updateDisplay = 1;
                        
                        // Clear the line
                        GLCD_GotoXY(1, 8);
                        GLCD_PrintString("        ");  // 8 spaces
                        GLCD_Render();
                    } else {
                        currentMenu = MENU_MAIN;
                        return;  // Exit if no characters to delete
                    }
                    continue;
                }

                // Handle number input
                if(key >= '0' && key <= '9' && idIndex < STUDENT_ID_LENGTH) {
                    studentID[idIndex++] = key;
                    studentID[idIndex] = '\0';
                    updateDisplay = 1;
                }

                // Handle submission (#)
                if(key == '#') {
                    // Check if ID is complete
                    if(idIndex != STUDENT_ID_LENGTH) {
                        GLCD_Clear();
                        GLCD_GotoXY(1, 1);
                        GLCD_PrintString("ID must be");
                        GLCD_GotoXY(1, 9);
                        GLCD_PrintString("8 digits!");
                        GLCD_Render();
                        buzzerBeep();
                        _delay_ms(2000);
                        
                        // Reset input
                        idIndex = 0;
                        memset(studentID, 0, sizeof(studentID));
                        
                        // Show input prompt again
                        GLCD_Clear();
                        GLCD_GotoXY(1, 1);
                        GLCD_PrintString("Enter ID:");
                        GLCD_Render();
                        updateDisplay = 1;
                        continue;
                    }

                    // Validate ID format
                    if(!validateStudentID(studentID)) {
                        GLCD_Clear();
                        GLCD_GotoXY(1, 1);
                        GLCD_PrintString("Invalid ID");
                        GLCD_GotoXY(1, 9);
                        GLCD_PrintString("Format!");
                        GLCD_Render();
                        buzzerBeep();
                        _delay_ms(2000);
                        
                        // Reset input
                        idIndex = 0;
                        memset(studentID, 0, sizeof(studentID));
                        
                        // Show input prompt again
                        GLCD_Clear();
                        GLCD_GotoXY(1, 1);
                        GLCD_PrintString("Enter ID:");
                        GLCD_Render();
                        updateDisplay = 1;
                        continue;
                    }

                    // Check for duplicates 
                    uint8_t isDuplicate = 0;
                    for(uint8_t i = 0; i < studentCount; i++) {
                        if(strcmp(studentID, presentStudents[i].id) == 0) {
                            isDuplicate = 1;
                            break;
                        }
                    }

                    if(isDuplicate) {
                        GLCD_Clear();
                        GLCD_GotoXY(1, 1);
                        GLCD_PrintString("Already");
                        GLCD_GotoXY(1, 9);
                        GLCD_PrintString("Present!");
                        GLCD_Render();
                        buzzerBeep();
                        _delay_ms(2000);
                        // Reset input
                        idIndex = 0;
                        memset(studentID, 0, sizeof(studentID));
                        
                        // Show input prompt again
                        GLCD_Clear();
                        GLCD_GotoXY(1, 1);
                        GLCD_PrintString("Enter ID:");
                        GLCD_Render();
                        updateDisplay = 1;
                        continue;
                    }

                    // Add new student if space available
                    if(studentCount >= MAX_STUDENTS) {
                        GLCD_Clear();
                        GLCD_GotoXY(1, 1);
                        GLCD_PrintString("Maximum");
                        GLCD_GotoXY(1, 9);
                        GLCD_PrintString("Reached!");
                        GLCD_Render();
                        buzzerBeep();
                        _delay_ms(2000);
                        // Reset input
                        idIndex = 0;
                        memset(studentID, 0, sizeof(studentID));
                        
                        // Show input prompt again
                        GLCD_Clear();
                        GLCD_GotoXY(1, 1);
                        GLCD_PrintString("Enter ID:");
                        GLCD_Render();
                        updateDisplay = 1;
                        continue;
                    }

                    // Add student and save to EEPROM
                    strcpy(presentStudents[studentCount].id, studentID);
                    presentStudents[studentCount].id[STUDENT_ID_LENGTH] = '\0';
                    presentStudents[studentCount].timestamp = systemTime;
                    studentCount++;
                    
                    // Save to EEPROM immediately
                    saveToEEPROM();

                    // Show success message
                    GLCD_Clear();
                    GLCD_GotoXY(1, 1);
                    GLCD_PrintString("Attendance");
                    GLCD_GotoXY(1, 9);
                    GLCD_PrintString("Recorded!");
                    GLCD_Render();
                    buzzerSuccessBeep();  // Success beep
                    idIndex = 0;
                    memset(studentID, 0, sizeof(studentID));
                    currentMenu = MENU_ATTENDANCE;
                    _delay_ms(2000);
                    return;
                }
            // Update display every second for countdown
            static uint32_t lastUpdate = 0;
            if(systemTime != lastUpdate) {
                lastUpdate = systemTime;
                updateDisplay = 1;
            }
            _delay_ms(100);  // Debounce delay
        }

        }
    }

    // ----------------- Main -----------------
    int main(void) {  
        initSystem();
        displayMenu();
        
        uint8_t displayUpdateNeeded = 0;
        uint8_t lastKey = 0;

        while (1) {
            char key = getKeypadInput();
            
            if (key && key != lastKey) {
                lastKey = key;
                handleKeypad();
                displayUpdateNeeded = 1;
            }
            
            if (!key) {
                lastKey = 0;
            }

            if (displayUpdateNeeded || previousMenu != currentMenu) {
                displayMenu();
                displayUpdateNeeded = 0;
            }        
            GLCD_Render();
        }
        
        return 0;
    }