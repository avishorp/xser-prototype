/*******************************************************************/
/******** USB stack hardware selection options *********************/
/*******************************************************************/

//#define USE_SELF_POWER_SENSE_IO
#define tris_self_power     TRISCbits.TRISC2    // Input
#define self_power          1

//#define USE_USB_BUS_SENSE_IO
#define tris_usb_bus_sense  TRISCbits.TRISC2    // Input
#define USB_BUS_SENSE       1 


#define PIC18F2553_UBW
#define CLOCK_FREQ 48000000
#define GetSystemClock() CLOCK_FREQ   

/** LED ************************************************************/
#define mInitAllLEDs()     LATC &= 0xFC; TRISC &= 0xFC;
    
#define mLED_1              LATCbits.LATC0
#define mLED_2              LATCbits.LATC1
    
#define mGetLED_1()         mLED_1
#define mGetLED_2()         mLED_2

#define mLED_1_On()         mLED_1 = 1;
#define mLED_2_On()         mLED_2 = 1;
    
#define mLED_1_Off()        mLED_1 = 0;
#define mLED_2_Off()        mLED_2 = 0;

#define mLED_1_Toggle()     mLED_1 = !mLED_1;
#define mLED_2_Toggle()     mLED_2 = !mLED_2;


/** RS 232 lines ****************************************************/
#define UART_TRISTx   TRISBbits.TRISB7
#define UART_TRISRx   TRISBbits.TRISB5
#define UART_Tx       PORTBbits.RB7
#define UART_Rx       PORTBbits.RB5
#define UART_ENABLE RCSTAbits.SPEN

/** I/O pin definitions ********************************************/
#define INPUT_PIN 1
#define OUTPUT_PIN 0

//// NOTE: The XSer prototype does not implement handshake lines (DTS, CTS, ...)
    
//These definitions are only relevant if the respective functions are enabled
//in the usb_config.h file.
//Make sure these definitions match the GPIO pins being used for your hardware
//setup.
//    #define UART_DTS PORTBbits.RB0
//    #define UART_DTR LATBbits.LATB2
//    #define UART_RTS LATBbits.LATB3
//    #define UART_CTS PORTBbits.RB1
    
//    #define mInitRTSPin() {TRISBbits.TRISB3 = 0;}   //Configure RTS as a digital output.
//    #define mInitCTSPin() {TRISBbits.TRISB1 = 1;}   //Configure CTS as a digital input.  (Make sure pin is digital if ANxx functions is present on the pin)
//    #define mInitDTSPin() {TRISBbits.TRISB0 = 1;}   //Configure DTS as a digital input.  (Make sure pin is digital if ANxx functions is present on the pin)
//    #define mInitDTRPin() {TRISBbits.TRISB2 = 0;}   //Configure DTR as a digital output.

#define mInitRTSPin() {}  // Stub
#define mInitCTSPin() {}  // Stub
#define mInitDTSPin() {}  // Stub
#define mInitDTRPin() {}  // Stub

/** LCD ****************************************************/
// In the prototype, only one LCD digit is implemented. The seven segments (a-g)
// are conected to PORTA pins 0..6 respectively and the common is connected to pin 7.

#define mLCD_Init() { TRISB = 0; LATB = 0; TRISAbits.TRISA2 = 0; LATAbits.LATA2 = 0;}
#define mLCD_Invert() { LATB = ~LATB; LATAbits.LATA2 = ~LATAbits.LATA2; }
#define mLCD_SetDigit(v) { LATB = (v) ^ (LATAbits.LATA2? 0xff : 00); }
#define mLCD_GetDigit() (LATB  ^ (LATAbits.LATA2? 0xff : 00))


