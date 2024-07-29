#include <Arduino.h>
#include <ADC.h>
#include <SD.h>
#include <TimeLib.h>

const int ledpin  = 13;
#define  LEDON   digitalWriteFast(ledpin, HIGH);
#define  LEDOFF  digitalWriteFast(ledpin,  LOW);

// specify the number of channels and which pins to use
#define NUMCHANNELS 13
// refer to the Teensy 4.1 key to find which pin corresponds to which
const uint16_t ADCPins[NUMCHANNELS] = {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12 };

// Specify how fast to collect samples
#define SAMPRATE  10


// We will save a 32-bit Unix seconds value, 16-bit spare and NUMCHANNELS 16-bit ADC counts for each record
#define RECORDSIZE (4 +2 + NUMCHANNELS * 2)// size in bytes of each record 


// define a new data type for the samples
// with default packing, the structure will be a 
// multiple of 4 byte--so I added spare to make  it come
// out to 16 bytes.
typedef struct tSample {
  uint32_t useconds;   
  uint16_t avals[NUMCHANNELS];
} sampletype;  // each record is 14 bytes long for now

// now define two buffers, each holding 1024 samples.
// For efficiency, the number of samples in each buffer
// is a multiple of 512, which means that complete sectors
// are writen to the output file each time a buffer is written.
#define SAMPLESPERBUFFER 15
tSample buffer1[SAMPLESPERBUFFER];
tSample buffer2[SAMPLESPERBUFFER];


// Define pointers to buffers used for ADC Collection
// and SD card file writing.
tSample *adcptr = NULL;
tSample *sdptr = NULL;

static uint32_t samplecount = 0;
volatile uint32_t bufferindex = 0;
volatile uint32_t overflows = 0;

const char compileTime [] = "\n\n11-channel ADC Test Compiled on " __DATE__ " " __TIME__;

static ADC *adc = new ADC(); // adc object
IntervalTimer CollectionTimer;


#define ADCVREF  3.3166  // what I measured for V3.3 with my voltmeter
// Show voltages in first record of buffer1.  You will see a valid
// result only when  collection has been run.



void ShowADCVolts(void) {
  float volts;
  int i;
  Serial.println("Approximate ADC input voltages");
  for(i= 0; i<NUMCHANNELS; i++){
    volts = buffer1[0].avals[i] * ADCVREF/4096.0;
    Serial.printf(" %8.4f",volts); 
  }
  Serial.println();
}

File adcFile;

// Calculates the temperature in Farenheit from pin 25 (A11)
float temp(float therm) {

  float R1 = 10000;
  float logR2, R2, T;
  float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;

  R2 = R1 * (4096.0 / therm - 1.0);
  logR2 = log(R2);

  T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
  T = T - 273.15;
  T = (T * 9.0)/ 5.0 + 32.0;

  return T;
}


// code for the SD card as a backup
void CMSI(void) {
  Serial.println();
  Serial.println(compileTime);
  if (adcFile) {
    Serial.printf("adcFile is open and contains %lu samples.\n", samplecount);
  } else {
    Serial.println("adcFile is closed.");
  }
  Serial.println("Valid commands are:");
  Serial.println("   s : Show this message");
  Serial.println("   d : Show SD Card Directory");
  Serial.println("   v : Show approximate volts");
  Serial.println("   c : Start data collection");
  Serial.println("   q : Stop data collection");
  Serial.println();
}

// Add data buffer to output file when needed
void CheckFileState(void) {

  // ADC ISR sets sdptr to a buffer point in ISR
  if (sdptr != NULL) { // write buffer to file
    if (adcFile) { // returns true when file is open
      LEDON
      adcFile.write(sdptr, SAMPLESPERBUFFER * sizeof(tSample));
      adcFile.flush();  // update directory and reduce card power
      LEDOFF
    } // end of if(adcfile)
    sdptr = NULL;  // reset pointer after file is written
  }  // end of if(sdptr != NULL)

}


