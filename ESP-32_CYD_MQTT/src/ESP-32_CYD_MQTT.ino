// Modified by Mike Eitel to display on ILI9341 based board
// All special modified libraries are stored in the via platformio.ini defined libraries directory !!!
// Took some time to get the right versions etc. working flawless, so this unusual approach is choosen deliberately.
// No rights reserved when used non commercial, otherwise contact author.

#define Rhy2        // If defined ( Me .. MeIOT .. LU ..  Rhy ) use private network for testing, otherwise use IOT standard

//#define LCDtype     // Witch LCDtype of CYD  choose by additional letter  N .. R .. C (Defined in platformio.ini )
//#define TochSleep     // The screen gets dark and touch messages sending disabled after defined time 

//#define WithSensors   // A variant that measures I2C sensors
#if defined(WithSensors)
  //#define BMP280    // BMP180 [0x77] or BMP280 aka (HW611) [0x76] Temperatur and air pressure in celsius and pascal 
  //#define CCS811    // CCS811 [0x5A] (CO2 and TOVC)  or  AGS10 [0x1A] (only TOVC air quality)  
  //#define AS5600    // AS5600 [0x36] Magnetic rotation board
  //#define LM75      // [0x48] Temperature precision sensor
//  #define MLX90614  // [0x5A] Temperature IR sensor on GY906 board.  PICKY sensor best run as last one !
#endif

//#define TEST        // Testmodus

// Automatic config of touch hw depending from board environement choice in platform.ini
#ifdef ESP32_2432S024N
  #define LCDtypeN
#elif ESP32_2432S024C
  #define LCDtypeC
#elif ESP32_2432S024R
  #define LCDtypeR
#endif

// Needed standard libraries
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <driver/adc.h>

#if defined(LCDtypeC) 
  #include <bb_captouch.h>  
#elif defined(LCDtypeR)
  #include "CYD_XPT2046_Touchscreen.h"    // Adapted Adafruit with permanant change to HSPI
#endif

// This files contain the device definitions for different networks
// In the credential files the fixed IP has to be defined
#if defined(Me)                       
    #include <Me_credentials.h>      
#elif defined(MeIOT)
    #include <MeIOT_credentials.h>     
#elif defined(LU)
    #include <LU_credentials.h>     
#elif defined(Rhy)
    #include <Rhy_credentials.h> 
#elif defined(Rhy2)
    #include <Rhy2_credentials.h> 
#else
    #include <credentials.h>         
#endif

// CYD uses HSPI and even same pins for screen and XPT2046 chip !!!
#define HSPI_MISO     12    // GPIO pin for HSPI MISO
#define HSPI_MOSI     13    // GPIO pin for HSPI MOSI
#define HSPI_SCK      14    // GPIO pin for HSPI clock
#define HSPI_SS       15    // GPIO pin for HSPI SS (slave select)

// Define used pins of the lcd
#define CYD_RST       -1    // org -1 but if needed probably unused pin 23 used
#define CYD_DC         2
#define CYD_MISO HSPI_MISO  // 12
#define CYD_MOSI HSPI_MOSI  // 13
#define CYD_SCLK HSPI_SCK   // 14
#define CYD_CS        15    // Chip select for screen
#define CYD_BL        27    // The display backlight
#define CYD_LDR       34    // The ldr light sensor different from CYD source = 21
// Replaced by light controled pwm mode
//#define Bon HIGH                  // Backlight ON easier to read in code when non pwm use
//#define Boff LOW                  // Backlight OFF easier to read in code when non pwm use

// Screen size for the raw to coordinate transformation width and height for landscape orientation
#define HRES 320            // Default screen resulution for X axis
#define VRES 320            // Default screen resulution for Y axis

#if defined(LCDtypeC)       // These are for the capacitive touch type  
  #define CST820_SDA  33    // diffrent from CYD source = 
  #define CST820_SCL  32    // diffrent from CYD source = 
  #define CST820_RST  -1 //25    // diffrent from CYD source = 
  #define CST820_IRQ  -1 //21    // diffrent from CYD source = // Define touch areas on screen
  // Define touch areas on screen
  const int XTmin =   1;    // realistic values that come from touch upper left corner
  const int XTmax = 320;    // realistic values that come from touch lower right corner
  const int YTmin =   1;    // realistic values that come from touch upper left corner
  const int YTmax = 240;    // realistic values that come from touch lower right corner

#elif defined(LCDtypeR)     // These are for the capacitive touch type
  // Problem with standard libs as only one SPI is used -> special CYD_xxx lib needed
  #define XPT2046_IRQ 36    // Unused interupt pin
  #define XPT2046_MOSI HSPI_MOSI  // 13 // diffrent from CYD source = 32
  #define XPT2046_SCLK HSPI_MISO  // 12 // diffrent from CYD source = 39
  #define XPT2046_SCLK HSPI_SCK   // 14 // diffrent from CYD source = 25
  #define XPT2046_CS  33    // Chip select for touch
  // Define touch areas on screen
  const int XTmin =  220;   // realistic values that come from touch upper left corner
  const int YTmin =  250;   // realistic values that come from touch upper left corner
  const int XTmax = 3720;   // realistic values that come from touch lower right corner
  const int YTmax = 3790;   // realistic values that come from touch lower right corner
#endif

// Define onboard led
#define CYD_LED_RED    4    // The all in one led defining the lower left corner  
#define CYD_LED_GREEN 16    // All in one led
#define CYD_LED_BLUE  17    // All in one led
#define Lon LOW             // Led ON easier to read in code
#define Loff HIGH           // Led OFF easier to read in code

#if defined(WithSensors)
  //Plug hw pins Colours are with original delivered cables
  //#define PIN_CN1_GND	    // BLACK GND
  #define CN1_IO22	  22    // BLUE    Also connected on P3 pin 3 
  #define CN1_IO21		21    // YELLOW   
  //#define PIN_CN1_33      // RED 3.3V
  //#define PIN_P2_GND 	    // BLACK GND
  #define PIN_P3_IO35	35    // BLUE    Input only pin, no internal pull-ups available !!
  //#define PIN_P3_IO22	    // YELLOW  Also on the CN1-pin 2
  //#define PIN_P3_IO21	    // RED     Also on the CN1-pin 3
  #define SDA	CN1_IO21	    // Original hw SDA bui ventually different as pin 21 is used for C touch 
  #define SCL CN1_IO22      // Original hw SCL 

  volatile float sensor_angle = 9876;     // Angle position                   This value stays when not activated
  volatile uint16_t sensor_co2 = 9876;;   // Air co2 ppm                      This value stays when not activated
  volatile uint16_t sensor_tvoc = 9876;;  // Air quality                      This value stays when not activated
  volatile float sensor_pressure = 9876;; // Air pressure                     This value stays when not activated
  volatile float sensor_humidity = 9876;; // Humidity                         This value stays when not activated  
  volatile float sensor_temp = 9876;;     // Normal temperature               This value stays when not activated
  volatile float sensor_tempa = 9876;;    // MXL90614 own temperature         This value stays when not activated 
  volatile float sensor_tempo = 9876;;    // MXL90614 meassured temperature   This value stays when not activated

  #if defined(BMP180)       // BMP180 board
    #include <BMP180I2C.h>
    #define I2C_ADDRESS 0x77
//    #define SCL CN1_IO22    // Use hw I2C
//    #define SDA	CN1_IO21    // Use hw I2C
    BMP180I2C bmp180(I2C_ADDRESS);
  #elif defined(BMP280) // HW611 board with BMP280
    #include <Adafruit_Sensor.h>    
    #include <Adafruit_BMP280.h>
    #define I2C_ADDRESS 0x76
//    #define SCL CN1_IO22    // Use hw I2C
//    #define SDA	CN1_IO21    // Use hw I2C
    Adafruit_BMP280 bmp280; // I2C  
  #endif
  #if defined(CCS811)        // TVOC and C02 measurement CJMCU-811 board
    #include <DFRobot_CCS811.h>
    DFRobot_CCS811 ccs811;
//    #define SCL CN1_IO22    // Use hw I2C     ????
//    #define SDA CN1_IO21    // Use hw I2C     ????
    #define I2C_ADDRESS 0x5A
  #elif defined(AGS10)        // TVOC measurement ASG10 board
    #include <MeAGS10.h>
//    #define SCL CN1_IO22    // Use hw I2C     ????
//    #define SDA CN1_IO21    // Use hw I2C     ????
    #define I2C_ADDRESS 0x1A
    Me_AGS10 ags10 = Me_AGS10();
    int tvoc;
  #endif  
  #if defined(AS5600)       // Magnetic rotation board
    #include <AS5600.h>     // Seeed Studio runs without errors
    //    #define SCL CN1_IO22    // Use hw I2C     ????
    //    #define SDA CN1_IO21    // Use hw I2C     ????
    #define I2C_ADDRESS 0x36
      AMS_5600 as5600;
  #endif
  #if defined(LM75)     // IR temperature measurement board
    #include <LM75_derived.h>
//    #define SCL CN1_IO22    // Use hw I2C     ????
//    #define SDA CN1_IO21    // Use hw I2C     ????
    #define I2C_ADDRESS 0x48
    Generic_LM75 lm75(I2C_ADDRESS);
  #endif 
  #if defined(MLX90614)     // IR temperature measurement board
    #include <Adafruit_MLX90614.h>
//    #define SCL CN1_IO22    // Use hw I2C     ????
//    #define SDA CN1_IO21    // Use hw I2C     ????
    #define I2C_ADDRESS 0x5A
    Adafruit_MLX90614 mlx90614 = Adafruit_MLX90614();
  #endif 
#endif

// Define additional colour variants
#define ILI9341_DARK      050505  // a very dark gray
#define ILI9341_DARK_BLUE 000005  // a very dark blue
int showsensorslocal = 17;                // Field number to show sensor on screen

#define mqMaxtext 55              // Maximal tranverable text via mqtt minus 1

// Multitasking for 2 cores
BaseType_t taskCreationResult;
TaskHandle_t Task0;

// Declare global variables and constants
unsigned long currentMillis;                  // Actual timer 
unsigned long prevRMQTTMillis = 2764472319;   // Stores last MQTT time value was published 2764472319->FASTER START
unsigned long prevSMQTTMillis = 2764472319;   // Stores last MQTT time value was published 2764472319->FASTER START
unsigned long prevMinMillis = 2764472319;     // Stores last minutes time value ->  2764472319->FASTER START
unsigned long prevTickerMillis = 2764472319;  // Stores last ticker time value ->  2764472319->FASTER START
unsigned long bklonMillis = 2764472319;       // Stores last backlighton time value ->  2764472319->FASTER START

