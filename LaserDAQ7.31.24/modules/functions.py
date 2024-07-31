# functions.py
import sys
import time
from modules import globals, plot, interface
import math

import serial

port = None
inter = None

# This describes useful button functions
def quit():
    global inter

    print("Shutting Down Teensy...")
    if inter:
        inter.stop_recording()
        inter.serialThread.stop()
        time.sleep(1)

    print("Exitting...") 
    sys.exit(0)

def save(val, powers):
    globals.sampling_rate = val.get()
    print(f"Saved {globals.sampling_rate}")
    
    if not globals.power_factors:
        for p in powers:
            globals.power_factors.append(p.get())

    else:
        for i, p in enumerate(powers):
            globals.power_factors[i] = p.get()

    # time.sleep(1)

## Attempt at getting the power factors into the c code
    ser = serial.Serial('COM4', 9600)  # Update 'COM4' with the correct port

    time.sleep(2)  # Wait for the serial connection to initialize


    for item in globals.power_factors:
        ser.write(f"{item}\n".encode())  # Send each item
        time.sleep(0.1)  # Small delay to ensure data is sent
    ser.close()

def stop_animation():
    globals.anim.event_source.stop()

def start():
    global port, inter
    print("Connecting to Teensy...")
    port = interface.get_port()
    inter = interface.CLI(port, globals.filename)
    time.sleep(2)
    
    print("Started Data Collection...")
    inter.start_recording()
    time.sleep(globals.sampling_rate)

    print("Launching Plot...")
    plot.plot(globals.window)

def stop():
    global inter
    print("Stopping Data Collection...")
    inter.stop_recording()
    inter.serialThread.stop()
    stop_animation()

    
def export():
    print("Exported")


## I don't think this is really used because the temperature is taken in the c script.
## Keeping it here just in case
def gettemp(therm):
    R1 = 10000
    c1 = 1.009249522e-03
    c2 = 2.378405444e-04
    c3 = 2.019202697e-07

    R2 = R1 * (4096.0 / therm - 1.0)
    logR2 = math.log(R2)

    T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2))
    T = T - 273.15
    T = (T * 9.0)/ 5.0 + 32.0

def laser_check():

    for button in globals.channel_vars:
        if button.get():
            for b in globals.therm_curr:
                b.set(0)
    
def therm_check():
    for button in globals.therm_curr:
        if button.get():
            for b in globals.channel_vars:
                b.set(0) 

    globals.therm_curr[1].set(0)

def curr_check():
    for button in globals.therm_curr:
        if button.get():
            for b in globals.channel_vars:
                b.set(0) 

    globals.therm_curr[0].set(0)