# gui.py
import tkinter as tk
from tkinter import ttk

def TextBox(frame, var, text, row, col):
    ttk.Label(frame, text=text).grid(row=row, column=col)
    ttk.Entry(frame, textvariable=var).grid(row=row, column=col+1)
    

def IntInput():
    return tk.IntVar()

def Button(frame, text, row, col, command):
    ttk.Button(frame, text=text, command=command).grid(row=row, column=col)

def CheckButton(frame, text, variable, row, col, command):
    return ttk.Checkbutton(frame, text=text, variable=variable, command=command).grid(row=row, column=col)

    

def create_window():
    window = tk.Tk() 
    window.title('Plotting in Tkinter') 
    
    frame = tk.Frame(window)
    frame.pack()
    
    return window, frame