String Sendme;                                // Used for clear text messages in MQTT
String MySensors;                             // A list of usable sensors
int receivedlenght;                           // How long is mqtt message
char lastreceived;                            // Stores the last received status
char receivedChar[mqMaxtext + 35];            //  = "";
bool received;                                // Actual received status
int watchdogW = 1;                            // Counter if there is no wifi connection
int watchdogM = 1;                            // Counter if there is no MQTT connection
int mqttstatus;                               // Helper to see whats going on
bool watchdog = true;                         // Signal via mqtt that device is still ok
bool statusreset = false;                     // Used to minimize error 0 sendouts
bool Ticker;                                  // Ticker for fx. blinking leds
int looped = 1;                               // Loop counter as debug helper
byte bkl_set = 255;                           // Control of pwm dimmed backlight
byte bkl_now = 1;                             // Helper to control dimmed backlight
byte bkl_last = 0;                            // Helper to control dimmed backlight
int ldr;                                      // Value of LDR readout

// Variables needed for mqtt commands
int mqX;                          // Received from mqtt start a X position                
int mqY;                          // Received from mqtt start a Y position
int mqlX;                         // Received from mqtt lenght off X area
int mqlY;                         // Received from mqtt lenght off Y area
int mqtX;                         // Received from mqtt X start of text
int mqtY;                         // Received from mqtt X start of text
int mqS;                          // Received from mqtt text size
char mqT[mqMaxtext];              // Received from mqtt text
int mqC;                          // Received from mqtt text colour
int mqB;                          // Received from mqtt background colour
int trend[3] = {25, 24, 61};      // cyd preset chars for rising/falling/equal

uint16_t Colourtable[20];
bool LEDsta_used;                 // Status that LEDsta was set via mqtt
int LEDsta = 1;                   // Commandstatus for all set by mqtt
int LEDsta_BL = 1;                // Commandstatus for backlight
int LEDsta_R = 0;                 // Commandstatus for red led
int LEDsta_G = 0;                 // Commandstatus for green led
int LEDsta_B = 0;                 // Commandstatus for blue led

// Only needed when there is touch included     xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
#if !defined (LCDtypeN)
  float XTdiv = 320.0 / (XTmax - XTmin);    // Needed to adjust touch error in X
  float YTdiv = 240.0 / (YTmax - YTmin);    // Needed to adjust touch error in Y
  int Xraw;                                 // Last detected X position on touch
  int Yraw;                                 // Last detected Y position on touch
  int X = 0;                                // Last calculated X position on touch
  int Y = 0;                                // Last calculated Y position on touch
  int field = 0;                   // Actual detected screen field      USED in both tasks
  volatile int lastfield = 0;               // Helper to avoid double activities USED in both tasks
  bool bkl_ON= true;                        // Can switch off backlight when a touch screen
  const long bkl_NotSleep = 60000;             // defines how long the screen stays bright
  bool touch_SHOW = false;                  // Show the touch field on screen for testing
#endif

  struct areastruct
  {             // The areas are defined as numbered area based on pixels
    int area;   // Unique nr. off the defined field, starting from 0
    int Xstart; // Start/reference point if the field X
    int Xlen;   // Length of the field in X
    int Xtext;  // Start of text X based on Xstart
    int Xtlen;  // Length of the texts background X
    int Ystart; // Start/reference point if the field Y
    int Ylen;   // Length of the field in Y
    int Ytext;  // Start of text Y based on Ystart
    int Ytlen;  // Length of the texts background Y
};
areastruct areas[12];

struct areastaticstruct { // Used to have a static "menue's text" on screen
  int field;              // This unique area number is NOT the same as in area 
  int area;               // This is based on areas.area[]. Can be multiple times in this struct                                
  int Xoff;               // X offset of background, based on areas.Xstart of areas.area[]
  int Xlen;               // Background length X, based on areas.Xstart of areas.area[]
  int Xtext;              // X offset of text, based on area.Xstart of areas.area[]
  int Yoff;               // Y offset of background, based on area.Ystart  of areas.area[]
  int Ylen;               // Background length Y, based on area.Ystart of areas.area[] 
  int Ytext;              // Y offset of text, based on area.Ystart of areas.area[]
  int size;               // Size of text 
  int fg;                 // Foreground colour
  int bg;                 // Background colour
  const char* text;       // The text
};
areastaticstruct ar_sta[40];   // A application depending structure of static texts

struct fieldvaluestruct { // Used to have a static "menue's text" on screen
  int field;              // This unique field number is NOT the same as in area 
  int area;               // This is a area.field[]. Can be multiple times in this struct                                
  int Xvoff;              // X Start of value based on area Xstart
  int Xvlen;              // X Length of value based on area Xstart
  int Xvbsta;             // Start of background based on Xstart
  int Xvblen;             // Length of background based on Xstart
  int Yvoff;              // Y Start of value based on area Xstart
  int Yvlen;              // Y Length of value based on area Xstart
  int Yvbsta;             // Start of background based on Ystart
  int Yvblen;             // Length of background based on Ystart
};
fieldvaluestruct fieldval[40];   // A application depending structure of static texts

int XY1tareas = 4;        // Possible touch areas on X axis for upper row Y1
int XY2tareas = 4;        // Possible touch areas on X axis for middle row Y2
int XY3tareas = 4;        // Possible touch areas on X axis in case there are 3 rows Y3
int XYtamax  = 17;        // Maximal number of areas
int Staticmax  = 12;      // Maximal number of static texts for areas
int Fieldmax  = 40;       // Maximal number of field for areas

Adafruit_ILI9341 cyd = Adafruit_ILI9341(CYD_CS, CYD_DC, CYD_RST); // When resistive touch below software spi is not usable !
//Adafruit_ILI9341 cyd = Adafruit_ILI9341(CYD_CS, CYD_DC, CYD_MOSI, CYD_SCLK, CYD_RST, CYD_MISO);
#include <gfxfont.h>

// Only needed when there is touch included     xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
#if defined(LCDtypeC)
  BBCapTouch touch;
  TOUCHINFO ti;
  const char *szNames[] = {"Unknown", "FT6x36", "GT911", "CST820"};
#elif defined(LCDtypeR)
  XPT2046_Touchscreen touchHW(XPT2046_CS);
#endif

// Setup the background classes  
WiFiClient   espClient;     // Get Wifi access
PubSubClient mqttclient;    // MQTT protokol handler


// AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA  APPLICATION SPECIFIC SCREENS startAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
                      // Rhy is 4_2_0
#define touch_3_2_0   // This decides witch of the below screen layouts is used

