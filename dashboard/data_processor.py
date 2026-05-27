"""
Data processing module.
Author: Jhon Lee Quint

Rules:
- Only the sectors in ALLOWED_SECTORS are kept.
- Aggregation uses the mean (rates are never summed).
- Missing values stay NaN (never filled with zero).
"""

import pandas as pd
import numpy as np

# Column mapping
COLS = {
    'prod': 'PROD P/HR',
    'meta_prod': 'META PROD P/HR',
    'res_prod_perc': 'Resultado Meta Prod P/HR (%)',
    'upph': 'UPPH',
    'meta_upph': 'Meta UPPH',
    'res_upph_perc': 'Resultado Meta UPPH (%)',
    'fqa': 'FQA',
    'ipqc': 'IPQC',
    'turnover': 'Turnover',
    'abs': 'Absenteísmo'
}

ALLOWED_SECTORS = ['L1A', 'L2B-RF', 'L2B', 'L2C', 'L3D']

def load_data(filepath):
    try:
        df = pd.read_excel(filepath)
    except Exception as e:
        return None, str(e)

    df.columns = df.columns.str.strip()
    
    missing = [c for c in COLS.values() if c not in df.columns]
    
    if 'Data' in df.columns:
        df['Data'] = pd.to_datetime(df['Data'])
    
    # --- SECTOR FILTER ---
    if 'Setor' in df.columns:
        # Exact match on purpose (no normalization).
        df['Setor'].fillna("SEM SETOR", inplace=True)  # avoid NaN comparisons
        initial_len = len(df)
        df = df[df['Setor'].isin(ALLOWED_SECTORS)]
    
    # --- DATE FIELDS ---
    df['Ano'] = df['Data'].dt.year
    df['Mes_Sort'] = df['Data'].dt.strftime('%Y-%m')
    df['Mes'] = df['Data'].dt.strftime('%b/%Y')
    df['Semana'] = df['Data'].dt.strftime('Semana %U')
    df['Trimestre'] = df['Data'].dt.to_period('Q').astype(str)
    df['Dia_Format'] = df['Data'].dt.strftime('%d/%m')

    # Numeric metric columns, keeping NaN
    for col in COLS.values():
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors='coerce')

    # Trim strings to avoid duplicates caused by spaces
    if 'Nome do Produto' in df.columns:
        df['Nome do Produto'] = df['Nome do Produto'].astype(str).str.strip()
    if 'Setor' in df.columns:
        df['Setor'] = df['Setor'].astype(str).str.strip()
            
    return df, None

def filter_data(df, selected_products, selected_sectors):
    filtered = df.copy()
    if selected_products:
        filtered = filtered[filtered['Nome do Produto'].isin(selected_products)]
    if selected_sectors:
         if 'Setor' in filtered.columns:
            filtered = filtered[filtered['Setor'].isin(selected_sectors)]
    return filtered

def aggregate_by_period(df, period_mode):
    """
    Aggregates by mean. NaN in partial columns (FQA, etc.) is skipped by pandas.
    """
    grouper_map = {
        "Diario": 'Dia_Format',
        "Semanal": 'Semana',
        "Mensal": 'Mes_Sort',  # groups by YYYY-MM
        "Trimestral": 'Trimestre'
    }
    grouper = grouper_map.get(period_mode, 'Dia_Format')
    
    # Aggregate only the columns present
    existing_cols = [c for c in COLS.values() if c in df.columns]
    agg_dict = {c: 'mean' for c in existing_cols}
    
    if grouper == 'Dia_Format':
        df_sorted = df.sort_values('Data')
        grouped = df_sorted.groupby(grouper, sort=False).agg(agg_dict).reset_index()
    else:
        grouped = df.groupby(grouper).agg(agg_dict).reset_index()
    
    # Recompute percentages from the aggregated means instead of averaging
    # the spreadsheet's percentage column, so they match the actual-vs-target chart.
    
    if COLS['prod'] in grouped and COLS['meta_prod'] in grouped:
         grouped[COLS['res_prod_perc']] = grouped.apply(
            lambda r: (r[COLS['prod']] / r[COLS['meta_prod']] * 100) if r[COLS['meta_prod']] > 0 else 0, axis=1
        )
         
    if COLS['upph'] in grouped and COLS['meta_upph'] in grouped:
         grouped[COLS['res_upph_perc']] = grouped.apply(
            lambda r: (r[COLS['upph']] / r[COLS['meta_upph']] * 100) if r[COLS['meta_upph']] > 0 else 0, axis=1
        )
    
    return grouped, grouper

def calculate_kpis(df):
    """
    Global means as a dict. A column that is entirely NaN returns None.
    """
    if df.empty:
        return {}
    
    results = {}
    
    # Main metrics: % recomputed from global means
    avg_prod = df[COLS['prod']].mean()
    avg_meta_prod = df[COLS['meta_prod']].mean()
    results['prod_real'] = avg_prod
    results['prod_meta'] = avg_meta_prod
    results['prod_perc'] = (avg_prod / avg_meta_prod * 100) if avg_meta_prod > 0 else 0
    
    avg_upph = df[COLS['upph']].mean()
    avg_meta_upph = df[COLS['meta_upph']].mean()
    results['upph_real'] = avg_upph
    results['upph_meta'] = avg_meta_upph
    results['upph_perc'] = (avg_upph / avg_meta_upph * 100) if avg_meta_upph > 0 else 0
    
    # Optional metrics (may be NaN)
    # Helper
    def get_mean_or_none(col_name):
        if col_name not in df.columns: return None
        if df[col_name].isna().all(): return None
        return df[col_name].mean()
        
    results['fqa'] = get_mean_or_none(COLS['fqa'])
    results['ipqc'] = get_mean_or_none(COLS['ipqc'])
    results['turnover'] = get_mean_or_none(COLS['turnover'])
    results['abs'] = get_mean_or_none(COLS['abs'])
    
    return results
