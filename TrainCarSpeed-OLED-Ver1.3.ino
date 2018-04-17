
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/*
Arduino Pro Mini ATmega328P 5V 16Mhz

SDA A4
SCL A5
VCC and GND to external 5v
*/

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2


#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#define irSensor 2//pin connected to IR Sensor

//storage variables
float diameter = 33;// wheel size

long lcdInterval = 250;   // Screen refresh interval
int maxIrSensorCounter=10;   // Adjust if speed spikes happening
int noPulses=1500;   // Time before assumed stopped
int timeout = 20000;  // Time before "Screen Saver"
int dispTimer = 0; // Timer to switch display
int erased = 0;

const int numReadings = 10;

int readings[numReadings]; // the readings from the analog input
int readIndex = 0;         // the index of the current reading
float total = 0.00;           // the running total
float average = 0.00;         // the average

int irSensorVal;
long timer=0;
float mph=0.00;
float maxmph=0.00;
float distance=0.00;
float actdis=0.00;
float circumference;
int irSensorCounter;
const int buttonMPin=3;  //Mode Button
int buttonMCounter=0;
long buttonMTimer=0;

boolean currentstate; // Current state of IR input scan
boolean prevstate; // State of IR sensor in previous scan

long previousLCDMillis = 0;

int resetPin = 6;

void setup()   {                
  digitalWrite(resetPin, HIGH);
  delay(200);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
   // init done
  
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(2000);

  // Clear the buffer.
  display.clearDisplay();
 
  irSensorCounter = maxIrSensorCounter;
  circumference = 3.14*diameter;
  pinMode(irSensor, INPUT);
  pinMode(buttonMPin, INPUT);  //Set Interupt Mode Button
  attachInterrupt(digitalPinToInterrupt(buttonMPin),selection1, RISING);

  prevstate = 1;
 
  cli();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  
  OCR1A = 1999;// = (1/1000) / ((1/(16*10^6))*8) - 1
  
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS11);   
  TIMSK1 |= (1 << OCIE1A);
  
  sei();//allow interrupts
  //END TIMER SETUP

Serial.begin(115200);

  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }

}

ISR(TIMER1_COMPA_vect) {//Interrupt at freq of 1kHz to measure irSensor switch
  currentstate = digitalRead(irSensor); // Read IR sensor state
  if(prevstate != currentstate) // If there is change in input
    {
      prevstate = currentstate; // store this scan (prev scan) data for next scan
      if (irSensorCounter == 0 && currentstate == 0){
        mph = (56.82*float(circumference))/float(timer);//calculate miles per hour
        if (mph > maxmph)
        {
          maxmph = mph;
        }
        timer = 0;//reset timer
        irSensorCounter = maxIrSensorCounter;//reset irSensorCounter
      }
      else{
        if (irSensorCounter > 0){//don't let irSensorCounter go negative
        irSensorCounter -= 1;//decrement irSensorCounter
      }
    }
  }
  else{//if irSensor switch is open
    if (irSensorCounter > 0){//don't let irSensorCounter go negative
      irSensorCounter -= 1;//decrement irSensorCounter
    }
  }
  if (timer > noPulses){
    mph = 0;  //if no new pulses from irSensor switch- tire is still, set mph to 0
    timer += 1;  //increment timer
  }
  else{
    timer += 1;  //increment timer
  }
 distance +=mph;
 actdis = distance/3600000/87.1;
 dispTimer += 1;
}

void displayMPH(){
  Serial.write(12);//clear
  Serial.print(mph);
  Serial.print("\t");
  Serial.print(average);
  Serial.print("\t");
  Serial.print(maxmph);
  Serial.print("\t");
  Serial.print(distance/3600000);
  Serial.print("\t");
  Serial.print(actdis);
  Serial.print("\t");
  Serial.print(currentstate);
  Serial.print("\t");
  Serial.print(prevstate);
  Serial.print("\t");
  Serial.print(buttonMCounter);
  Serial.print("\t");
  Serial.print(buttonMTimer);
  Serial.print("\t");
  Serial.print(irSensorCounter);
  Serial.print("\t");
  Serial.print(timer);
  Serial.print("\t");
  Serial.print(previousLCDMillis);
  Serial.println("");
}

void selection1(){
  if (buttonMTimer < millis()){
    buttonMTimer = millis()+1000;// Debounce Timer
    buttonMCounter++;
    timer = 0;
  }
}

void displayOLED(){
  display.stopscroll();
  display.clearDisplay();
  display.setTextColor(WHITE);
  if (buttonMCounter == 0){
    displayOLED1();
  }
  if (buttonMCounter == 1){
    displayOLED2();
  }
  if (buttonMCounter == 2){
    displayOLED3();      
  }
}
  
