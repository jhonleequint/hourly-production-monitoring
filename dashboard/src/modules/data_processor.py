"""
Data processing module.
Author: Jhon Lee Quint
"""

import pandas as pd
import numpy as np

def load_data(filepath):
    try:
        df = pd.read_excel(filepath)
    except Exception as e:
        return None, str(e)

    df.columns = df.columns.str.strip()
    
    if 'Data' in df.columns:
        df['Data'] = pd.to_datetime(df['Data'])
    
    # --- DATA AUGMENTATION ---
    df['Ano'] = df['Data'].dt.year
    df['Mes_Sort'] = df['Data'].dt.strftime('%Y-%m')
    df['Mes'] = df['Data'].dt.strftime('%b/%Y')
    df['Semana'] = df['Data'].dt.strftime('Semana %U')
    df['Trimestre'] = df['Data'].dt.to_period('Q').astype(str)

    # Coerce metric columns to numbers
    cols_num = ['PROD HR', 'META P/HR', 'UPPH', 'META UPPH']
    for c in cols_num:
        if c in df.columns:
            df[c] = pd.to_numeric(df[c], errors='coerce').fillna(0)
            
    return df, None

def filter_data(df, selected_products):
    if not selected_products:
        return df
    return df[df['Nome do produto'].isin(selected_products)]

def aggregate_by_period(df, period_mode):
    """
    Sum/sum aggregation so percentages are correctly weighted.
    """
    grouper_map = {
        "Semanal": 'Semana',
        "Mensal": 'Mes_Sort',
        "Trimestral": 'Trimestre'
    }
    grouper = grouper_map.get(period_mode, 'Semana')
    
    # Base aggregation
    agg_cols = {
        'PROD HR': 'sum',
        'META P/HR': 'sum',
        # UPPH is a rate (units/hour/person); without hours worked, summing
        # rates is meaningless, so the period mean is used as the average speed.
        'UPPH': 'mean', 
        'META UPPH': 'mean'
    }
    
    grouped = df.groupby(grouper).agg(agg_cols).reset_index()
    
    # Percentages computed after aggregation
    # 1. Production % = sum(actual) / sum(target)
    grouped['PROD HR %'] = grouped.apply(
        lambda row: (row['PROD HR'] / row['META P/HR'] * 100) if row['META P/HR'] > 0 else 0,
        axis=1
    )
    
    # 2. UPPH % = mean(actual) / mean(target)
    grouped['UPPH %'] = grouped.apply(
        lambda row: (row['UPPH'] / row['META UPPH'] * 100) if row['META UPPH'] > 0 else 0,
        axis=1
    )
    
    return grouped, grouper

def calculate_kpis(df):
    """
    Weighted KPIs for the selected period, returned as a dict.
    """
    total_prod_real = df['PROD HR'].sum()
    total_prod_meta = df['META P/HR'].sum()
    prod_perc = (total_prod_real / total_prod_meta * 100) if total_prod_meta > 0 else 0
    
    avg_upph_real = df['UPPH'].mean()
    avg_upph_meta = df['META UPPH'].mean()
    upph_perc = (avg_upph_real / avg_upph_meta * 100) if avg_upph_meta > 0 else 0
    
    return {
        "prod_real": total_prod_real,
        "prod_meta": total_prod_meta,
        "prod_perc": prod_perc,
        "upph_real": avg_upph_real,
        "upph_meta": avg_upph_meta,
        "upph_perc": upph_perc
    }
