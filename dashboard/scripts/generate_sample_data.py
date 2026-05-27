"""
Sample data generator.
Author: Jhon Lee Quint

Writes data/production_data.xlsx with synthetic records (sector, product,
production, UPPH, quality and HR columns).

All product names and sector codes are fictional and exist only to demo
the dashboard.
"""

import os
import random
from datetime import datetime, timedelta

import numpy as np
import pandas as pd

RNG_SEED = 42

# Fictional product catalog
PRODUCTS = [
    "CV-AX53(BR) 1.6",
    "CV-EX520(BR) 1.6",
    "CV-EX520(BR) 1.9",
    "CV-EX521(BR) 1.6",
    "CV-EX521(BR) 1.9",
    "CV-MR30G(BR) 1.26",
    "CV-MR60X(BR) 3.6.0",
    "CV-EX511(BR) 2.6",
    "CV-Mesh M5 (3-Pack)",
]

# Fictional sectors (production lines)
SECTORS = ["L1A", "L2B", "L2B-RF", "L2C", "L3D"]
ACTIVE_SECTORS = ["L1A", "L2B"]  # sectors with volume in the sample period


def generate_full_dataset(start_date, end_date):
    """Full schema: sector, quality and HR (FQA/IPQC/HR columns left empty)."""
    random.seed(RNG_SEED + 1)
    data = []
    current_date = start_date
    while current_date <= end_date:
        for sector in ACTIVE_SECTORS:
            meta_prod = random.choice([185, 190, 205, 275, 450])
            meta_upph = round(random.uniform(15.4, 42.1), 2)
            prod = round(meta_prod * random.uniform(0.6, 1.3), 1)
            upph = round(meta_upph * random.uniform(0.6, 1.3), 2)

            data.append({
                "Setor": sector,
                "Nome do Produto": random.choice(PRODUCTS),
                "Data": current_date,
                "PROD P/HR": prod,
                "META PROD P/HR": meta_prod,
                "Resultado Meta Prod P/HR (%)": (prod / meta_prod * 100) if meta_prod else 0,
                "UPPH": upph,
                "Meta UPPH": meta_upph,
                "Resultado Meta UPPH (%)": (upph / meta_upph * 100) if meta_upph else 0,
                "FQA": np.nan,
                "IPQC": np.nan,
                "Turnover": np.nan,
                "Absenteísmo": np.nan,
            })
        current_date += timedelta(days=1)
    return pd.DataFrame(data)


def main():
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    df_full = generate_full_dataset(datetime(2025, 10, 1), datetime(2025, 12, 17))
    full_path = os.path.join(repo_root, "data", "production_data.xlsx")
    os.makedirs(os.path.dirname(full_path), exist_ok=True)
    df_full.to_excel(full_path, index=False)
    print(f"Sample data written to {full_path} ({len(df_full)} rows)")


if __name__ == "__main__":
    main()
