import sys
from modules import data, plot, gui, globals, functions 


def main():
    globals.window, globals.frame = gui.create_window()
    
    # Create an input box for sample rate 
    sampling_period = gui.IntInput()
    gui.TextBox(globals.frame, sampling_period, text= "Sample Period (s)", row=0, col=0)
    


    # Create an input box for test criteria
    channels = []
    for i in range(1, 11):
        channels.append(gui.IntInput())
        channel_var = gui.IntInput()
        channel_var.set(1)
        globals.channel_vars.append(channel_var)
        gui.TextBox(globals.frame, channels[i-1], text=f"Laser {i}", row=i, col=0)
        gui.CheckButton(globals.frame, text=f'Show Laser {i}', variable=channel_var, row=i-1, col=3, command = functions.laser_check)

    temp_var = gui.IntInput()
    temp_var.set(0)

    curr_var = gui.IntInput()
    curr_var.set(0)

    globals.therm_curr.append(temp_var)
    globals.therm_curr.append(curr_var)

    gui.CheckButton(globals.frame, text = f'Show Temperature', variable=temp_var, row = 0, col = 5, command = functions.therm_check)
    gui.CheckButton(globals.frame, text = f'Show Current', variable=curr_var, row = 1, col = 5, command=functions.curr_check)


    gui.Button(globals.frame, "Save", 0, 2, command=lambda: functions.save(sampling_period, channels))

    # Create buttons for start, stop, pause
    gui.Button(globals.frame, "Start", 1, 2, command=functions.start)
    gui.Button(globals.frame, "Stop", 2, 2, command=functions.stop)
    gui.Button(globals.frame, "Export", 3, 2, command=functions.export)
    gui.Button(globals.frame, "Quit", 4, 2, command=functions.quit)
    
    # Run GUI
    globals.window.mainloop() 
    
if __name__ == "__main__":
    main()