// Voltage reading found on Robojax.com ARDVC-01 Measure any voltage with Arduino
// Follow this Youtube video if you want to change the power supply voltage and the resistor values:
// https://www.youtube.com/watch?v=t8xwrVj2aFs
// Includes functions readVoltage and getVoltageAverage

// variables for the voltage calculations
// Exact voltage values help with more accurate calculations
const float arduinoVCC = ADCVREF;//Teensy voltage
unsigned long ValueR1 = 328;  // This resistor is connected to ground and the voltage across it is read by the Teensy 
unsigned long ValueR2 = 2537;  // This resistor value is connected to the positive rail and consists of a 2k, 330, and 220 OHM.
double Voltage_Source = 28.2;
const int analogPin = A12; //the pin connecting the voltage - labeled pin 26. 
const int inputResolution =1023; // A12 reads out of 1024 
const float average_of = 100;//Averages this number of readings 
float voltage;

void readVoltage(){
  
  int A0Value = analogRead(analogPin);
  float voltage_sensed = A0Value * (arduinoVCC / (float)inputResolution); 
  voltage = voltage_sensed * ( 1 + ( (float) ValueR2 /  (float) ValueR1) );
}

float getVoltageAverage(){
  float voltage_temp_average=0;
  for(int i=0; i < average_of; i++)
  {
    readVoltage();
    voltage_temp_average +=voltage;
  }
    
  return voltage_temp_average / average_of;
}


// const int tot_power_factors = 10;  // Total number of power factors used
// float power_factors[tot_power_factors];      // Array to store power factors from the GUI
// int currentIndex = 0;

// This is the interrupt handler called by the collection interval timer
void ADC_ISR(void) {
  tSample *sptr;
  static uint32_t lastmicros;
  if (adcptr == NULL) return; // don't write unless adcptr is valid
  if (bufferindex >= SAMPLESPERBUFFER) {  // Switch buffers and signal write to SD
    if(sdptr != NULL) overflows++; // foreground didn't write buffer in time
    sdptr = adcptr; // notify foreground to write buffer to SD
    bufferindex = 0;
    if (adcptr == buffer1) {
      adcptr = buffer2;   // collect to buffer2 while buffer1 is written to SD
    } else { // use buffer 1 for collection
      adcptr = buffer1;
    }
  }
  // now we know that bufferindex is less than SAMPLESPERBUFFER
  // Please forgive the mix of pointers and array indices in the next line.
  sptr = (tSample *)&adcptr[bufferindex];
  // pure pointer arithmetic MIGHT be faster--depending on how well the compiler optimizes.
  sptr->useconds = now();
  lastmicros =  micros();  // we can use this later  to check for sampling jitter
  
  // if (Serial.available() > 0) {
  //   int value = Serial.parseInt();  // Read an integer from serial
  //   if (currentIndex < tot_power_factors) {
  //       power_factors[currentIndex++] = value;  // Store the value in the array
  //   }
  // }
  
  for(int i=0; i<NUMCHANNELS; i++) {
    sptr->avals[i] = (uint16_t)analogRead(ADCPins[i]);
    


    // Print Values to Serial Monitor:
    if (i != NUMCHANNELS - 1) 
    {
      if (i == NUMCHANNELS - 2) {
        Serial.print(temp((float) sptr->avals[i]), 4);    // Temperature
      }
      else if(i == NUMCHANNELS - 5)
      {
        Serial.print((sptr->avals[i]) * ADCVREF * 2/4096.0 , 4);     // for monitoring the 5 volt supply that is set up with a voltage divider 
      }
      else {

// TRYING TO GET THE POWER FACTORS INCORPORATED
    //  if globals.power_factors and not data.iloc[1].isnull().any():
    //      for i, pf in enumerate(globals.power_factors):
            
    //          if i == enumerate(globals.power_factors):
    //              done = True
    //          else:
    //              scalar = (pf/data[data.columns[i+1]].tail(1).values[0])
    //              data.iloc[:, i+1] = data.iloc[:, i+1].astype('float64') * (0.0 if float(scalar) == 0 else float(scalar))

        // float printed_power = sptr->avals[i] * ADCVREF/4096.0;

        // float pf = power_factors[i];

        //     if (i == tot_power_factors - 1) {
        //         bool done = true;
        //     } else {
        //         float scalar = pf / data[rows - 1][i + 1]; // Assuming tail(1) is the last row

        //         for (int j = 0; j < rows; j++) {
        //             data[j][i + 1] = data[j][i + 1] * (scalar == 0.0 ? 0.0 : scalar);
        //         }
        //     }

        Serial.print((sptr->avals[i]) * ADCVREF/4096.0, 4); // voltages for the lasers and for set mon and i mon (right now)

      }
      Serial.print(",");  // print a comma after each value except the last one
    } 
    else
    {
      Serial.println(getVoltageAverage());      // voltage from the 28 volt supply
    }
  }
  samplecount++;
  bufferindex++;
}