void MakeScreenTable(){

    #if defined(touch_3_2_0)       // Possible touch areas on X axis for middle row Y2
    XY1tareas = 3;                 // Possible touch areas on X axis for upper row Y1
    XY2tareas = 2;                 // Possible touch areas on X axis for middle row Y2
    XY3tareas = 0;                 // Possible touch areas on X axis in case there are 3 rows Y3
    
    // Definition of the main areas
    int fieldValues[] =  {   1,   2,   3,   4,   5,  99};
    int XstartValues[] = {   1, 108, 215,   1, 161,  99};
    int XlenValues[] =   { 105, 105, 105, 158, 159,  99};
    int XtextValues[] =  {   5,   5,   5,   5,   5,  99};
    int XtlenValues[] =  {  98,  98,  98, 150, 150,  99};
    int YstartValues[] = {   1,   1,   1, 121, 121,  99};
    int YlenValues[] =   { 118, 118, 118, 118, 118,  99};
    int YtextValues[] =  {   5,   5,   5,   5,   5,  99};
    int YtlenValues[] =  {  25,  25,  25,  25,  25,  99};

    // Definition of the static / menue text in an area
    int SfieldValue[] = {   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  99};
    int SareaValue[] =  {   1,   1,   2,   2,   3,   3,   4,   4,   5,   5,  99};
    int SXoffValue[] =  {   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   1};
    int SXlenValue[] =  { 100, 100, 100, 100, 100, 100, 154, 154, 154, 154,   1};
    int SXtextValue[] = {  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,   1};
    int SYoffValue[] =  {   3,  23,   3,  23,   3,  23,   3,  23,   3,  23,   1};
    int SYlenValue[] =  {  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,   1};
    int SYtextValue[] = {   5,  23,   5,  23,   5,  23,   5,  23,   6,  24,   1};
    int SsizeValue[] =  {   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   1};
    int SfgValue[] =    {  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,   1};
    int SbgValue[] =    {   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   1};
    const char* textValue[] = { " Sauna"," Jurte"," Arven"," Sauna","grosses","  Fass",
                                " Ruhe Jurte","  mit Ofen","kleines Fass","  mit Ofen","x"};

    // Definition of the dynamic written fields in the areas
    int FfieldValue[] =  {   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  99};
    int FareaValue[] =   {   1,   1,   2,   2,   5,   5,   3,   3,   4,   4,   4,   4,   5,   5,  99};
    int FXvbstaValue[] = {   2,  83,   2,  83,   2, 138,   2,  83,   2, 138,   2, 138,   2, 138,  99};
    int FXvblenValue[] = {  79,  18,  79,  18, 135,  18,  79,  18, 135,  18, 135,  18, 135,  18,  99};
    int FYvbstaValue[] = {  65,  65,  65,  65,  45,  45,  65,  65,  45,  45,  80,  80,  80,  80,  99};
    int FYvblenValue[] = {  37,  37,  37,  37,  34,  34,  37,  37,  34,  34,  34,  34,  34,  34,  99};
    int FXvoffValue[] =  {   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,  99};
    int FXvlenValue[] =  {  75,  17,  75,  17, 132,  17,  75,  17, 132,  17, 132,  17, 132,  17,  99};
    int FYvoffValue[] =  {   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,  99};
    int FYvlenValue[] =  {  29,  29,  29,  29,  25,  25,  29,  29,  25,  25,  25,  25,  25,  25,  99};

  #elif defined(touch_4_2_0)   // The 4 * 2 
    XY1tareas = 4;                 // Possible touch areas on X axis for upper row Y1
    XY2tareas = 2;                 // Possible touch areas on X axis for middle row Y2
    XY3tareas = 0;                 // Possible touch areas on X axis in case there are 3 rows Y3
    showsensorslocal = 4;
    // Definition of the main areas
    int fieldValues[] =  {   1,   2,   3,   4,   5,   6,  99};
    int XstartValues[] = {   1,  81, 161, 241,   1, 161,  99};
    int XlenValues[] =   {  78,  78,  78,  79, 158, 159,  99};
    int XtextValues[] =  {   5,   5,   5,   5,   5,   5,  99};
    int XtlenValues[] =  {  70,  70,  70,  70, 150, 150,  99};
    int YstartValues[] = {   1,   1,   1,   1, 121, 121,  99};
    int YlenValues[] =   { 118, 118, 118, 118, 118, 118,  99};
    int YtextValues[] =  {   5,   5,   5,   5,   5,   5,  99};
    int YtlenValues[] =  {  25,  25,  25,  25,  25,  25,  99};

    // Definition of the static / menue text in an area
    int SfieldValue[] = {   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  99};
    int SareaValue[] =  {   1,   1,   2,   2,   3,   3,   4,   4,   5,   5,   6,   6,  99};
    int SXoffValue[] =  {   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   1};
    int SXlenValue[] =  {  73,  73,  73,  73,  73,  73,  74,  74, 154, 154, 154, 154,   1};
    int SXtextValue[] = {  10,  10,  10,  10,   5,  17,   5,  17,  28,  33,  50,  10,   1};
    int SYoffValue[] =  {   3,  23,   3,  23,   3,  23,   3,  23,   3,  23,   3,  23,   1};
    int SYlenValue[] =  {  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,   1};
    int SYtextValue[] = {   5,  23,   5,  23,   5,  23,   5,  23,   6,  24,   6,  24,   1};
    int SsizeValue[] =  {   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   1};
    int SfgValue[] =    {  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,   1};
    int SbgValue[] =    {   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   1};
    const char* textValue[] = { "Sauna","Jurte","Arven","Sauna","klein.","Fass","gross.","Fass",
                                "Ruhejurte","mit Ofen","Ofen","kleines Fass","off"};

    // Definition of the dynamic written fields in the areas
    int FfieldValue[] =  {   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  99};
    int FareaValue[] =   {   1,   1,   2,   2,   3,   3,   4,   4,   5,   5,   5,   5,   6,   6,  99};
    int FXvbstaValue[] = {   2,  58,   2,  58,   2,  58,   2,  58,   1, 138,   1, 138,   1, 138,  99};
    int FXvblenValue[] = {  52,  18,  52,  18,  52,  18,  52,  18, 135,  18,  135, 18,  135, 18,  99};
    int FYvbstaValue[] = {  65,  65,  65,  65,  65,  65,  65,  65,  45,  45,  80,  80,  60,  60,  99};
    int FYvblenValue[] = {  37,  37,  37,  37,  37,  37,  37,  37,  32,  32,  32,  32,  36,  36,  99};
    int FXvoffValue[] =  {   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,  99};
    int FXvlenValue[] =  {  50,  17,  50,  17,  50,  17,  50,  17, 132,  17, 132,  17, 132,  17,  99};
    int FYvoffValue[] =  {   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,  99};
    int FYvlenValue[] =  {  29,  29,  29,  29,  29,  29,  29,  29,  23,  23,  23,  23,  26,  26,  99};

  #elif defined(touch_3_3_4)   // The 3+3+4 
    XY1tareas = 3;                 // Possible touch areas on X axis for upper row Y1
    XY2tareas = 3;                 // Possible touch areas on X axis for middle row Y2
    XY3tareas = 4;                 // Possible touch areas on X axis in case there are 3 rows Y3

    showsensorslocal = 10;
    // Definition of the main areas
    int fieldValues[] =  {   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  99};
    int XstartValues[] = {   1, 108, 215,   1, 108, 215,   1,  81, 161, 241,  99};
    int XlenValues[] =   { 105, 105, 105, 105, 105, 105,  78,  78,  78,  79,  99};
    int XtextValues[] =  {   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,  99};
    int XtlenValues[] =  {  98,  98,  98,  98,  98,  98,  65,  65,  65,  65,  99};
    int YstartValues[] = {   1,   1,   1,  98,  98,  98, 194, 194, 194, 194,  99};
    int YlenValues[] =   {  95,  95,  95,  95,  95,  95,  46,  46,  46,  46,  99};
    int YtextValues[] =  {   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,  99};
    int YtlenValues[] =  {  25,  25,  25,  25,  25,  25,  25,  25,  25,  25,  99};

    // Definition of the static / menue text in an area  
    int SfieldValue[] = {   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  99};
    int SareaValue[] =  {   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,   1,   2,   3,   4,   5,   6,  99};
    int SXoffValue[] =  {   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   1};
    int SXlenValue[] =  { 100, 100, 100, 100, 100, 100,  73,  73,  73,  73, 100, 100, 100, 100, 100, 100,   1};
    int SXtextValue[] = {  10,  10,  10,  10,  10,  10,   6,  10,   4,  10,  10,  10,  10,  10,  10,  10,   1};
    int SYoffValue[] =  {   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   1};
    int SYlenValue[] =  {  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,   1};
    int SYtextValue[] = {   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   1};
    int SsizeValue[] =  {   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   1};
    int SfgValue[] =    {  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,   1};
    int SbgValue[] =    {   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   1};
    const char* textValue[] = { "Decke","Vorhang","Sofa","Decke","Vorhang","Sofa",
                                "Wohnzi","Kuech","Garten","Sensors",
                                "AA","BB","CC","AA","BB","CC","off"};

    // Definition of the dynamic written fields in the areas
    int FfieldValue[] =  {   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  99};
    int FareaValue[] =   {   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  99};
    int FXvbstaValue[] = {   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,  99};
    int FXvblenValue[] = { 100, 100, 100, 100, 100, 100,  74,  74,  74,  74,  99};
    int FYvbstaValue[] = {  25,  25,  25,  25,  25,  25,  21,  21,  21,  21,  99};
    int FYvblenValue[] = {  67,  67,  67,  67,  67,  67,  24,  24,  24,  24,  99};
    int FXvoffValue[] =  {   7,   7,   7,   7,   7,   7,   3,   3,   3,   3,  99};
    int FXvlenValue[] =  {  90,  90,  90,  90,  90,  90,  70,  70,  70,  70,  99};
    int FYvoffValue[] =  {  13,  13,  13,  13,  13,  13,   3,   3,   3,   3,  99};
    int FYvlenValue[] =  {  53,  53,  53,  53,  53,  53,  19,  19,  19,  19,  99};

  #elif defined(touch_4_4_4)   // The 3+3+4 
    // Max sensfull matrix of 4x4x4
    XY1tareas = 4;                 // Possible touch areas on X axis for upper row Y1
    XY2tareas = 4;                 // Possible touch areas on X axis for middle row Y2
    XY3tareas = 4;                 // Possible touch areas on X axis in case there are 3 rows Y3

    // Definition of the main areas
    int fieldValues[] =  {   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  99};
    int XstartValues[] = {   1,  81, 161, 241,   1,  81, 161, 241,   1,  81, 161, 241,  99};
    int XlenValues[] =   {  78,  78,  78,  79,  78,  78,  78,  79,  78,  78,  78,  79,  99};
    int XtextValues[] =  {   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,  99};
    int XtlenValues[] =  {  65,  65,  65,  65,  65,  65,  65,  65,  65,  65,  65,  65,  99};
    int YstartValues[] = {   1,   1,   1,   1,  81,  81,  81,  81, 161, 161, 161, 161,  99};
    int YlenValues[] =   {  78,  78,  78,  78,  78,  78,  78,  78,  79,  79,  79,  79,  99};
    int YtextValues[] =  {   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,  99};
    int YtlenValues[] =  {  25,  25,  25,  25,  25,  25,  25,  25,  25,  25,  25,  25,  99};

    // Definition of the static / menue text in an area
    int SfieldValue[] = {   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  99};
    int SareaValue[] =  {   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  99};
    int SXoffValue[] =  {   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   1};
    int SXlenValue[] =  {  73,  73,  73,  73,  73,  73,  73,  73,  73,  73,  73,  73,   1};
    int SXtextValue[] = {  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,   1};
    int SYoffValue[] =  {   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   1};
    int SYlenValue[] =  {  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,   1};
    int SYtextValue[] = {   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   1};
    int SsizeValue[] =  {   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   1};
    int SfgValue[] =    {  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,   1};
    int SbgValue[] =    {   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   1};
    const char* textValue[] = {"A","B","C","D","E","F","G","H","I","J","K","L","off"};

    // Definition of the dynamic written fields in the areas
    int FfieldValue[] =  {   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  99};
    int FareaValue[] =   {   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  99};
    int FXvbstaValue[] = {   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,  99};
    int FXvblenValue[] = {  73,  73,  73,  73,  73,  73,  73,  73,  73,  73,  73,  73,  99};
    int FYvbstaValue[] = {  35,  35,  35,  35,  35,  35,  35,  35,  35,  35,  35,  35,  99};
    int FYvblenValue[] = {  35,  35,  35,  35,  35,  35,  35,  35,  35,  35,  35,  35,  99};
    int FXvoffValue[] =  {   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,  99};
    int FXvlenValue[] =  {  67,  67,  67,  67,  67,  67,  67,  67,  67,  67,  67,  67,  99};
    int FYvoffValue[] =  {   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,  99};
    int FYvlenValue[] =  {  26,  26,  26,  26,  26,  26,  26,  26,  26,  26,  26,  26,  99};

  #elif defined(touch_2_2_0)
    XY1tareas = 2;                 // Possible touch areas on X axis for upper row Y1
    XY2tareas = 2;                 // Possible touch areas on X axis for middle row Y2
    XY3tareas = 0;                // Possible touch areas on X axis in case there are 3 rows Y3

    // Definition of the main areas
    int fieldValues[] =  {   1,   2,   3,   4,  99};
    int XstartValues[] = {   1, 161,   1, 161,  99};
    int XlenValues[] =   { 158, 158, 158, 158,  99};
    int XtextValues[] =  {  10,  10,  10,  10,  99};
    int XtlenValues[] =  { 140, 140, 140, 140,  99};
    int YstartValues[] = {   1,   1, 121, 121,  99};
    int YlenValues[] =   { 118, 118, 118, 118,  99};
    int YtextValues[] =  {  10,  10,  10,  10,  99};
    int YtlenValues[] =  { 100, 100, 100, 100,  99};

    // Definition of the static / menue text in an area
    int SfieldValue[] =  {   1,   2,   3,   4,  99};
    int SareaValue[] =   {   1,   2,   3,   4,  99};
    int SXoffValue[] =   {   3,   3,   3,   3,  99};
    int SXlenValue[] =   { 152, 152, 152, 152,  99};
    int SXtextValue[] =  {  10,  10,  10,  10,  99};
    int SYoffValue[] =   {   3,   3,   3,   3,  99};
    int SYlenValue[] =   { 112, 112, 112, 112,  99};
    int SYtextValue[] =  {  10,  10,  10,  10,  99};
    int SsizeValue[] =   {   2,   2,   2,   2,  99};
    int SfgValue[] =     {  12,  12,  12,  12,  99};
    int SbgValue[] =     {   6,   6,   6,   6,  99};
    const char* textValue[] = {"Area A","Area B","Area C","Area D","off"};

    // Definition of the dynamic written fields in the areas
    int FfieldValue[] =  {   1,   2,   3,   4,   5,   6,   7,   8,  99};
    int FareaValue[] =   {   1,   1,   2,   2,   3,   3,   4,   4,  99};
    int FXvbstaValue[] = {   3, 116,   3, 116,   3, 116,   3, 116,  99};
    int FXvblenValue[] = { 120,  39, 115,  39, 100,  39, 100,  39,  99};
    int FYvbstaValue[] = {  40,  40,  40,  40,  40,  40,  40,  40,  99};
    int FYvblenValue[] = {  72,  72,  72,  72,  72,  72,  72,  72,  99};
    int FXvoffValue[] =  {   5,   5,   5,   5,   5,   5,   5,   5,  99};
    int FXvlenValue[] =  { 108,  28, 108,  28, 108,  28, 108,  28,  99};
    int FYvoffValue[] =  {   5,   5,   5,   5,   5,   5,   5,   5,  99};
    int FYvlenValue[] =  {  60,  60,  60,  60,  60,  60,  60,  60,  99};

  #else  
    XY1tareas = 1;                 // Possible touch areas on X axis for upper row Y1
    XY2tareas = 0;                 // Possible touch areas on X axis for middle row Y2
    XY3tareas = 0;                // Possible touch areas on X axis in case there are 3 rows Y3

    // Definition of the main areas
    int fieldValues[] =  {   1,   2,  99};
    int XstartValues[] = {   1,   1,  99};
    int XlenValues[] =   { 318,   1,  99};
    int XtextValues[] =  {  10,   1,  99};
    int XtlenValues[] =  { 300,   1,  99};
    int YstartValues[] = {   1,   1,  99};
    int YlenValues[] =   { 238,   1,  99};
    int YtextValues[] =  {  10,   1,  99};
    int YtlenValues[] =  { 220,   1,  99};


    // Definition of the static / menue text in an area
    int SfieldValue[] = {   1,   2,  99};
    int SareaValue[] =  {   1,   2,  99};
    int SXoffValue[] =  {   3,   1,  99};
    int SXlenValue[] =  { 317,   1,  99};
    int SXtextValue[] = {  10,   1,  99};
    int SYoffValue[] =  {   3,   1,  99};
    int SYlenValue[] =  { 220,   1,  99};
    int SYtextValue[] = {   5,   1,  99};
    int SsizeValue[] =  {   2,   1,  99};
    int SfgValue[] =    {  12,   1,  99};
    int SbgValue[] =    {   6,   1,  99};
    const char* textValue[] = {"A","","off"};

      // Definition of the dynamic written fields in the areas
    int FfieldValue[] =  {   1,   2,  99};
    int FareaValue[] =   {   1,   1,  99};
    int FXvbstaValue[] = {   3, 270,  99};
    int FXvblenValue[] = { 262,  47,  99};
    int FYvbstaValue[] = {  40,  40,  99};
    int FYvblenValue[] = { 178, 178,  99};
    int FXvoffValue[] =  {   5,   5,  99};
    int FXvlenValue[] =  { 253,  38,  99};
    int FYvoffValue[] =  {   5,   5,  99};
    int FYvlenValue[] =  { 165, 166,  99};
  #endif

  XYtamax = XY1tareas + XY2tareas + XY3tareas;  // Maximal number of areas
    
  for (int i = 0; i < XYtamax; i++){
    areas[i].area = fieldValues[i];
    areas[i].Xstart = XstartValues[i];
    areas[i].Xlen = XlenValues[i];
    areas[i].Xtext = XtextValues[i];
    areas[i].Xtlen = XtlenValues[i];
    areas[i].Ystart = YstartValues[i];
    areas[i].Ylen = YlenValues[i];
    areas[i].Ytext = YtextValues[i];
    areas[i].Ytlen = YtlenValues[i];
  }

  for (int s = 0; s < 40 && SfieldValue[s] < 99; s++) {
    Staticmax = SfieldValue[s];
  }
  for (int s = 0; s < Staticmax && ar_sta[s + 1].field < 99; s++) {
    ar_sta[s].field = SfieldValue[s]; 
    ar_sta[s].area = SareaValue[s];  
    ar_sta[s].Xoff = SXoffValue[s];  
    ar_sta[s].Xlen = SXlenValue[s];
    ar_sta[s].Xtext = SXtextValue[s]; 
    ar_sta[s].Yoff = SYoffValue[s];    
    ar_sta[s].Ylen = SYlenValue[s];
    ar_sta[s].Ytext = SYtextValue[s];     
    ar_sta[s].size = SsizeValue[s];    
    ar_sta[s].fg = SfgValue[s];    
    ar_sta[s].bg = SbgValue[s];    
    ar_sta[s].text = textValue[s];
  }

  for (int f = 0; f < 40 && FfieldValue[f] < 99; f++) {
    Fieldmax = FfieldValue[f];
  }
  for (int f = 0; f < Fieldmax && fieldval[f + 1].field < 99; f++) {
    fieldval[f].field = FfieldValue[f]; 
    fieldval[f].area = FareaValue[f];  
    fieldval[f].Xvoff = FXvoffValue[f];
    fieldval[f].Xvlen = FXvlenValue[f];
    fieldval[f].Xvbsta = FXvbstaValue[f];
    fieldval[f].Xvblen = FXvblenValue[f];
    fieldval[f].Yvoff = FYvoffValue[f];
    fieldval[f].Yvlen = FYvlenValue[f];
    fieldval[f].Yvbsta = FYvbstaValue[f];
    fieldval[f].Yvblen = FYvblenValue[f];
  }
}

