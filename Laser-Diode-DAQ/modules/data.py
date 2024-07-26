# data.py
import pandas as pd
from modules.globals import filename

def load_data():
    data = pd.read_csv('data/data.csv')
    return data
