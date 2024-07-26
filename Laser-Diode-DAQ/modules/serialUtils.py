import serial
import sys
import threading
import os
import time
import copy
from datetime import datetime
from modules import globals
import csv
from pathlib import Path

class ReadLine:
    def __init__(self, s):
        self.buf = bytearray()
        self.s = s

    def readline(self):
        i = self.buf.find(b"\n")
        if i >= 0:
            r = self.buf[:i + 1]
            self.buf = self.buf[i + 1:]
            return r
        while True:
            i = max(1, min(2048, self.s.in_waiting))
            data = self.s.read(i)
            i = data.find(b"\n")
            if i >= 0:
                r = self.buf + data[:i + 1]
                self.buf[0:] = data[i + 1:]
                return r
            else:
                self.buf.extend(data)

class SerialThread(threading.Thread):
    def __init__(self, port, dataHolder, baud):
        super(SerialThread, self).__init__()
        self._stop = threading.Event()
        self.port = port

        # Receiving data variables
        self.serialData = ""
        self.dataHolder = dataHolder
        self.data = []
        self.baud = baud

        # Serial printing data variables
        self.RTS = False # Ready To Send
        self.message = ""

        # Recording data variables
        self.recording = False
        self.recordingNum = 0
        self.basefilename = "recording"
        self.recordingfilename = ""
        self.saveDir = "data"
        self.startRecordTime = 0
        self.stopRecordTime = 0

        os.makedirs(self.saveDir, exist_ok=True)

    def stop(self):
        self._stop.set()

    def stopped(self):
        return self._stop.isSet()


    def start_recording(self, filename=None):
        # If input filename, take it. Otherwise make a default one from recordingNum
        if filename is None:
            self.recordingfilename = os.path.join(self.saveDir,
                                         self.basefilename + str(self.recordingNum) + ".txt")
            print("filename not supplied, recording into", self.recordingfilename)
        else:
            self.recordingfilename = os.path.join(self.saveDir, filename)
            print("recording into", self.recordingfilename)

            ## Change the labels as needed
            ## I don't really know how it works super well but when you first run the program with no data points, it may prevent the GUI graph to work
            ## Once you quit it and rerun it, it works 
            existance = Path('data//data.csv')
            if not existance.is_file():
                with open(self.recordingfilename, 'w', newline='') as csvfile:
                    filewriter = csv.writer(csvfile, delimiter=',',
                                            quotechar='|', quoting=csv.QUOTE_MINIMAL)
                    filewriter.writerow(['Data/Time', ' Laser1', ' Laser10', ' Laser3', ' Laser4', ' Laser5', ' Laser6', ' Laser7', ' Battery', ' Vin (5)', ' TIA ILX', ' float', ' Temp(F)', ' Vin (28)'])

        # Set variables and tell Teensy to start recording
        self.recording = True
        self.send_message("c")
        self.startRecordTime = time.time()


    def stop_recording(self):
        # Turn off teensy recording
        self.send_message("q")

        # Reset variables and iterate the default file num
        self.data = []
        self.recording = False
        self.recordingNum += 1

    def send_message(self, message):
        self.RTS = True
        self.message = message
        t1 = threading.Thread(target=self.threadSendMess)
        t1.start()

    def threadSendMess(self):
        self.com.write(self.message.encode('utf-8'))
        self.stopRecordTime = time.time()
        # print("Sent in thread", self.message)
        self.RTS = False
        self.message = ""
        
        
    def avg(self, line):
        values = map(int, line.split(','))
        
        
    def run(self):
        try:
            self.com = serial.Serial(self.port, self.baud, timeout = 0.5, rtscts=True)
            self.rl = ReadLine(self.com)
        except Exception as e:
            print("\nSERIAL PORT ERROR:", e)
            sys.exit()

        while True:
            if self.stopped():
                return

            # While there are bytes to be read, keep reading and recording
            # Do not record if there's a pending message to be sent
            if self.recording and not self.RTS:
                while self.com.in_waiting:
                    self.data.append(self.com.read(1000))

                # Write into file
                tmp = copy.copy(self.data)
                tmp = b''.join(tmp)
                tmp = tmp.strip().split(b'\r\n')
                tmp = [x.decode("utf-8").strip() for x in tmp]
                
                with open("./data/data.csv", "a") as f:
                    write_buff = []
                    for line in tmp:
                        values = line.split()
                        if len(values) == 1 and values[0][0].isdigit():
                            datapoint = list(map(float,values[0].split(',')))
                            
                            for i, point in enumerate(datapoint):
                                if len(write_buff) > i:
                                    curr = (write_buff[i][0] * write_buff[i][1]) + point
                                    write_buff[i] = (curr/(write_buff[i][1]+1), write_buff[i][1] + 1)
                                else:
                                    write_buff.append((point, 1))

                                
                                
                    if write_buff:
                        output = [str(x[0]) for x in write_buff]     
                        f.write(datetime.now().strftime("%D:%H:%M:%S") + ',' + ','.join(output) + "\n")
                        write_buff = []

                self.data = []

            time.sleep(globals.sampling_rate)
