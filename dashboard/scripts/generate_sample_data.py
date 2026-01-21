"""
Sample data generator.
Author: Jhon Lee Quint

Writes production_data.xlsx with synthetic records. Product names are fictional.
"""

import pandas as pd
import numpy as np
from datetime import datetime, timedelta
import random
import os

def generate_data():
    print("Generating sample data...")

    start_date = datetime(2025, 10, 15)
    days = 120

    products = [
        "CV-AX53(BR) 1.6",
        "CV-EX520(BR) 1.6",
        "CV-Mesh M5 (3-Pack)"
    ]

    data = []

    current_date = start_date
    for _ in range(days):
        if current_date.weekday() <= 5: # Mon-Sat
            entries = random.randint(1, 4)
            for _ in range(entries):
                meta_p_hr = random.choice([100, 120, 150])
                meta_upph = random.choice([12, 15, 18])

                eff = random.uniform(0.80, 1.15) # 80% to 115%

                prod_hr = int(meta_p_hr * eff)
                upph = round(meta_upph * eff, 2)

                data.append({
                    "Nome do produto": random.choice(products),
                    "Data": current_date,
                    "PROD HR": prod_hr,
                    "UPPH": upph,
                    "META P/HR": meta_p_hr,
                    "META UPPH": meta_upph,
                })

        current_date += timedelta(days=1)

    df = pd.DataFrame(data)

    output_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    output_path = os.path.join(output_dir, "production_data.xlsx")

    df.to_excel(output_path, index=False)
    print(f"Sample data written to {output_path}")

if __name__ == "__main__":
    generate_data()