void StartScreen() {    // MQTT S 
  // Test that profes the full screen adressing is from 0,0 to 319,239
  //  cyd.fillRect(0, 239, 1, 1, tCol(17)); cyd.fillRect(319, 239, 1, 1, tCol(17));
  cyd.fillRect(0, 0, 320, 240, tCol(3));
  for (int s = 0; s < XYtamax; s++)
  {
    cyd.drawRect(areas[s].Xstart, areas[s].Ystart, areas[s].Xlen, areas[s].Ylen, tCol(6));
  }
}

void  StaticText2Screen(int Sta, int Stb) {   // MQTT M
  if (Stb > Staticmax) { Stb = Staticmax; }
  for (int s = Sta; s < Stb ; s++) {
    int arr = ar_sta[s].area -1;
    int Xarr = areas[arr].Xstart;
    int Yarr = areas[arr].Ystart;    
    int Xra = Xarr + ar_sta[s].Xoff;
    int Yra = Yarr + ar_sta[s].Yoff;
    int XrSi = ar_sta[s].Xlen;
    int YrSi = ar_sta[s].Ylen;
    int XTa = Xarr + ar_sta[s].Xtext;
    int YTa = Yarr + ar_sta[s].Ytext;
    int S = ar_sta[s].size;
    u_int16_t Textcol = tCol(ar_sta[s].fg);
    u_int16_t Backcol = tCol(ar_sta[s].bg);
    const char* Stext = ar_sta[s].text;
    //                 Xra  Yra XrSi YrSi  XTa  YTa  S  Textcol   Backcol Text
    PrintArea2Screen(Xra, Yra, XrSi, YrSi, XTa, YTa, S, Textcol, Backcol, Stext );
  }
}

// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx APPLICATION SPECIFIC SCREENS end  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx


// mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm mqtt connection  start mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
void setup_wifi() {
  vTaskDelay(10);

  WiFi.config(staticIP, gateway, subnet);

  Serial.println("");
  Serial.print("Try connect to: ");
  Serial.println(wifi_ssid);
  Serial.print("With IP address: ");
  Serial.println(WiFi.localIP());

  cyd.setTextColor(ILI9341_WHITE); 
  cyd.setTextSize(1);
  cyd.setCursor(0,35);
  cyd.print("Try connect to: ");
  cyd.setTextColor(ILI9341_GREEN); 
  cyd.setTextSize(2);
  cyd.setCursor(120,30);
  cyd.println(wifi_ssid);

  cyd.setTextColor(ILI9341_WHITE); 
  cyd.setTextSize(1);
  cyd.setCursor(0,65);
  cyd.print("With IP address: ");
  cyd.setTextColor(ILI9341_GREEN); 
  cyd.setTextSize(2);  
  cyd.setCursor(120,60);
  cyd.println(WiFi.localIP());

  WiFi.begin(wifi_ssid, wifi_password);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  
  while ((WiFi.status()!= WL_CONNECTED) && (watchdogW <= WiFi_timeout)) {
    vTaskDelay(250);
    Serial.print(".");
    Serial.print(watchdogW);

    #if defined(TEST)
      cyd.setTextColor(ILI9341_WHITE); 
      cyd.setTextSize(2);  
      cyd.setCursor(55,100);      
      cyd.println("TEST MODE Wifi try");
    #else
      cyd.setTextColor(ILI9341_WHITE); 
      cyd.setTextSize(2);  
      cyd.setCursor(120,100);      
      cyd.println(" Wifi try");
    #endif
    cyd.fillRect(150,130,60,22,ILI9341_DARKGREY);
    cyd.setTextColor(ILI9341_GREEN); 
    cyd.setTextSize(3);
    cyd.setCursor(155,130); 
    cyd.print(watchdogW);

    watchdogW++;
  }
  if (WiFi.status()!= WL_CONNECTED){
    Serial.println ("No connection ");
    Serial.println ("Restart");
    cyd.fillRect(60,130,200,50,ILI9341_YELLOW);
    cyd.setTextColor(ILI9341_RED); 
    cyd.setTextSize(4);
    cyd.setCursor(80,140); 
    cyd.print("RESTART");
    vTaskDelay(5000);
    ESP.restart();
    }
  else {
    Serial.println(" Successfull connected Wifi");
    Serial.print("RSSI: ");   Serial.println(WiFi.RSSI());
    cyd.fillRect(0,95,320,25,ILI9341_GREEN);
    cyd.fillRect(60,130,200,50,ILI9341_BLACK);
    cyd.setTextColor(ILI9341_BLACK); 
    cyd.setTextSize(2);
    cyd.setCursor(5,100); 
    cyd.print("Successfull connected Wifi");
  }
}

