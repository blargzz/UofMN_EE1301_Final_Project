/*****************************************************************************************|
* HP 3580a analog X-Y pen plotter to HP-GL  translator. 
* (Hewlett-Packard Graphics Language http://en.wikipedia.org/wiki/HPGL)
*/

#ifdef ARDUINO_UNOR4_WIFI
#include "Peakdetector.h"
//******************************************
#include <Arduino_LED_Matrix.h>
ArduinoLEDMatrix matrix;
//******************************************
#endif

#define plotmin_x 0
#define plotmax_x 7650
#define plotmin_y 0
#define plotmax_y 7650
#define scalemin_x 0
#define scalemax_x 1023
#define scalemin_y 0
#define scalemax_y 1023

#define mkrboxmin_x 2650
#define mkrboxmax_x 10300
#define mkrboxmin_y 0
#define mkrboxmax_y 7650
#define mkrscalemin_x 0
#define mkrscalemax_x 354
#define mkrscalemin_y 0
#define mkrscalemax_y 1023

#define ADCref
#define ADCx A1
#define ADCy A2

#define black 1
#define blue 2
#define green 3
#define red 4

//    #define num_peaks 10

int grat_x = 0;
int grat_y = 0;
int center_x = 0.5 * scalemax_x;
int center_y = 0.5 * scalemax_y;

struct point {
  uint16_t x;
  uint16_t y;
};
point plot;

#ifdef ARDUINO_UNOR4_WIFI
//****************************************BEGIN IMPORTED CODE FROM GROK (modified) FOR PEAK DETECTION

//  CODE IS IN "Peakdetector.h"
SpectrumPoint fftData[FFT_SIZE];
SpectrumPoint detectedPeaks[MAX_PEAKS];
/*
void loop() {
  // ... your code fills fftData[0..1023] with latest spectrum ...

  uint8_t numPeaks = findSpectrumPeaks(fftData, detectedPeaks);

  Serial.printf("Found %d peaks:\n", numPeaks);
  for (int i = 0; i < numPeaks; i++) {
    Serial.printf("  Peak %d: Bin %d, Value %d\n",
                  i+1, detectedPeaks[i].bin, detectedPeaks[i].value);
  }

  delay(100);
}
*/
//******************************************
const int sensePin = A3;        // pin to monitor
const float threshold = 0.30;   // voltage threshold (adjust if needed)

// 12×8 bitmap for "Y" (currently blank as you gave it)
uint8_t Y_bitmap[12][8] = {
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0}
};

// 12×8 solid "N"
uint8_t N_bitmap[12][8] = {
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1}
};

// Blank bitmap for blinking "off"
uint8_t N_blank[12][8] = {
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0}
};

bool blinkState = false;      // tracks whether N is ON or OFF
unsigned long lastBlink = 0;  // last blink timestamp

void runMatrixIndicator() {
    int adc = analogRead(sensePin);
    float voltage = adc * (3.3 / 4095.0);

    if (voltage > threshold) {
        matrix.renderBitmap(Y_bitmap, 12, 8);
    } else {
        unsigned long now = millis();
        if (now - lastBlink > 300) {
            blinkState = !blinkState;
            lastBlink = now;
        }

        if (blinkState) {
            matrix.renderBitmap(N_bitmap, 12, 8);
        } else {
            matrix.renderBitmap(N_blank, 12, 8);
        }
    }
}

//******************************************END IMPORTED CODE FROM GROK (modified) FOR PEAK DETECTION
#endif

