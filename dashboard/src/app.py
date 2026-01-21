"""
Production Dashboard v3.0
Author: Jhon Lee Quint
Status: beta
"""

import streamlit as st
import pandas as pd
import os
import sys

current_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(current_dir)
sys.path.append(parent_dir)

from src.modules import data_processor as dp
from src.modules import charts

# --- I18N DICTIONARY ---
I18N = {
    "PT": {
        "prod_label": "Produção Real",
        "meta_prod_label": "Meta Produção",
        "perc_prod_label": "% Atingimento Prod.",
        "upph_label": "UPPH Real",
        "meta_upph_label": "Meta UPPH",
        "perc_upph_label": "% Atingimento UPPH",
        "all_products": "Todos os Produtos",
        "period": "Período",
        "products": "Produtos",
        "week": "Semana", "month": "Mês", "quarter": "Trimestre"
    },
    "EN": {
        "prod_label": "Actual Production",
        "meta_prod_label": "Production Target",
        "perc_prod_label": "Prod. Hit Rate %",
        "upph_label": "Actual UPPH",
        "meta_upph_label": "UPPH Target",
        "perc_upph_label": "UPPH Hit Rate %",
        "all_products": "All Products",
        "period": "Period",
        "products": "Products",
        "week": "Week", "month": "Month", "quarter": "Quarter"
    },
    "ZH-TW": {
        "prod_label": "實際生產",
        "meta_prod_label": "生產目標",
        "perc_prod_label": "生產達成率 %",
        "upph_label": "實際 UPPH",
        "meta_upph_label": "UPPH 目標",
        "perc_upph_label": "UPPH 達成率 %",
        "all_products": "所有產品",
        "period": "時期",
        "products": "產品",
        "week": "週", "month": "月", "quarter": "季度"
    }
}

st.set_page_config(page_title="Production Dashboard | Jhon Lee Quint", page_icon="🏭", layout="wide", initial_sidebar_state="collapsed")

def load_css():
    with open(os.path.join(current_dir, "assets", "styles.css")) as f:
        st.markdown(f'<style>{f.read()}</style>', unsafe_allow_html=True)
load_css()

# --- LOAD DATA ---
@st.cache_data
def get_data():
    return dp.load_data(os.path.join(parent_dir, "production_data.xlsx"))

df, error = get_data()
if error:
    st.error(error)
    st.stop()

# --- HEADER AREA ---
c_brand, c_lang = st.columns([8, 1])
with c_lang:
    lang = st.selectbox("Language", ["PT", "EN", "ZH-TW"], label_visibility="collapsed")
T = I18N[lang]

# Top banner
st.markdown(f"""
<div class="header-box">
    <div>
        <div class="title-main">🏭 PRODUCTION DASHBOARD</div>
        <div class="subtitle-main">Developed by: <b>Jhon Lee Quint</b> | Status: <span style="color:#E67E22; font-weight:bold;">BETA</span></div>
    </div>
    <div>
    </div>
</div>
""", unsafe_allow_html=True)

# --- FILTERS ---
f1, f2 = st.columns([1, 2])
with f1:
    idx_map = {T['week']: "Semanal", T['month']: "Mensal", T['quarter']: "Trimestral"}
    p_sel = st.selectbox(f"📅 {T['period']}", list(idx_map.keys()))
    period_mode = idx_map[p_sel]

with f2:
    prod_list = df['Nome do produto'].unique()
    sel_prods = st.multiselect(f"📦 {T['products']}", prod_list, placeholder=T['all_products'])

selected = sel_prods if sel_prods else list(prod_list)
clean_df = dp.filter_data(df, selected)
grouped_df, x_axis = dp.aggregate_by_period(clean_df, period_mode)

# --- KPIs ---
kpis = dp.calculate_kpis(clean_df)

st.markdown("<!-- KPI GRID -->", unsafe_allow_html=True)
k1, k2, k3, k4, k5, k6 = st.columns(6)

def kpi_card(col, label, value, sub="", highlight=False):
    css_class = "kpi-card highlight-upph-card" if highlight else "kpi-card"
    val_class = "highlight-upph-value" if highlight else "kpi-value"
    col.markdown(f"""
    <div class="{css_class}">
        <div class="kpi-label">{label}</div>
        <div class="{val_class}">{value}</div>
        <div class="kpi-sub">{sub}</div>
    </div>
    """, unsafe_allow_html=True)

# Row 1: production
kpi_card(k1, T['prod_label'], f"{int(kpis['prod_real']):,}".replace(',', '.'))
kpi_card(k2, T['meta_prod_label'], f"{int(kpis['prod_meta']):,}".replace(',', '.'))
kpi_card(k3, T['perc_prod_label'], f"{kpis['prod_perc']:.1f}%", highlight=False)

# Row 2: UPPH (K6 highlighted)
kpi_card(k4, T['upph_label'], f"{kpis['upph_real']:.2f}")
kpi_card(k5, T['meta_upph_label'], f"{kpis['upph_meta']:.2f}")
# UPPH % is the headline metric
kpi_card(k6, T['perc_upph_label'], f"{kpis['upph_perc']:.1f}%", highlight=True)

# --- CONTEXT ---
prod_name_display = sel_prods[0] if len(sel_prods) == 1 else T['all_products']
st.markdown(f"<div style='margin-top:20px; color:#7F8C8D; font-size:14px;'>Visualizando: <b>{prod_name_display}</b></div>", unsafe_allow_html=True)

# --- CHARTS ---
g1, g2 = st.columns(2)

with g1:
    st.markdown('<div class="chart-container">', unsafe_allow_html=True)
    fig_p = charts.create_combo_chart(grouped_df, x_axis, 'PROD HR', 'META P/HR', f"Produção ({T['period']})", "Pcs")
    st.plotly_chart(fig_p, use_container_width=True)
    st.markdown('</div>', unsafe_allow_html=True)

with g2:
    st.markdown('<div class="chart-container">', unsafe_allow_html=True)
    fig_u = charts.create_combo_chart(grouped_df, x_axis, 'UPPH', 'META UPPH', f"UPPH ({T['period']})", "UPPH")
    st.plotly_chart(fig_u, use_container_width=True)
    st.markdown('</div>', unsafe_allow_html=True)

# Footer
st.markdown(f'<div class="footer-fixed">Developed by: Jhon Lee Quint &nbsp;&nbsp;|&nbsp;&nbsp; Status: BETA</div>', unsafe_allow_html=True)
