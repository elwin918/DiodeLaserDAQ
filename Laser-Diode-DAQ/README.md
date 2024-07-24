# Laser-Diode-DAQ

<!-- INSTRUCTIONS TO COLLECT DATA 

Ensure the Teensy is plugged in

build and upload main.cpp under the src folder
    if you get this error in yellow: 
        src\main.cpp: In function 'void ADC_ISR()':
        src\main.cpp:127:19: warning: variable 'lastmicros' set but not used [-Wunused-but-set-variable]
        127 |   static uint32_t lastmicros;
    just reupload main.cpp


click the plus sign to add a new powershell terminal

delete the original terminal and any extra ones 

enter the following command
    python main.py
This will take you to the GUI

on the GUI, select your sampling rate and power factors and click save then start
    enter 1 as the com port in the terminal if asked

the Teensy should now add the data to the file "data.csv" 

if you want to make a new csv file, just change the name of data.csv
    this will put the data that is currently in the data.csv file into a new csv file with the name you chose
    the data.csv file will reset the next time you run main.py  -->