time_t getTeensy3Time() {
  return Teensy3Clock.get();
}

// User provided date time callback function.
// See SdFile::dateTimeCallback() for usage.
void dateTime(uint16_t* date, uint16_t* time) {
  // use the year(), month() day() etc. functions from timelib
  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(year(), month(), day());
  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(hour(), minute(), second());
}

bool StartSDCard() {
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("\nSD File initialization failed.\n");
    return false;
  } else  Serial.println("initialization done.");
  // set date time callback function for file dates
  SdFile::dateTimeCallback(dateTime);
  return true;
}


// make a new file name based on time and date
char* NewFileName(void) {
  static char fname[36];
  time_t nn;
  nn = now();
  int mo = month(nn);
  int dd = day(nn);
  int hh = hour(nn);
  int mn = minute(nn);
  sprintf(fname, "ADC5CH_%02d%02d%02d%02d.dat", mo, dd, hh, mn);
  return &fname[0];
}

void setup() {
  uint16_t i;
  delay(500);
  Serial.begin(115200);
  delay(1000);
  for (i = 0; i < NUMCHANNELS; i++) {
    pinMode(ADCPins[i], INPUT_DISABLE);
  }

  Serial.printf("Size of tSample is %lu\n", sizeof(tSample));
  pinMode(ledpin, OUTPUT);
  CMSI();
  Serial.print("Initializing SD card...");

  if (!StartSDCard()) {
    Serial.println("initialization failed!");
    while (1) { // Fast blink LED if SD card not ready
      LEDON; delay(100);
      LEDOFF; delay(100);
    }
  }
  // use the default reference, the 3.3V analog voltage
  adc->adc0->setAveraging(32);
  adc->adc0->setResolution(12);
  adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::HIGH_SPEED);   //was MED_SPEED
  adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::HIGH_SPEED); // change the sampling speed

  // Start the timer that controls ADC and DAC
  CollectionTimer.begin(ADC_ISR, 1000000 / SAMPRATE);

  Serial.println("Waiting for commands.");
  setSyncProvider(getTeensy3Time); // helps put time into file directory data
}


void loop() {
  char ch;
  if (Serial.available()) {
    ch = Serial.read();
    Serial.println(ch);
    switch (ch) {
      case 'd' :
        Serial.println("SD Card Directory");
        SD.sdfs.ls(LS_DATE | LS_SIZE);
        Serial.println();
        break;
      case 's' :
        CMSI();
        break;
      case 'v' :
        ShowADCVolts();
        break;
      case 'q' :
        bufferindex = 0;
        if (adcFile) {
          adcFile.close();
          Serial.println("File Collection halted.");
          Serial.printf("Buffer overflows = %lu\n", overflows);
        }
        sdptr = NULL;
        adcptr = NULL;
        bufferindex = 0;
        break;
      case 'c' :
        if (adcFile) adcFile.close();
        adcFile = SD.open(NewFileName(), FILE_WRITE);
        samplecount = 0;
        Serial.printf("Opened file %s\n", NewFileName());
        bufferindex = 0;
        overflows = 0;
        
        adcptr = &buffer1[0];
        sdptr = NULL;
        break;
    } // end of switch(ch)

  }  // end of if(Serial.available())
  CheckFileState();
}