void reconnect() {
  vTaskDelay(100);
  // Loop until we're reconnected to MQTT server
  while (!mqttclient.connected() && (watchdogM <= mqtt_timeout)) {
    mqttclient.clearWriteError();       // Cleaning MQTT write buffer
    mqttclient.flush();                 // Cleaning MQTT data buffer
    mqttstatus = mqttclient.state();    // Decoding of MQTT status 
    Serial.println("");
    Serial.print("Attempting MQTT connection try Nr. ");
    cyd.setTextColor(ILI9341_WHITE); 
    cyd.setTextSize(1);
    cyd.setCursor(0,135);
    cyd.print("Attempting MQTT connection: ");
    cyd.fillRect(170,128,40,22,ILI9341_DARKGREY);
    cyd.setTextColor(ILI9341_GREEN); 
    cyd.setTextSize(2);  
    cyd.setCursor(170,130);
    cyd.println(watchdogM);
   
    Serial.println(watchdogM);
    
    const char *reason;
    switch (mqttstatus) {
      case -4 : {reason = "The server didn't respond within the keepalive time"; break;}
      case -3 : {reason = "The network connection was broken"; break;}
      case -2 : {reason = "The network connection failed"; break;}
      case -1 : {reason = "The client is disconnected cleanly"; break;}
      case  0 : {reason = "The client is connected"; break;}
      case  1 : {reason = "The server doesn't support the requested version of MQTT"; break;}
      case  2 : {reason = "The server rejected the client identifier"; break;}
      case  3 : {reason = "The server was unable to accept the connection"; break;}
      case  4 : {reason = "The username/password were rejected"; break;}
      case  5 : {reason = "The client was not authorized to connect"; break;}
      default: {   }    // Wrong               
    }
    Serial.println(reason);
    cyd.fillRect(0,148,320,30,ILI9341_YELLOW);
    cyd.setTextColor(ILI9341_BLACK); 
    cyd.setTextSize(1);  
    cyd.setCursor(0,150);
    cyd.println(reason);
 
    if (mqttclient.connect(iamclient, mqtt_user, mqtt_password)) {
      Serial.print("Connected as: ");
      Serial.println(iamclient);
      Serial.println("");
      watchdogM = 1;
      cyd.fillRect(0,128,320,80,ILI9341_BLACK);
      cyd.setTextColor(ILI9341_WHITE); 
      cyd.setTextSize(1);  
      cyd.setCursor(0,150);
      cyd.println("Listening to MQTT:");
      cyd.setTextColor(ILI9341_GREEN); 
      cyd.setTextSize(2);  
      cyd.setCursor(20,170);
      cyd.println(in_topic);
      vTaskDelay(1500);                               // Restore the Startupscreen
      StartScreen();
      #if defined(TEST)                               // Send status after start
        mqttclient.publish(out_status, "1" ,false);
      #else
        mqttclient.publish(out_status, "MQTT Connected" ,false);
        vTaskDelay(500);
      #endif
    }         
    Serial.print("RSSI: ");   Serial.println(WiFi.RSSI());
    Serial.println("Retry MQTT in 1 second");
    vTaskDelay(800);
    watchdogM++;
    Serial.println("_");
  }

  if (watchdogM >= mqtt_timeout) {
    Serial.println("");
    Serial.println("NO MQTT available");
    Serial.println("RESTART");
    cyd.fillRect(0,125,320,105,ILI9341_YELLOW);
    cyd.setTextColor(ILI9341_RED); 
    cyd.setTextSize(4);
    cyd.setCursor(80,170); 
    cyd.print("RESTART");
    vTaskDelay(5000);
    ESP.restart();                               // REBOOT of the system !!!!!
  }
}

void callback(char* topic, uint8_t* payload, unsigned int length) {
  receivedlenght = length;
  Serial.println("");
  Serial.print("Message for [");
  Serial.print(topic);
  Serial.print("] arrived = L:(");
  Serial.print(length);
  Serial.print(")->");
  for (unsigned int i = 0; i < length; i++) {
    receivedChar[i] = payload[i];
    Serial.print(receivedChar[i]);                          //     Received from mqtt text
  }
  Serial.println();
  #if defined(TEST)
      mqttclient.publish(out_status, "3" ,false);
    #else
      mqttclient.publish(out_status, "Command received" ,false);
  #endif
  // The decision point to analyse commands received via MQTT
  switch (receivedChar[0]) {                                                // Detecition what command is detected 
    case '?' : {                                                            // Request parameters
      Sendme = "RSSI: ";
      Sendme = Sendme + (WiFi.RSSI());
      mqttclient.publish(out_param, (String(Sendme).c_str()), false);       // Send the wifi signal strenght
      Sendme = "Light: ";
      Sendme = Sendme + (analogRead(CYD_LDR));
      mqttclient.publish(out_ligth, (String(Sendme).c_str()), false);       // Send the cyd internal light sensor
      #if defined(WithSensors)
        mqttclient.publish(out_sensors, (String(MySensors).c_str()), false);  // Send list of the usable sensors
      #endif
      break;
    }
    case 'C' : { cyd.fillRect(0,0,319,239,tCol(0)); break;}                 // Clear screen dark
    case 'S' : { StartScreen(); StaticText2Screen(0,Fieldmax); break;}      // Start screen

    case 'U' : { cyd.fillRect(0,0,320,240,tCol(9)); break;}                 // Clear screen light grey
    case 'V' : { StartScreen(); break;}                                     // Only the frame
    case 'W' : { StaticText2Screen(0,Fieldmax); ShowArea(); break;}         // Static menue text to screen
    #if !defined(LCDtypeN)                                                  // Helper for AREA concept when touch
      case 'Y' : { int Va; Va = x2i(receivedChar,1,2);                      // Switch touch testmode via mqtt
        if (Va != 0) {touch_SHOW = true;} else {touch_SHOW = false;}; break;}
      case 'Z' : {  StartScreen(); StaticText2Screen(0, Fieldmax); vTaskDelay(1500); // Show all fields in touch
                    ShowTouch(); vTaskDelay(1500); ShowArea(); break; }
    #endif

    case 'I' : {int V; V = x2i(receivedChar,1,2); bkl_set = V; break;}      // Set intensity of display
    case 'L' : {int V; V = x2i(receivedChar,1,2); LEDsta = V; LEDsta_used = HIGH; break; } // All led's in one                                                                    // All led togeather
    case 'R' : {int V; V = x2i(receivedChar,1,2); LEDsta_R = V; break;}     // Red led
    case 'G' : {int V; V = x2i(receivedChar,1,2); LEDsta_G = V; break;}     // Green led
    case 'B' : {int V; V = x2i(receivedChar,1,2); LEDsta_B = V; break;}     // Blue led

    case 'T': { // Posibility to write on every position of screen in free format       T050081050113aBigTeXt
      mqX = x2i(receivedChar,1,3);            // Received from mqtt start a X position                
      mqY = x2i(receivedChar,4,6);            // Received from mqtt start a Y position
      mqS = x2i(receivedChar,7,8);            // Received from mqtt text size
      mqC = x2i(receivedChar,9,10);           // Received from mqtt text colour
      mqB = x2i(receivedChar,11,12);          // Received from mqtt background colour 
      for (unsigned int i = 0; i < (receivedlenght - 13); i++) {mqT[i] = receivedChar[i + 13];} // Received from mqtt text
      mqT[receivedlenght - 13] = '\0';
      PrintText2Screen(mqX, mqY, mqS, tCol(mqC), tCol(mqB), mqT);
      break; 
    }
    case 'M' : { // Used to write the predefined menue texts ( by given by start and end "array"-nr  )    M0106
      int Va;   int Vb; 
      Va = x2i(receivedChar,1,2);           // Received from mqtt witch variable is send
      Vb = x2i(receivedChar,3,4);           // Received from mqtt text colour
      StaticText2Screen(Va,Vb);
      break; 
    }
    case 'F': { // Most used command to write in predefined variables in according areas    F01020607mytext
      int V = x2i(receivedChar,1,2) -1;     // Received from mqtt witch field is written to
      mqS = x2i(receivedChar,3,4);          // Received from mqtt text size
      mqC = x2i(receivedChar,5,6);          // Received from mqtt text colour
      mqB = x2i(receivedChar,7,8);          // Received from mqtt background colour 
      for (unsigned int i = 0; i < (receivedlenght - 9); i++) {mqT[i] = receivedChar[i + 9];} // Received from mqtt text
      mqT[receivedlenght - 9] = '\0';
     
      int fff = fieldval[V].area - 1;
      int iarX = areas[fff].Xstart;         int iarY = areas[fff].Ystart;
      int iXb = iarX + fieldval[V].Xvbsta;  int iYb = iarY + fieldval[V].Yvbsta;
      int iXbl = fieldval[V].Xvblen;        int iYbl = fieldval[V].Yvblen;
      int iXv = iXb + fieldval[V].Xvoff;    int iYv = iYb + fieldval[V].Yvoff;
      int iXvl = fieldval[V].Xvlen;         int iYvl = fieldval[V].Yvlen;
      int fg = tCol(mqC); int bg = tCol(mqB);
      //cyd.drawRoundRect(iXb, iYb, iXbl, iYbl, 5, fg);
      //cyd.drawRect(iXb, iYb, iXbl, iYbl, fg); // tCol(6));
      PrintInArea2Screen(iXb, iYb, iXbl, iYbl, iXv, iYv, mqS, tCol(mqC), tCol(mqB), mqT);
      break;  
    }
    case 'A': { // Very definable way to write to screen      A020070110050045090040513Free Text
      mqX = x2i(receivedChar,1,3);            // Received from mqtt start a X position                
      mqY = x2i(receivedChar,4,6);            // Received from mqtt start a Y position
      mqlX = x2i(receivedChar,7,9);           // Received from mqtt lenght off X area
      mqlY = x2i(receivedChar,10,12);         // Received from mqtt lenght off Y area
      mqtX = x2i(receivedChar,13,15);         // Received from mqtt X start of text
      mqtY = x2i(receivedChar,16,18);         // Received from mqtt X start of text
      mqS = x2i(receivedChar,19,20);          // Received from mqtt text size
      mqC = x2i(receivedChar,21,22);          // Received from mqtt text colour
      mqB = x2i(receivedChar,23,24);          // Received from mqtt background colour 
      for (unsigned int i = 0; i < (receivedlenght - 25); i++) {mqT[i] = receivedChar[i + 25];} // Received from mqtt text
      mqT[receivedlenght - 25] = '\0';
      PrintInArea2Screen(mqX, mqY, mqlX, mqlY,mqtX, mqtY, mqS, tCol(mqC), tCol(mqB), mqT);
      break;  
    }
    case 'X' : { mqttclient.publish(out_status, "Restart" ,false);          // REMOTE RESTART
      vTaskDelay(1000); ESP.restart(); break; 
    }
    default: {                                                             // Wrong command
      #if defined(TEST)
        mqttclient.publish(out_status, "-1" ,false);
      #else
        mqttclient.publish(out_status, "No valid command" ,false);
        statusreset = true;
        vTaskDelay(500);
      #endif
      break;
    } 
  }

  // DIRTY TRICK to read all mqtt's fast if they are stacked
  // This overwrites the normal mqtt request timing by simulating an "older" timestamp
  prevRMQTTMillis = currentMillis - (readinterval - 250); // But at least 250ms before retrigger

}
// mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm mqtt connection    end mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm


// tttttttttttttttttttttttttttttttttttttttttttttt touch related start tttttttttttttttttttttttttttttttttttttttttttttt
#if !defined(LCDtypeN)

void printTouch2Serial() {
    Serial.println();
    Serial.print("Touched X/Y: ");
    Serial.print(X);
    Serial.print("/");
    Serial.println(Y);
  }
