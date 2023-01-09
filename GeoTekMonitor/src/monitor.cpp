/*  
    Author: Odinkaru Steve for GeoTek
    Hand Pump Monitoring using ADXL3xx Accelerometer
    See hardware setup here https://docs.arduino.cc/built-in-examples/sensors/ADXL3xx
*/ 

// Please select the corresponding model

// #define SIM800L_IP5306_VERSION_20190610
#define SIM800L_AXP192_VERSION_20200327
// #define SIM800C_AXP192_VERSION_20200609
// #define SIM800L_IP5306_VERSION_20200811

// Define the serial console for debug prints, if needed
#define DUMP_AT_COMMANDS
#define TINY_GSM_DEBUG          SerialMon
#include<string.h>
#include "utilities.H"

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial
// Set serial for AT commands (to the module)
#define SerialAT  Serial1

// Configure TinyGSM library
#define TINY_GSM_MODEM_SIM800          // Modem is SIM800
#define TINY_GSM_RX_BUFFER      1024   // Set RX buffer to 1Kb

#include <TinyGsmClient.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

String testSms = "This is a test.";
String smsMsg1 = "Today's average is less than the total number of turns";
String smsMsg2 = "Today's average is greater than the total number of turns";
#define SMS_TARGET  "+234xxxxxxxxx" // This is the number that will recieve the text message. 
#define relay_pin 12

//---------------------------------------------------------------------------------------------------------

const int groundpin = 34;             // analog input pin 4 -- ground
const int powerpin = 35;              // analog input pin 5 -- voltage
const int xpin = A3;                  // x-axis of the accelerometer
const int zpin = A10;                  // y-axis

// Take multiple samples pf accelerometer to reduce noise
const int samples = 10;

// Get the minimum and maximum values of y and z axis
int xMin = 0;
int xMax = 0;

int zMin = 0;
int zMax = 0;

//---------------------------------------------------------------------------------------------------------
double aveTotal = 0;
double total = 0;
void setup()
{

  // Set console baud rate
  SerialMon.begin(115200);
  pinMode(relay_pin,OUTPUT);
  delay(10);

  // Start power management
  if (setupPMU() == false) {
    Serial.println("Setting power error");
  }

  // Some start operations
  setupModem();

  // Set GSM module baud rate and UART pins
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);

  delay(6000);

  SerialMon.println("Initializing modem...");
  modem.restart();
  // modem.init();

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);
  delay(1000);

  // Provide ground and power by using the analog inputs as normal digital pins.

  // This makes it possible to directly connect the breakout board to the

  // Arduino. If you use the normal 5V and GND pins on the Arduino,

  // you can remove these lines.

  pinMode(groundpin, OUTPUT);
  pinMode(powerpin, OUTPUT);

  digitalWrite(groundpin, LOW);
  digitalWrite(powerpin, HIGH);

  // Send test sms
  modem.sendSMS(SMS_TARGET, testSms);
}

void loop()
{
  Serial.print(analogRead(xpin));
  Serial.print(analogRead(zpin));

  int xRaw=0,yRaw=0,zRaw=0;
  for(int i=0;i<samples;i++){
    xRaw+=analogRead(xpin);
    zRaw+=analogRead(zpin);
  }

  //Get average using samples
  xRaw/=samples; 
  zRaw/=samples;
  //--------------------------------------------------------------
  //Convert raw values to 'milli-Gs"
  //Convert value of RawMin to -1000
  //Convert value of RawMax to 1000
  long xMilliG = map(xRaw, xMin, xMax, -1000, 1000);
  long zMilliG = map(zRaw, zMin, zMax, -1000, 1000);
  //--------------------------------------------------------------
  // re-scale to fractional Gs

  float x_g_value = xMilliG / 1000.0;
  float z_g_value = zMilliG / 1000.0;
  for (; x_g_value > 0 && z_g_value > 0; ){
    if (x_g_value > 0 && z_g_value > 0) {
    total += 1;
    }
  }
  
  /*
      After getting average total of number of turns for a handpump in a day, juxtapose values with x and z
  */
  if (aveTotal < total){
    modem.sendSMS(SMS_TARGET, smsMsg1);
  }
  else{
    modem.sendSMS(SMS_TARGET, smsMsg2);
  }

  delay(2000);
}