void displayOLED1(){ //Cycle through speed, distance, max and actual 
  if (0 < dispTimer && dispTimer < 5000){
    display.setCursor(3,2);
    display.setTextSize(4);
    display.print(average,0);
    display.setTextSize(3);
    display.setCursor(72,6);
    display.print("MPH");
    display.display();
    erased = 0;
  }
  if (5001 < dispTimer && dispTimer < 8000){
    display.setCursor(4,2);
    display.setTextSize(2);
    display.print(distance/3600000);
    display.setCursor(4,21);
    display.setTextSize(1);
    display.print("Miles");
    display.setCursor(76,2);
    display.setTextSize(2);
    display.print(maxmph,0);
    display.setCursor(76,21);
    display.setTextSize(1);
    display.print("Max MPH");
    display.display();
    erased = 0;
  }
  if (8001 < dispTimer && dispTimer < 13000){
    display.setCursor(3,2);
    display.setTextSize(4);
    display.print(average,0);
    display.setTextSize(3);
    display.setCursor(72,6);
    display.print("MPH");
    display.display();
    erased = 0;
  }
  if (13001 < dispTimer && dispTimer < 16000){
    display.setTextSize(2);
    display.setCursor(4,2);
    display.print(average/87.1);
    display.setTextSize(1);
    display.setCursor(4,21);
    display.print("Real MPH");
    display.setCursor(76,2);
    display.setTextSize(2);
    display.print(actdis*5280,0);
    display.setCursor(76,21);
    display.setTextSize(1);
    display.print("Feet");
    display.display();
    erased = 0;
  }
  if (dispTimer > 16001){
    dispTimer = 0;
  }
}

void displayOLED2(){  // Display just scale speed
  display.drawRoundRect(0,0,128,32,3,WHITE);
  display.setCursor(6,6);
  display.setTextSize(3);
  display.print(average,0);
  display.setTextSize(3);
  display.setCursor(72,6);
  display.print("MPH");
  display.display();
  erased = 0;
}

void displayOLED3(){  // display just actual speed and distance
  display.setTextSize(2);
  display.setCursor(4,2);
  display.print(average/87.1);
  display.setTextSize(1);
  display.setCursor(4,21);
  display.print("Real MPH");
  display.setCursor(76,2);
  display.setTextSize(2);
  display.print(actdis*5280,0);
  display.setCursor(76,21);
  display.setTextSize(1);
  display.print("Feet");
  display.drawCircle(117,25,5,WHITE);
  display.drawPixel(115,23,WHITE);
  display.drawPixel(119,23,WHITE);
  display.drawPixel(114,26,WHITE);
  display.drawPixel(120,26,WHITE);
  display.drawPixel(115,27,WHITE);
  display.drawPixel(119,27,WHITE);
  display.drawLine(116,28,118,28,WHITE);
  display.display();
  erased = 0;
}

void loop(){
  displayMPH();
  if (buttonMCounter > 2) buttonMCounter=0;
  unsigned long currentLCDMillis = millis();

  // subtract the last reading:
  total = total - readings[readIndex];
  // read from the sensor:
  readings[readIndex] = mph;
  // add the reading to the total:
  total = total + readings[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;

  // if we're at the end of the array...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }
  // calculate the average:
  average = total / numReadings;

  if (timer > timeout){
    screenSaver();
  }
  else if(currentLCDMillis - previousLCDMillis > lcdInterval){
    previousLCDMillis = currentLCDMillis;
    displayOLED();
  }
}

void screenSaver(){
    if (erased == 0){
      display.clearDisplay();
      display.display();
      erased = 1;
    }
    display.setTextSize(2);
    display.setTextColor(WHITE);
/*    
    display.setCursor(12,0);
    display.print("HenryTown");
    display.setCursor(0,16);
    display.print("Freight");
*/
    display.setCursor(14,0);
    display.print("Shelly");
    display.setCursor(2,16);
    display.print("ShortLine");

    display.drawLine(120,3,122,3,WHITE);
    display.drawCircle(114,7,3,WHITE);
    display.drawPixel(119,4,WHITE);
    display.drawPixel(123,4,WHITE);
    display.drawPixel(118,5,WHITE);
    display.drawPixel(124,5,WHITE);
    display.drawPixel(113,6,WHITE);
    display.drawLine(125,6,125,9,WHITE);
    display.drawLine(113,8,113,9,WHITE);
    display.drawPixel(117,9,WHITE);
    display.drawPixel(126,9,WHITE);
    display.drawLine(117,10,126,10,WHITE);
    display.drawPixel(117,11,WHITE);
    display.drawPixel(120,11,WHITE);
    display.drawPixel(122,11,WHITE);
    display.drawPixel(125,11,WHITE);
    display.drawLine(118,12,119,12,WHITE);
    display.drawLine(123,12,124,12,WHITE);
    
    display.display();
    while(timer > timeout){
      display.startscrollleft(0x00, 0x0F);
    }
    
//    lcd.print(" L.C.S. of M.E ");
//    lcd.print("  Mundelein, IL");
}