void findTouchPos() {
    // printTouch2Screen();
    // First the backlight must be activated
    currentMillis = millis();
    if (currentMillis - bklonMillis >= bkl_NotSleep) {
      bklonMillis = currentMillis; }  //Adjust to actual timing when first touch too long
    // Then there must be a pause of at least one second before command is send 
    else if (currentMillis - bklonMillis >= 300) { // Not more than 3 touches per second
      bkl_ON = true;
      float helper; 
      helper = (Xraw - XTmin) * XTdiv * 1.0;
      X = helper;   // conversion to integer  
      helper = (Yraw - YTmin) * YTdiv * 1.0;
      Y = helper; // conversion to integer  
      if (X<=0) {X = 1;}; if (X>320){X = 320;}
      if (Y<=0) {Y = 1;}; if (Y>240){Y = 240;}
      int Xh; int Yh;   int fh; int fhh = -1;   // Initialize with an invalid value
      for (int i = 0; i < XYtamax; i++) {       // Conversion to integer needed
        Xh = areas[i].Xstart + areas[i].Xlen;
        Yh = areas[i].Ystart + areas[i].Ylen;
        fh = areas[i].area;
        if (X >= areas[i].Xstart && X <= Xh && Y >= areas[i].Ystart && Y <= Yh) {
          if (fhh == -1 || fh < fhh) {fhh = fh;}
        }
      }
      if (fhh != -1) {
        ShowSensorsOnLocalScreen();
        if (touch_SHOW) {What2DoInField(fhh); };    // Possibility to show touchfields on screen

        Serial.println("Touched quadrant with field value: " + String(fhh));
        field = fhh;
        mqttclient.publish(out_button, (String(field).c_str()),false); } 
      else { Serial.println("No valid quadrant touched."); }
    }
    bklonMillis = currentMillis;  // Normal adjust time meanwhile non sleep cycle
  }
void showTouchTable() {
    for (int i = 0; i < XYtamax; i++) {
      char ftext[3];
      itoa(areas[i].area, ftext, 10);
      Serial.print(areas[i].area);
      // Write number to positioned field
      PrintField2Screen(areas[i].area, 2, 18, 2, ftext);
    }
  }
void ShowTouch(){                      // MQTT H
    for (int i = 0; i < (XYtamax); i++){  // Draw rectangle to show the fields that contain variables
      int iX = areas[i].Xstart; int iY = areas[i].Ystart;
      int iXl = areas[i].Xlen;  int iYl = areas[i].Ylen;
      int iXt = areas[i].Xstart + areas[i].Xtext; int iYt = areas[i].Ystart + areas[i].Ytext;
      int fg = tCol(10);  int bg = tCol(18);
      int is = 1;
      char itx[10]; 
      strcpy(itx, "Touch ");
      char numStr[3];
      itoa(i + 1, numStr, 10);
      strcat(itx, numStr);
      cyd.drawRect(iX, iY,iXl, iYl, fg); 
      PrintInArea2Screen(iX, iY, iXl, iYl, iXt, iYt, is, fg, bg, itx);
    }
  }
void printTouch2Screen(){
    cyd.fillRect(270,230,40,20,ILI9341_BLACK);
    cyd.setTextSize(1);
    cyd.setTextColor(ILI9341_CYAN); 
    cyd.setCursor(270,230);
    cyd.print(X);
    cyd.print("/");
    cyd.println(Y);
  }

void ShowSensorsOnLocalScreen(){
    if (field == showsensorslocal){
      cyd.fillRect(0,0,320,240,ILI9341_DARKGREY);
      cyd.setTextColor(ILI9341_CYAN); cyd.setCursor(5,3); cyd.setTextSize(3); cyd.println("Lokale Sensoren");

      cyd.setTextColor(ILI9341_GREEN); cyd.setCursor(0,40);
      if (sensor_humidity != 9876)  {cyd.print(sensor_humidity, 1); cyd.println(" %rHf"); 
                                    cyd.setTextSize(1); cyd.println(); cyd.setTextSize(3); };
      if (sensor_temp != 9876)      {cyd.print(sensor_temp,1);cyd.println(" C");  
                                    cyd.setTextSize(1); cyd.println(); cyd.setTextSize(3); };
      if (sensor_pressure != 9876)  {cyd.print(sensor_pressure,1); cyd.println(" mBar");  
                                    cyd.setTextSize(1); cyd.println(); cyd.setTextSize(3); };
      if (sensor_co2 != 9876)       {cyd.print(sensor_co2); cyd.println(" ppm CO2");  
                                    cyd.setTextSize(1); cyd.println(); cyd.setTextSize(3); };
      if (sensor_tvoc != 9876)      {cyd.print(sensor_tvoc); cyd.println(" TVOC"); 
                                    cyd.setTextSize(1); cyd.println(); cyd.setTextSize(3); };
      if (sensor_angle != 9876)     {cyd.print(sensor_angle,1); cyd.println(" WinkelGrad");
                                    cyd.setTextSize(1); cyd.println(); cyd.setTextSize(3); };
      if (sensor_tempa != 9876)     {cyd.print(sensor_tempa,1);cyd.println(" C Sensor"); 
                                    cyd.setTextSize(1); cyd.println(); cyd.setTextSize(3); };
      if (sensor_tempo != 9876)     {cyd.print(sensor_tempo,1);cyd.println(" C Objekt"); };
      lastfield = field;
    }
    if (lastfield  == showsensorslocal && field != showsensorslocal){
      lastfield = field;
      StartScreen();
      StaticText2Screen(0, Fieldmax);
      mqttclient.publish(out_status, "Reconnected" ,false);   // Dirty trick to get values from mqtt back into fields
    };
  }

#endif

// tttttttttttttttttttttttttttttttttttttttttttttt touch related end tttttttttttttttttttttttttttttttttttttttttttttt

// pppppppppppppppppppppppppppppppppppppppppppppp print to screen start pppppppppppppppppppppppppppppppppppppppppppp

void PrintField2Screen(int thefield, int size, uint16_t colour, uint16_t bkcolour, char *text){ 
  int fxx = areas[thefield - 1].Xstart;
  int fyy = areas[thefield - 1].Ystart;
  int fxlen = areas[thefield - 1].Xlen;
  int fylen = areas[thefield - 1].Ylen; 

  int fxtext = areas[thefield - 1].Xstart + areas[thefield - 1].Xtext;
  int fytext = areas[thefield - 1].Ystart + areas[thefield - 1].Ytext; 

  cyd.fillRect(fxx,fyy,fxlen,fylen,bkcolour);
  cyd.setTextSize(size);
  cyd.setTextColor(colour); 
  cyd.setCursor(fxtext,fytext);
  cyd.print(text);
}

void PrintText2Screen(int Xpos, int Ypos, int size, uint16_t colour, uint16_t bkcolour, char *text){
  int lenght = strlen(text);
  cyd.fillRect(Xpos-1,Ypos-1,(lenght *size *6),size * 8,bkcolour);
  cyd.setTextSize(size);
  cyd.setTextColor(colour); 
  cyd.setCursor(Xpos,Ypos);
  cyd.print(text);
}

void PrintInArea2Screen(int Xpos, int Ypos, int Xlen, int Ylen,int Xtpos, int Ytpos,int size, uint16_t colour, uint16_t bkcolour, const char *text) {
  cyd.fillRect(Xpos,Ypos,Xlen,Ylen,bkcolour);
  cyd.setTextSize(size);
  cyd.setTextColor(colour); 
  cyd.setCursor(Xtpos,Ytpos);
  cyd.print(text);
}

void PrintArea2Screen(int Xpos, int Ypos, int Xlen, int Ylen,int Xtpos, int Ytpos,int size, uint16_t colour, uint16_t bkcolour, const char *text) {
  int lenght = strlen(text);
  cyd.fillRect(Xpos,Ypos,Xlen,Ylen,bkcolour);
  cyd.setTextSize(size);
  cyd.setTextColor(colour); 
  cyd.setCursor(Xtpos,Ytpos);
  cyd.print(text);
}

void ShowArea(){ 
    for (int j = 0; j < Fieldmax; j++){  // Write the text
      int fff = fieldval[j].area - 1;
      int iarX = areas[fff].Xstart;         int iarY = areas[fff].Ystart;
      int iXb = iarX + fieldval[j].Xvbsta;  int iYb = iarY + fieldval[j].Yvbsta;
      int iXbl = fieldval[j].Xvblen;        int iYbl = fieldval[j].Yvblen;
      int iXv = iXb + fieldval[j].Xvoff;    int iYv = iYb + fieldval[j].Yvoff;
      int iXvl = fieldval[j].Xvlen;         int iYvl = fieldval[j].Yvlen;
      int fg = tCol(5); int bg = tCol(8);; int dg = tCol(17);
      int is = 1;
      char itx[3]; itoa(j+1, itx, 10);
      cyd.drawRoundRect(iXb, iYb, iXbl, iYbl, 5, dg);
      //cyd.drawRect(iXb, iYb, iXbl, iYbl, fg); // tCol(6));
      PrintInArea2Screen(iXv, iYv, iXvl, iYvl, iXv, iYv, is, fg, bg, itx);
    }
  }


// pppppppppppppppppppppppppppppppppppppppppppppp print to screen  end  pppppppppppppppppppppppppppppppppppppppppppp


// ddddddddddddddddddddddddddddddddddddddddddddddddd Divers  start  dddddddddddddddddddddddddddddddddddddddddddddddd

// Helper function for substring to hex conversion 
int x2i(char *s, int from_a, int to_b) {// String, starting position, endposition in string
  int x = 0;
  for(int a = from_a;  a <= to_b ;a++) {
 //   char c = *s;
    char c = s[a];
    if (c >= '0' && c <= '9') {   // Just the numbers
      x *= 16;
      x += c - '0'; 
    }
    else if (c >= 'A' && c <= 'F') {  // When using big letters
      x *= 16;
      x += (c - 'A') + 10; 
    }
    else if (c >= 'a' && c <= 'f') {  // When using low letters
      x *= 16;
      x += (c - 'a') + 10; 
    }
    else break;
    //s++;
  }
  return x;
}