void setup() {
  // put your setup code here, to run once:
  // Initialize Serial output, plot area, scale factor and pen color
  Serial.begin(115200);
  //******************************************
  matrix.begin();
  //******************************************
  Serial.println("IN;");
  Serial.println("IP" + String(plotmin_x) + "," + String(plotmin_y) + "," + String(plotmax_x) + "," + String(plotmax_y) + ";");
  Serial.println("SC" + String(scalemin_x) + "," + String(scalemax_x) + "," + String(scalemin_y) + "," + String(scalemax_y) + ",1;");
  Serial.println("SP" + String(black) + ";");

  //Draw the major graticule lines, horizontal and vertical
  for (int i = 0; i <= 10; i++) {
    grat_x = (0.1 * i) * scalemax_x;
    grat_y = (0.1 * i) * scalemax_y;
    //Plot vertical graticule
    Serial.print("PU" + String(grat_x) + "," + String(scalemin_y) + ";");
    Serial.print("PD" + String(grat_x) + "," + String(scalemax_y) + ";");
    //Plot horizontal graticule
    Serial.print("PU" + String(scalemin_x) + "," + String(grat_y) + ";");
    Serial.print("PD" + String(scalemax_x) + "," + String(grat_y) + ";");
    Serial.println();
  }

  //Draw the minor tic-marks on the central axes
  for (int i = 0; i <= 50; i++) {
    grat_x = (0.02 * i * scalemax_x);
    grat_y = (0.02 * i * scalemax_y);
    //Plot vertical axis ticmarks
    Serial.println("PU" + String(grat_x) + "," + String(center_y) + ";" + "PR0,-10;PD;PR0,20;PA;");
    //Plot horizontal axis ticmarks
    Serial.println("PU" + String(center_x) + "," + String(grat_y) + ";" + "PR-10,0;PD;PR20,0;PA;");
  }

  //Initialize on-screen settings, pen color, character size, string terminator and coordinates offset
  Serial.println("SP" + String(blue) + ";" + "SR2,3;" + "DT@;" + "LO15;");

  //Plot/print on-screen settings at scaled coordinates
  grat_x = 0.25 * scalemax_x;
  grat_y = 0.95 * scalemax_y;
  Serial.println("PU" + String(grat_x) + "," + String(grat_y) + ";");
  Serial.println("LB10db/div@;");

  grat_x = 0.75 * scalemax_x;
  grat_y = 0.95 * scalemax_y;
  Serial.println("PU" + String(grat_x) + "," + String(grat_y) + ";");
  Serial.println("LBREF 0dbm@;");

  grat_x = 0.25 * scalemax_x;
  grat_y = 0.05 * scalemax_y;
  Serial.println("PU" + String(grat_x) + "," + String(grat_y) + ";");
  Serial.println("LB5KHz/div@;");

  grat_x = 0.75 * scalemax_x;
  grat_y = 0.05 * scalemax_y;
  Serial.println("PU" + String(grat_x) + "," + String(grat_y) + ";");
  Serial.println("LBRBW 30KHz@;");

   while(analogRead(ADCx) >10){

  runMatrixIndicator();

   };

  //Change pen color. Obtain and output first plot value
  plot.x = 0;
  plot.y = analogRead(ADCy);


#ifdef ARDUINO_UNOR4_WIFI
  //  Load Peakdetector array
  fftData[plot.x].bin = plot.x;
  fftData[plot.x].value = plot.y;
#endif

  //  Output HPGL
  Serial.println("SP" + String(red) + ";");
  Serial.print("PU" + String(plot.x) + "," + String(plot.y) + ";PD;");

  // Obtain  remainder of plot values
  while (plot.x < 1023) {
    if (analogRead(ADCx) > plot.x) {
      plot.x = plot.x + 1;
      analogRead(ADCy);
      plot.y = analogRead(ADCy);

#ifdef ARDUINO_UNOR4_WIFI
      //  Load Peakdetector array
      fftData[plot.x].bin = plot.x;
      fftData[plot.x].value = plot.y;
#endif

                        delay(0);
      //  Output HPGL
      //  Output formatting, 10 plot points per line <cr,lf>
      if (plot.x % 10 == 0) {
        Serial.println("PA" + String(plot.x) + "," + String(plot.y) + ";");
      } else {
        Serial.print("PA" + String(plot.x) + "," + String(plot.y) + ";");
      }
      
    }
  }
  Serial.println();

#ifdef ARDUINO_UNOR4_WIFI
  //Plot markers, initialize pen color, character size, string terminator and coordinates offset
  Serial.println("SP"+String(green)+";"+"SR1,1;"+"DT@;"+"LO4;");


  //CODE FOR PLOTTING MARKERS GOES HERE
  uint8_t numPeaks = findSpectrumPeaks(fftData, detectedPeaks);

  for(int i = 0; i < numPeaks; i++){
    Serial.print("PU"+String(detectedPeaks[i].bin)+","+String(detectedPeaks[i].value)+";");
    Serial.print("PD;PR-5,-15,10,0,-5,15;PA;");
    Serial.print("PU"+String(detectedPeaks[i].bin)+","+String(detectedPeaks[i].value)+";");
    Serial.println("LB"+String(i + 1)+"@;");
  }
  //Establish new plot box for marker data


  Serial.println("IP" + String(mkrboxmin_x) + "," + String(mkrboxmin_y) + "," + String(mkrboxmax_x) + "," + String(mkrboxmax_y) + ";");
  //Serial.println("SC" + String(mkrscalemin_x) + "," + String(mkrscalemax_x) + "," + String(mkrscalemin_y) + "," + String(mkrscalemax_y) + ",1;");
  Serial.println("SC0,1023,0,1023,1,100,100;");
//669
  Serial.println("PU675,0;PD675,1023;PD1023,1023;PD1023,0;PD675,0;PU;");
  //Print a marker value
  Serial.println("SP" + String(green) + ";" + "SR2,3;" + "DT@;" + "LO15;");
  grat_x = 669 + 0.5 * (1023 - 669);
  grat_y = 1023 * 0.95;
  Serial.print("PU"+String(grat_x)+","+String(grat_y)+";");
  Serial.println("LB-22db 30KHz@;");
#endif


  Serial.println("PU;SP;IN;");



}

 //******************************************
void loop() {

  int adc = analogRead(sensePin);
  float voltage = adc * (3.3 / 4095.0);

  if (voltage > threshold) {
    // show steady Y
    matrix.renderBitmap(Y_bitmap, 12, 8);

  } else {
    // handle blinking N
    unsigned long now = millis();
    if (now - lastBlink > 300) {  // toggle every 300 ms
      blinkState = !blinkState;
      lastBlink = now;
    }

    if (blinkState) {
      matrix.renderBitmap(N_bitmap, 12, 8);   // N ON
    } else {
      matrix.renderBitmap(N_blank, 12, 8);    // N OFF (blank)
    }
  }

  delay(20);  // small loop delay
}
