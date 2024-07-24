# plot.py
import matplotlib.pyplot as plt
import pandas as pd
from modules import data as dt, globals
from matplotlib.figure import Figure 
from matplotlib.animation import FuncAnimation
from matplotlib.backends.backend_tkagg import (FigureCanvasTkAgg,  NavigationToolbar2Tk) 

import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.dates as mdates

import csv
import os 


def animate(i, fig):
    
    ## Skips the first row for the labels
    data = pd.read_csv('data/data.csv', skiprows=[0])



    data[data.columns[0]] = pd.to_datetime(data[data.columns[0]], format='%m/%d/%y:%H:%M:%S')  # Convert to datetime object
    x = data[data.columns[0]]

    fig.clear()
    ax = fig.subplots()
    plotted = False
    colors = ['r', 'g', 'blue', 'gray', 'orange', 'purple', 'black', 'olivedrab', 'pink', 'sienna']
    for i in range(1, 11):
        if globals.channel_vars[i-1].get():
            ax.plot(x, data[data.columns[i]], label=f'Col {i}', color=colors[i-1])       ## CHANGE COL TO LASER WHEN READY TO GO; GOT IT LIKE THAT NOW WITH THINGS MOVING AROUND
            ax.set_ylabel('Laser Power (Watts)')
            plotted = True

    if globals.therm_curr[1].get():
        ax.plot(x, data[data.columns[11]], label=f'Set Mon', color='navy')
        ax.set_ylabel('Set Monitor (Volts)')
        plotted = True

    if globals.therm_curr[0].get():
        ax.plot(x, data[data.columns[12]], label=f'Temperature', color='teal')
        ax.set_ylabel('Temperature (Fahrenheit)')
        plotted = True



    # Format the x-axis to properly display dates
    ax.xaxis.set_major_formatter(mdates.DateFormatter('%d/%H:%M:%S'))
    ax.xaxis.set_major_locator(mdates.DayLocator())
    for label in ax.get_xticklabels():
        label.set_rotation(45)
        label.set_ha('right')
        label.set_fontsize(6)


    if plotted:
        ax.legend(loc='upper center', bbox_to_anchor=(0.5, 1.18),
        ncol=4, fancybox=True, shadow=True)

        plt.tight_layout()


    
def plot(window): 
    # the figure that will contain the plot 
    fig = Figure(figsize = (5, 5), dpi = 100) 

    # creating the Tkinter canvas containing the Matplotlib figure 
    canvas = FigureCanvasTkAgg(fig, master = window)   
    canvas.draw() 

    # placing the canvas on the Tkinter window 
    canvas.get_tk_widget().pack() 

    # creating the Matplotlib toolbar 
    toolbar = NavigationToolbar2Tk(canvas, window) 
    toolbar.update() 

    # placing the toolbar on the Tkinter window 
    canvas.get_tk_widget().pack() 

    # Assign the animation to the global variable anim
    globals.anim = FuncAnimation(fig, animate, fargs=(fig,), interval=1000, save_count=1000)
    plt.tight_layout()