void LedControl(){ 
  if (LEDsta_used) {
    LEDsta_BL = (LEDsta & 0b11);         // Extracting 2 bits starting from the rightmost side (Bit0 and Bit1)
    LEDsta_R = ((LEDsta >> 2) & 0b11);   // Shifting 2 bits to the right and then extracting the next 2 bits
    LEDsta_G = ((LEDsta >> 4) & 0b11);   // Shifting 4 bits to the right and then extracting the next 2 bits
    LEDsta_B = ((LEDsta >> 6) & 0b11);   // Shifting 6 bits to the right and then extracting the next 2 bits
    LEDsta_used = LOW;
  }
  ldr = (analogRead(CYD_LDR));
  bkl_now = bkl_set;

  switch(LEDsta_BL){
    // Used not dimmed    case 0:{digitalWrite(CYD_BL, Boff);break;} case 1:{digitalWrite(CYD_BL, Bon);break;}
    //                    case 2:{digitalWrite(CYD_BL, Ticker);break;} case 3:{digitalWrite(CYD_BL, !Ticker);break;} 
    // Here dimmed mode
    case 0:{bkl_now = 0 ;break;}    
    case 1:{ bkl_now = bkl_set;
      #if !defined(LCDtypeN)
        #if defined(TochSleep)
          if (millis() - bklonMillis >= bkl_NotSleep) { bkl_ON= false; bkl_now = 10; }   // Dimmed to 10
        #endif
      #endif  
      break; }
    case 2: {
      if (ldr >= 0 && ldr <= 10) {bkl_now = 255;}
      else if (ldr >= 11 && ldr <= 100) {bkl_now = 64;}      
      else if (ldr >= 101 && ldr <= 500) {bkl_now = 16;}
      else {bkl_now = 04;}
      break;}
    case 3: {
      if (!Ticker) {bkl_now = bkl_set;}
      else { bkl_now = 0;};break;}
    default: {break;}
  }
  if (bkl_now != bkl_last){analogWrite(CYD_BL,bkl_now); bkl_last = bkl_now;}
  switch (LEDsta_R)
    {                                  
    case 0:{digitalWrite(CYD_LED_RED, Loff);break;}
    case 1:{digitalWrite(CYD_LED_RED, Lon);break;}
    case 2:{digitalWrite(CYD_LED_RED, Ticker);break;}
    case 3:{digitalWrite(CYD_LED_RED, !Ticker);break;}
    default:{break;}
  }
  switch(LEDsta_G){                                  
    case 0:{digitalWrite(CYD_LED_GREEN, Loff);break;}
    case 1:{digitalWrite(CYD_LED_GREEN, Lon);break;}
    case 2:{digitalWrite(CYD_LED_GREEN, Ticker);break;}
    case 3:{digitalWrite(CYD_LED_GREEN, !Ticker);break;}
    default:{break;}
  }
  switch(LEDsta_B){                                  
    case 0:{digitalWrite(CYD_LED_BLUE, Loff);break;}
    case 1:{digitalWrite(CYD_LED_BLUE, Lon);break;}
    case 2:{digitalWrite(CYD_LED_BLUE, Ticker);break;}
    case 3:{digitalWrite(CYD_LED_BLUE, !Ticker);break;}
  default:{break;}
  }
}

uint16_t tCol(int colconst) {          // Transform easy mqtt colour number to ILI9341 unint_16 colour
  uint16_t result = 40;
  result = Colourtable[colconst];
  return result;
}  

void MakeColourTable() {              // Make mqtt colour definition easier
  Colourtable[0] = ILI9341_BLACK;           // 0x0000  //   0,   0,   0
  Colourtable[1] = ILI9341_BLUE;            // 0x001F  //   0,   0, 255
  Colourtable[2] = ILI9341_CYAN;            // 0x07FF  //   0, 255, 255
  Colourtable[3] = ILI9341_DARK;            // 050505  //   0,   5,   5 a very dark gray
  Colourtable[4] = ILI9341_DARKCYAN;        // 0x03EF  //   0, 125, 123
  Colourtable[5] = ILI9341_DARKGREEN;       // 0x03E0  //   0, 125,   0
  Colourtable[6] = ILI9341_DARKGREY;        // 0x7BEF  // 123, 125, 123
  Colourtable[7] = ILI9341_GREEN;           // 0x07E0  //   0, 255,   0
  Colourtable[8] = ILI9341_GREENYELLOW;     // 0xAFE5  // 173, 255,  41
  Colourtable[9] = ILI9341_LIGHTGREY;       // 0xC618  // 198, 195, 198
  Colourtable[10] = ILI9341_MAGENTA;        // 0xF81F  // 255,   0, 255
  Colourtable[11] = ILI9341_MAROON;         // 0x7800  // 123,   0,   0
  Colourtable[12] = ILI9341_NAVY;           // 0x000F  //   0,   0, 123
  Colourtable[13] = ILI9341_OLIVE;          // 0x7BE0  // 123, 125,   0
  Colourtable[14] = ILI9341_ORANGE;         // 0xF8 0xF// 255, 165,   0
  Colourtable[15] = ILI9341_PINK;           // 0xFC18  // 255, 130, 198
  Colourtable[16] = ILI9341_PURPLE;         // 0x780F  // 123,   0, 123
  Colourtable[17] = ILI9341_RED;            // 0xF800  // 255,   0,   0
  Colourtable[18] = ILI9341_WHITE;          // 0xFFFF  // 255, 255, 255
  Colourtable[19] = ILI9341_YELLOW;         // 0xFFE0  // 255, 255,   0
  Colourtable[20] = ILI9341_DARK_BLUE;      // 0xFFE0  //   0,   5,   5 a very dark blue
}

void WitchField2Screen(int thefield){
  int fxx;  
  int fyy;
  char ftext[3];
  itoa(thefield, ftext, 10);
  fxx = areas[thefield - 1].Xstart;
  fyy = areas[thefield - 1].Ystart;
  Serial.print(thefield);
  Serial.print(">>");
  Serial.print(fxx);
  Serial.print("/");
  Serial.print(fyy); 
  // Write number to positioned field
  PrintField2Screen(thefield, 2, 18, 2, ftext);
}

void What2DoInField(int field){
  switch (field) {
    case 1: { WitchField2Screen(field); break;}
    case 2: { WitchField2Screen(field); break;}
    case 3: { WitchField2Screen(field); break;}
    case 4: { WitchField2Screen(field); break;}
    case 5: { WitchField2Screen(field); break;}
    case 6: { WitchField2Screen(field); break;}
    case 7: { WitchField2Screen(field); break;}
    case 8: { WitchField2Screen(field); break;}
    case 9: { WitchField2Screen(field); break;}
    case 10: { WitchField2Screen(field); break;}
    case 11: { WitchField2Screen(field); break;}
    case 12: { WitchField2Screen(field); break;}
    case 13: { WitchField2Screen(field); break;}
    case 14: { WitchField2Screen(field); break;}
    case 15: { WitchField2Screen(field); break;}
    case 16: { WitchField2Screen(field); break;}
    default: { Serial.println(" Invalid field given"); break;}
  }
}

// ddddddddddddddddddddddddddddddddddddddddddddddddd Divers  end  dddddddddddddddddddddddddddddddddddddddddddd

//MMMMMM  MULTICORE APPLIKATION  MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM

void Core0Task( void * pvParameters ){
  int X_Raw, Y_Raw;
  Serial.print("Task0 running on second core ");
  Serial.println(xPortGetCoreID());
  #if defined(WithSensors)
    Wire.begin(SDA, SCL);
    Wire.setClock(100000);
    #if defined(LCDtypeC) 
      Wire1.begin(CST820_SDA, CST820_SCL);
    #endif
#endif
  MySensors = "";

  #if defined(BMP180)
    //Wire.begin(SDA, SCL);
    if (!bmp180.begin()) { Serial.println("begin() failed. check your BMP180 Interface and I2C Address."); }
    bmp180.resetToDefaults();
    bmp180.setSamplingMode(BMP180MI::MODE_UHR);
    Serial.println("BMP180 initialized");
    MySensors = MySensors + "BMP180 & ";

  #elif defined(BMP280)
    //Wire1.begin(SDA, SCL);
    unsigned bmp280status;
    bmp280status = bmp280.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID);
    if (!bmp280status) { 
      Serial.println(F("Could not find a valid BMP280 sensor.")); 
      //while (1) delay(10);
      MySensors = MySensors + "Error BMP280 & ";}
    else { Serial.print("BMP280 with sensorID: "); 
      Serial.print(bmp280.sensorID(),16);
      MySensors = MySensors + "BMP280 & ";
    }
    bmp280.setSampling (
      Adafruit_BMP280::MODE_NORMAL,  // Operating Mode
      Adafruit_BMP280::SAMPLING_X16,          // Temperature oversampling
      Adafruit_BMP280::SAMPLING_X16,          // Pressure oversampling
      Adafruit_BMP280::FILTER_X16,            // Filtering
      Adafruit_BMP280::STANDBY_MS_4000        // Standby time
    ); 
    Serial.println("  initialized");
  #endif
  vTaskDelay(300);

  #if defined(CCS811)
  Serial.print("CCS811 initializing with error : ");
  int status = ccs811.begin();
  Serial.println(status);
  if (status < 0) {
    Serial.println("Failed to start CCS811 sensor");
    MySensors = MySensors + "Error CCS811 & "; }
    else {
      ccs811.checkDataReady();
      vTaskDelay(500);
      MySensors = MySensors + "CCS811 & "; 
    }
  #elif defined(AGS10)
    //Wire1.begin(SDA, SCL);
    ags10.begin();
    MySensors = MySensors + "AGS10 & ";
  #endif

  #if defined(AS5600)
    //Wire1.begin(SDA, SCL);
    Serial.print("AS5600 Connect: "); Serial.println(as5600.getAddress());
    MySensors = MySensors + "AS5600 & ";
  #endif
  vTaskDelay(300);

  #if defined(LM75)
    Serial.println("LM75 started");
    MySensors = MySensors + "LM75 & ";
  #endif

  #if defined(MLX90614)
    //Wire1.begin(SDA, SCL);
    if (!mlx90614.begin(I2C_ADDRESS,&Wire)) { Serial.println("Error connecting to MLX sensor. Check wiring.");};
    vTaskDelay(100);
    Serial.print("MLX90614 Emissivity = "); Serial.println(mlx90614.readEmissivity());
    MySensors = MySensors + "MLX90614 & ";
    vTaskDelay(500);
  #endif
   MySensors = MySensors + "-";

  for(;;) {  // The loop for Task0 starts here loop_task0__loop_task0__loop_task0__loop_task0__loop_task0__loop_task0__
    Serial.println();

    #if defined(BMP180)
      if (!bmp180.measureTemperature()) { Serial.println("BMP180 still reading"); return;}
      do { vTaskDelay(350); } while (!bmp180.hasValue());                                                          
      sensor_temp = bmp180.getTemperature();
      Serial.print("BMP180 Temperature = "); Serial.print(sensor_temp); Serial.print(" *C  and ");

      if (!bmp180.measurePressure()) { Serial.println("BMP180 still reading"); return;}
      do { vTaskDelay(350); } while (!bmp180.hasValue()); 
      sensor_pressure = bmp180.getPressure()/ 100;  // change pascal to mBar
      Serial.print("Pressure: "); Serial.print(sensor_pressure); Serial.println(" Pa");
    #elif defined(BMP280)
      sensor_temp = bmp280.readTemperature();
      sensor_pressure = bmp280.readPressure()/ 100;  // change pascal to mBar
      Serial.print(F("BMP280  Temperature = ")); Serial.print(sensor_temp); Serial.print(" *C  and  "); 
      Serial.print(F("Pressure = ")); Serial.print(sensor_pressure); Serial.print(" Pa");
      Serial.println();
    #endif

    #if defined(CCS811)
      if(ccs811.checkDataReady()){
        sensor_co2 = ccs811.getCO2PPM();
        sensor_tvoc = ccs811.getTVOCPPB();
        Serial.print("CCS811  CO2: ");  Serial.print(sensor_co2);
        Serial.print(" ppm  and  TVOC: "); Serial.print(sensor_tvoc);Serial.println(" ppb");}
      else { Serial.println("CCS811 read ERROR!"); }
    #elif defined(AGS10)
      sensor_tvoc = ags10.readTVOC();
      Serial.print(F("AGS10  TVOC = "));
      Serial.println(sensor_tvoc);
    #endif

    #if defined(AS5600)
      sensor_angle = as5600.getRawAngle() * 0.087890625;
      Serial.print("AS5600  Angle = ");         Serial.print(sensor_angle);
      Serial.print("  and  Magnitude = ");       Serial.println(as5600.getMagnitude());
    #endif

    #if defined(LM75)
      sensor_temp = lm75.readTemperatureC();
      Serial.print("LM75  Temperature = ");
      Serial.print(sensor_temp);
      Serial.println(" *C");
    #endif
    
    #if defined(MLX90614)
      int a= mlx90614.readAmbientTempF();  // Just to activate the chip ??
      sensor_tempa = mlx90614.readAmbientTempC();
      sensor_tempo = mlx90614.readObjectTempC();
      Serial.print("MXL90614  Ambient = "); Serial.print(sensor_tempa); Serial.print(" *C  and  Object = ");
       Serial.print(sensor_tempo); Serial.println(" *C");
    #endif
    #if !defined(LCDtypeN)
      ShowSensorsOnLocalScreen();
    #endif
    vTaskDelay(2500);
  }   
}
// MMMMMM  end of MULTICORE APPLIKATION  MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM


