#include <math.h>
#include <PID_v1.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "average.h"

#define RELAY_PIN          ( 5 )
#define THERMISTOR_PIN     ( A2 )
#define INC_BUTTON_PIN     ( 6 )
#define DEC_BUTTON_PIN     ( 9 )
#define ADC_REF            ( 5000 )
#define ADC_RESOLUTION     ( 1023)
#define DIV_FIXED_RESISTOR ( 10000 )

// Pointers to the three below are passed to both PID and AutoTune
Adafruit_SSD1306 display   = Adafruit_SSD1306(); 
double input               = 20.0;
double output              = 0;
double output_ms           = 2500.0;
double setpoint            = 32.0;
Average* avg;

// PID Coeffs
double kp                  = 30;
double ki                  = 0;
double kd                  = 0;

// This is the size of our PWM window in mSec
int relayWindowSize        = 5000;

// PID
PID       rimsPID ( &input, &output, &setpoint, kp, ki, kd, DIRECT );

// Dem timez
unsigned long serialTime;
unsigned long relayWindowStartTime;

// Button flags
bool incButtonHasBeenPressed = false;
bool decButtonHasBeenPressed = false;

void pinStr( uint32_t ulPin, unsigned strength) // works like pinMode(), but to set drive strength
{
    // Handle the case the pin isn't usable as PIO
    if ( g_APinDescription[ulPin].ulPinType == PIO_NOT_A_PIN )
    {
        return;
    }
    
    if(strength) strength = 1;
    
    PORT->Group[g_APinDescription[ulPin].ulPort].PINCFG[g_APinDescription[ulPin].ulPin].bit.DRVSTR = strength ;
}

void doButtons()
{
    // INCrement button first
    if     ( !incButtonHasBeenPressed && ( digitalRead( INC_BUTTON_PIN ) == 0 ) )
    {
        incButtonHasBeenPressed = true;
    }
    else if(  incButtonHasBeenPressed && ( digitalRead( INC_BUTTON_PIN ) == 1 ) )
    {
        setpoint += 1.0;
        incButtonHasBeenPressed = false;
    }

    // DECrement button next
    if     ( !decButtonHasBeenPressed && ( digitalRead( DEC_BUTTON_PIN ) == 0 ) )
    {
        decButtonHasBeenPressed = true;
    }
    else if(  decButtonHasBeenPressed && ( digitalRead( DEC_BUTTON_PIN ) == 1 ) )
    {
        setpoint -= 1.0;
        decButtonHasBeenPressed = false;
    }
}

void doDisplay()
{
    String out1 = "CURRENT:  "+String(input);
    String out2 = "SETPOINT: "+String(setpoint);
    String out3 = "PWM:      "+String(output);
    
    display.clearDisplay(); // Clear the display
    
    display.setTextSize(1); //Set our text size, size 1 correlates to 8pt font
    display.setTextColor(WHITE); //We're using a Monochrome OLED so color is irrelevant, pixels are binary.
    display.setCursor(0,0); //Start position for the font to appear
    display.println(out1);
    display.println(out2);
    display.println(out3);
    display.display();
}

void SerialSend()
{
    Serial.print("setpoint: ");Serial.print(setpoint); Serial.print(" ");
    Serial.print("input: ");Serial.print(input); Serial.print(" ");
    Serial.print("output: ");Serial.print(output); Serial.print(" ");

    Serial.print("kp: ");Serial.print(rimsPID.GetKp());Serial.print(" ");
    Serial.print("ki: ");Serial.print(rimsPID.GetKi());Serial.print(" ");
    Serial.print("kd: ");Serial.print(rimsPID.GetKd());Serial.println();
}

void setup()
{
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // initialize with the I2C address 0x3C (for the 128x32)
    display.clearDisplay(); // Clear the display
    
    // Setup the IO
    pinMode(RELAY_PIN, OUTPUT);
    pinStr(RELAY_PIN, HIGH);
    
    pinMode(THERMISTOR_PIN, INPUT);
    pinMode(INC_BUTTON_PIN, INPUT);
    pinMode(DEC_BUTTON_PIN, INPUT);

    digitalWrite(INC_BUTTON_PIN, HIGH);
    digitalWrite(DEC_BUTTON_PIN, HIGH);

    // Setup the pid
    rimsPID.SetMode(AUTOMATIC);
    rimsPID.SetOutputLimits(0, 100);

    serialTime = 0;
    relayWindowStartTime = millis();

    Serial.begin(9600);
    
    avg = new Average();
}

double getTemperature()
{
    double temperature;

    double vout       = (double)( ( (double)analogRead( THERMISTOR_PIN ) / ADC_RESOLUTION ) * ADC_REF );
    double resistance = (double)( vout * DIV_FIXED_RESISTOR ) / (double)( ADC_REF - vout );

    // Using B-Parameter equation: 1/T = 1/T0 + 1/B*log(R/R0)
    temperature  = resistance / DIV_FIXED_RESISTOR;     // (R/Ro)
    temperature  = log(temperature);                    // ln(R/Ro)
    temperature /= 3950;                                // 1/B * ln(R/Ro)
    temperature += 1.0 / (25 + 273.15);                 // + (1/To)
    temperature  = 1.0 / temperature;                   // Invert
    temperature -= 273.15;                              // convert to C

    avg->addValue( temperature );
    
    return avg->getAverage();
}

void loop()
{
    input = getTemperature();

      //////////////
     // #pidlife //
    //////////////

    rimsPID.Compute();

      /////////////////////////////
     // Make display... display //
    /////////////////////////////
    doDisplay();
    

      /////////////////////////
     // Check button statez //
    /////////////////////////

    doButtons();

      /////////////////////////////
     // Handle the Relay Output //
    /////////////////////////////

    if( ( millis() - relayWindowStartTime ) > relayWindowSize )
    {
        // Time to shift the Relay Window
        relayWindowStartTime += relayWindowSize;
    }

    output_ms = output * 50;

    // Do we switch on or off?
    output_ms < ( millis() - relayWindowStartTime ) ? digitalWrite(RELAY_PIN, LOW) : digitalWrite(RELAY_PIN, HIGH);

      ////////////////////////
     // Write Serial Debug //
    ////////////////////////

    if( millis() > serialTime )
    {
        SerialSend();
        serialTime += 1000;
    }
}