// XXXXXXXXXXXXXXXXXXXXXXXX PROGRAM  START XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

void setup() {
  // Initialize onboard LEDs with "active low" -->  HIGH == off, LOW == on
  adc_attenuation_t attenuation = ADC_11db;
  analogSetAttenuation(attenuation);
  pinMode(CYD_LDR, INPUT);              // Measure suround light.
  pinMode(CYD_LED_BLUE, OUTPUT);
  pinMode(CYD_LED_GREEN, OUTPUT);
  pinMode(CYD_LED_RED, OUTPUT);
  digitalWrite(CYD_LED_RED, Loff);    
  digitalWrite(CYD_LED_GREEN, Loff);
  digitalWrite(CYD_LED_BLUE, Loff);

   // Initialize debug output and wifi and preset mqtt
  Serial.begin(115200);
  Serial.print("Base Task1 running on core ");        // Arduino compiles normaly for core 1 !!!
  Serial.println(xPortGetCoreID());

  SPI.begin(HSPI_SCK, HSPI_MISO, HSPI_MOSI);
  
  // Start screen
  pinMode(CYD_BL, OUTPUT);
  //digitalWrite(CYD_BL,Bon);           // As Adafruit library does not set backlight on
  analogWrite(CYD_BL,bkl_set);
 
  cyd.begin();                          // Display with LED in  -lower-left-  corner
  cyd.setRotation(1);
  cyd.writeCommand(ILI9341_GAMMASET); //Gamma curve selected
  cyd.write(2);
  vTaskDelay(120);
  cyd.writeCommand(ILI9341_GAMMASET); //Gamma curve selected
  cyd.write(1);
  cyd.fillScreen(ILI9341_BLACK); 
  cyd.setCursor(0, 0);
  // cyd.setFont(&Picopixel);
  Serial.println("");
  Serial.print("I am: ");
  Serial.print(iamclient);

  #if defined(LCDtypeC)
    Serial.println(" a CYD ILI9341 with CST820 touch.");
  #elif defined(LCDtypeR)
    Serial.println(" a CYD ILI9341 with XPT2046 touch.");
  #elif defined(LCDtypeN)
    Serial.println(" a CYD ILI9341 with no touch.");
  #endif
  Serial.println("");
  cyd.setTextColor(ILI9341_WHITE);
  cyd.setTextSize(1);
  cyd.setCursor(0, 0);
  cyd.println("I am a CYD ILI9341");
  #if defined(LCDtypeC)
    cyd.println("with CST820 touch:");
  #elif defined(LCDtypeR)
    cyd.println("with XPT2046 touch:");
   #elif defined(LCDtypeN)
    cyd.println("with no touch:");
  #endif
  cyd.setTextColor(ILI9341_GREEN); 
  cyd.setTextSize(2); 
  cyd.setCursor(120, 0);
  cyd.print(iamclient);

  setup_wifi();                     // Start the wifi connection
  vTaskDelay(100);
  mqttclient.setClient(espClient);
  mqttclient.setServer(mqtt_server, mqtt_port);
  mqttclient.setCallback(callback);
  mqttclient.setKeepAlive(15);      // MQTT_KEEPALIVE : keepAlive interval in seconds. Override setKeepAlive()
  mqttclient.setSocketTimeout(15);  // MQTT_SOCKET_TIMEOUT: socket timeout interval in Seconds. Override setSocketTimeout()
  void reconnect();                 // Start the mqtt connection
  mqttclient.subscribe(in_topic);   // Listen to the mqtt inputs
 
  MakeColourTable();                // Prepare easy colour definition and 
  MakeScreenTable();                // Write first static screen

  // MMMMMMMMMMMNNMM  MULTICORE start  ==  Application is in -->  void Core0Task() MMMMMMMMMMMMMMMMMMMMMMM

  taskCreationResult = xTaskCreatePinnedToCore(  // Second core = core0 ... as in Arduino the standart core is core1
    Core0Task,      // Task function.
    "Core0Task",    // name of task. 
    8000,           // Stack size of task
    NULL,           // parameter of the task
    1,              // priority of the task
    &Task0,         // Task handle to keep track of created task
    0               // pin task to core 0
  );

  if (taskCreationResult != pdPASS) { Serial.println("Error creating task!");} 
  else { Serial.println("Aditional Task0 created successfully"); }

  // MMMMMMMMMMMMMMMMMMMMMMMMMMM  MULTICORE  end MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM

  // Finaly start touch hardware
  #if defined(LCDtypeC)
    touch.init(CST820_SDA, CST820_SCL, CST820_RST, CST820_IRQ);	// sda, scl, rst, irq
    int iType = touch.sensorType();
    Serial.printf("Sensor type = %s\n", szNames[iType]);
  #elif defined LCDtypeR
    touchHW.begin();
    touchHW.setRotation(1);       // LED is in lower left corner
  #endif
}

//  LLLLLLLLLLLLLLLL   This is the main default loop of task1  LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
void loop() {
  //Serial.print("loop() running on core "); Serial.println(xPortGetCoreID());

  unsigned long currentMillis = millis();           // Ticker for blinking led etc.
  if (currentMillis - prevTickerMillis >= 500) {
    prevTickerMillis = currentMillis;
    Ticker = !Ticker;
  }

  // Every X number of seconds (interval = x milliseconds) it reads a new MQTT message
  if (currentMillis - prevRMQTTMillis >= readinterval) {
    prevRMQTTMillis = currentMillis;

    if (WiFi.status() == WL_CONNECTED) { Serial.print("+");}
    else {
      WiFi.begin(wifi_ssid, wifi_password);
      Serial.println("Try Wifi reconnect "); 
    }
   
    if (!mqttclient.connected()) {      
      reconnect();                                // In case no mqtt it will reconnect
      mqttclient.subscribe(in_topic);
      Serial.print("Go in loop with topic: " );
      Serial.println(in_topic);
      #if defined(TEST)
        mqttclient.publish(out_status, "2" ,false);
      #else
        mqttclient.publish(out_status, "Reconnected" ,false);
      #endif
      statusreset = true;
    }
    mqttclient.loop();                            // Request if there is a message
    
    watchdog = !watchdog;                         // Create toggeling watchdog signal
    mqttclient.publish(out_watchdog, String(watchdog).c_str() ,false); 

    if (statusreset){
      statusreset = false;                  
      // vTaskDelay(1000);                        // Often used in non screen application to make error messages readable
      #if defined(TEST)
        mqttclient.publish(out_status, "0" ,false);
      #else
        mqttclient.publish(out_status, "Normal" ,false);
      #endif
    }
    // Here are the sensordata send via mqtt when exist
    // It is intentional that a send is only done within receive interval. (To avoid unchecked connection abd with buffer problems)
    #if defined(WithSensors)
      if (currentMillis - prevSMQTTMillis >= sendinterval) {
        prevSMQTTMillis = currentMillis;
        if (sensor_angle != 9876) {mqttclient.publish(mqtt_out_angle, String(sensor_angle).c_str(), false);}      // AS5600 angle position
        if (sensor_co2 != 9876) {mqttclient.publish(mqtt_out_co2, String(sensor_co2).c_str(), false);}            // ASG10 air co2 ppm
        if (sensor_tvoc != 9876) {mqttclient.publish(mqtt_out_tvoc, String(sensor_tvoc).c_str(), false);}         // ASG10 air quality
        if (sensor_pressure != 9876) {mqttclient.publish(mqtt_out_press, String(sensor_pressure).c_str(), false);} // BMPxxx air pressure 
        if (sensor_humidity != 9876) {mqttclient.publish(mqtt_out_hum, String(sensor_humidity).c_str(), false);}  // Humidity sensor 
        if (sensor_temp != 9876) {mqttclient.publish(mqtt_out_temp, String(sensor_temp).c_str(), false);}         // BMPxxx temperatur 
        if (sensor_tempa != 9876) {mqttclient.publish(mqtt_out_tempa, String(sensor_tempa).c_str(), false);}      // MXL90614 ambient temp 
        if (sensor_tempo != 9876) {mqttclient.publish(mqtt_out_tempo, String(sensor_tempo).c_str(), false);}      // MXL90614 meassured temp
      }
    #endif
  }
  #if defined(LCDtypeR)                   // Version resistive
    if (touchHW.touched()) {              // Only when touch event
      digitalWrite(CYD_LED_BLUE, Lon);
      TS_Point p = touchHW.getPoint();
      Xraw = p.x; 
      Yraw = p.y;
      findTouchPos();}
    else { 
      digitalWrite(CYD_LED_BLUE, Loff);  
    }
  #elif defined(LCDtypeC)                 // Version capacitive
    if (touch.getSamples(&ti)) {          // Only when touch event
      digitalWrite(CYD_LED_BLUE, Lon);
      Xraw = ti.y[0];                     // ATTENTION X and Y are swapped        
      Yraw = YTmax- ti.x[0];              // ATTENTION X and Y are swapped 
      findTouchPos();
      }
    else { 
      digitalWrite(CYD_LED_BLUE, Loff);  
    }
  #endif 

  LedControl();                           // Jump to controls internal led and backlight

  #if defined(TEST)
  // mqttclient.publish(out_topic, String(looped).c_str() ,false); // Only use that when in doubts of loop timing
    looped++;
  #endif
  
  vTaskDelay(100);                        // Also to slow down the touch
}
