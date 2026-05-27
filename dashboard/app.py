"""
Production Dashboard v6.0
Author: Jhon Lee Quint
Status: beta
"""

import streamlit as st
import pandas as pd
import os

current_dir = os.path.dirname(os.path.abspath(__file__))

import data_processor as dp
import charts

# --- I18N (Internationalization) ---
I18N = {
    "PT": {
        # Header / General
        "main_title": "PRODUCTION DASHBOARD",
        "subtitle_dev": "Desenvolvido por",
        "subtitle_status": "Status",
        
        # Filters
        "period": "Período",
        "sector": "Setor",
        "products": "Produtos",
        
        # Period Options
        "daily": "Diário", 
        "weekly": "Semanal", 
        "monthly": "Mensal", 
        "quarterly": "Trimestral",
        
        # Sections
        "prod_title": "PRODUÇÃO", 
        "qual_title": "QUALIDADE", 
        "hr_title": "GESTÃO DE PESSOAS",
        
        # KPIs
        "kpi_prod_real": "Média Prod/Hr",
        "kpi_prod_meta": "Meta Prod/Hr",
        "kpi_prod_perc": "% Atingimento",
        "kpi_upph_real": "Média UPPH",
        "kpi_upph_meta": "Meta UPPH",
        "kpi_upph_perc": "% Atingimento UPPH",
        
        "kpi_fqa": "FQA",
        "kpi_ipqc": "IPQC",
        "kpi_abs": "Absenteísmo",
        "kpi_turn": "Turnover",
        
        # Charts
        "chart_prod": "Produção Média",
        "chart_upph": "UPPH Médio",
        "yaxis_pcs": "Pçs/Hr",
        "yaxis_upph": "UPPH",
        "meta_label": "Meta",

        # Export
        "export_btn": "📥 Exportar Dados (CSV)",
        "no_data": "Nenhum dado encontrado para os filtros selecionados.",
        "check_file": "Por favor, verifique se o arquivo Excel está no local correto."
    },
    "EN": {
        "main_title": "PRODUCTION DASHBOARD",
        "subtitle_dev": "Developed by",
        "subtitle_status": "Status",
        
        "period": "Period",
        "sector": "Sector",
        "products": "Products",
        
        "daily": "Daily", 
        "weekly": "Weekly", 
        "monthly": "Monthly", 
        "quarterly": "Quarterly",
        
        "prod_title": "PRODUCTION", 
        "qual_title": "QUALITY", 
        "hr_title": "PEOPLE MANAGEMENT",
        
        "kpi_prod_real": "Avg Prod/Hr",
        "kpi_prod_meta": "Target Prod/Hr",
        "kpi_prod_perc": "% Hit Rate",
        "kpi_upph_real": "Avg UPPH",
        "kpi_upph_meta": "Target UPPH",
        "kpi_upph_perc": "% Hit Rate UPPH",
        
        "kpi_fqa": "FQA",
        "kpi_ipqc": "IPQC",
        "kpi_abs": "Absenteeism",
        "kpi_turn": "Turnover",
        
        "chart_prod": "Avg Production",
        "chart_upph": "Avg UPPH",
        "yaxis_pcs": "Pcs/Hr",
        "yaxis_upph": "UPPH",
        "meta_label": "Target",

        "export_btn": "📥 Export Data (CSV)",
        "no_data": "No data found for selected filters.",
        "check_file": "Please check if the Excel file is in the correct location."
    },
    "ZH-TW": {
        "main_title": "生產儀表板", # Production Dashboard
        "subtitle_dev": "開發者",
        "subtitle_status": "狀態",
        
        "period": "期間",
        "sector": "部門",
        "products": "產品",
        
        "daily": "每日", 
        "weekly": "每週", 
        "monthly": "每月", 
        "quarterly": "每季",
        
        "prod_title": "生產", 
        "qual_title": "品質", 
        "hr_title": "人事管理",
        
        "kpi_prod_real": "平均產量/時",
        "kpi_prod_meta": "目標產量/時",
        "kpi_prod_perc": "達成率",
        "kpi_upph_real": "平均 UPPH",
        "kpi_upph_meta": "目標 UPPH",
        "kpi_upph_perc": "UPPH 達成率",
        
        "kpi_fqa": "FQA",
        "kpi_ipqc": "IPQC",
        "kpi_abs": "缺勤率", # Absenteeism
        "kpi_turn": "離職率", # Turnover
        
        "chart_prod": "平均生產",
        "chart_upph": "平均 UPPH",
        "yaxis_pcs": "件/時",
        "yaxis_upph": "UPPH",
        "meta_label": "目標",

        "export_btn": "📥 匯出資料 (CSV)",
        "no_data": "未找到所選篩選器的資料。",
        "check_file": "請檢查 Excel 檔案是否位於正確位置。"
    }
}

st.set_page_config(page_title="Production Dashboard | Jhon Lee Quint", page_icon="🏭", layout="wide", initial_sidebar_state="collapsed")

# Inject CSS
def load_css():
    css_path = os.path.join(current_dir, "assets", "styles.css")
    if os.path.exists(css_path):
        with open(css_path) as f:
            st.markdown(f'<style>{f.read()}</style>', unsafe_allow_html=True)
load_css()

# --- LOAD ---
@st.cache_data
def get_data():
    data_path = os.path.join(current_dir, "data", "production_data.xlsx")
    if os.path.exists(data_path):
        return dp.load_data(data_path)

    return None, "File Not Found"

df, error = get_data()

# Error header
if error:
    st.error(f"⚠️ {error}")
    st.info("Caminho esperado: data/production_data.xlsx (relativo à raiz do projeto).")
    st.stop()


# --- HEADER & LANG ---
c_brand, c_lang = st.columns([8, 1])
with c_lang:
    # PT-BR is the default (first option)
    lang_opts = ["PT", "EN", "ZH-TW"]
    lang = st.selectbox("Idioma/Language", lang_opts, label_visibility="collapsed")

T = I18N[lang]

st.markdown(f"""
<div class="header-box">
    <div>
        <div class="title-main">🏭 {T['main_title']}</div>
        <div class="subtitle-main">{T['subtitle_dev']}: <b>Jhon Lee Quint</b> | {T['subtitle_status']}: <span style="color:#E67E22; font-weight:bold;">BETA</span></div>
    </div>
</div>
""", unsafe_allow_html=True)

# --- FILTERS ---
f1, f2, f3 = st.columns([1, 1, 2])
with f1:
    # Map Display Name -> Internal Mode
    p_map = {
        T['daily']: "Diario", 
        T['weekly']: "Semanal", 
        T['monthly']: "Mensal", 
        T['quarterly']: "Trimestral"
    }
    p_sel = st.selectbox(f"📅 {T['period']}", list(p_map.keys()))
    period_mode = p_map[p_sel]

with f2:
    sectors = sorted(df['Setor'].unique()) if 'Setor' in df.columns else []
    sel_sec = st.multiselect(f"🏭 {T['sector']}", sectors)

with f3:
    prods = sorted(df['Nome do Produto'].unique()) if 'Nome do Produto' in df.columns else []
    sel_prod = st.multiselect(f"📦 {T['products']}", prods)

# Filter
clean_df = dp.filter_data(df, sel_prod, sel_sec)

if clean_df.empty:
    st.info(T['no_data'])
    st.stop()

# Aggregation
grouped_df, x_axis = dp.aggregate_by_period(clean_df, period_mode)
kpis = dp.calculate_kpis(clean_df)

# --- KPI HELPER ---
def kpi_card(col, label, val, meta=None, is_perc=False, highlight=False, is_na=False):
    css = "kpi-card highlight-upph-card" if highlight else "kpi-card"
    
    if is_na or val is None:
        val_display = '<span style="color:#BDC3C7; font-size:24px;">N/A</span>'
        meta_display = ""
    else:
        val_num = f"{val:.1f}%" if is_perc else f"{val:,.1f}".replace(',', '.')
        val_display = val_num
        
        if meta is not None:
             meta_display = f"{T['meta_label']}: {meta:,.1f}"
        else:
             meta_display = ""

    col.markdown(f"""
    <div class="{css}">
        <div class="kpi-label">{label}</div>
        <div class="kpi-value">{val_display}</div>
        <div class="kpi-sub">{meta_display}</div>
    </div>
    """, unsafe_allow_html=True)

# --- BLOCKS ---
st.markdown(f"### {T['prod_title']}")
k1, k2, k3, k4, k5, k6 = st.columns(6)

kpi_card(k1, T['kpi_prod_real'], kpis['prod_real'])
kpi_card(k2, T['kpi_prod_meta'], kpis['prod_meta'])
kpi_card(k3, T['kpi_prod_perc'], kpis['prod_perc'], is_perc=True)

kpi_card(k4, T['kpi_upph_real'], kpis['upph_real'])
kpi_card(k5, T['kpi_upph_meta'], kpis['upph_meta'])
kpi_card(k6, T['kpi_upph_perc'], kpis['upph_perc'], is_perc=True, highlight=True)

# Graphs
st.markdown("---")
g1, g2 = st.columns(2)
with g1:
    fig_p = charts.create_combo_chart(
        grouped_df, x_axis, dp.COLS['prod'], dp.COLS['meta_prod'], 
        f"{T['chart_prod']} ({p_sel})", T['yaxis_pcs'], period_mode
    )
    st.plotly_chart(fig_p, width="stretch")

with g2:
    fig_u = charts.create_combo_chart(
        grouped_df, x_axis, dp.COLS['upph'], dp.COLS['meta_upph'], 
        f"{T['chart_upph']} ({p_sel})", T['yaxis_upph'], period_mode
    )
    st.plotly_chart(fig_u, width="stretch")

# --- QUALITY & HR ---
st.markdown("---")
c_qual, c_hr = st.columns(2)

with c_qual:
    st.markdown(f"#### 🛡️ {T['qual_title']}")
    q1, q2 = st.columns(2)
    kpi_card(q1, T['kpi_fqa'], kpis['fqa'], is_perc=True, is_na=(kpis['fqa'] is None))
    kpi_card(q2, T['kpi_ipqc'], kpis['ipqc'], is_perc=True, is_na=(kpis['ipqc'] is None))

with c_hr:
    st.markdown(f"#### 👥 {T['hr_title']}")
    h1, h2 = st.columns(2)
    kpi_card(h1, T['kpi_abs'], kpis['abs'], is_perc=True, is_na=(kpis['abs'] is None))
    kpi_card(h2, T['kpi_turn'], kpis['turnover'], is_perc=True, is_na=(kpis['turnover'] is None))

# Export
st.markdown("---")
csv_data = clean_df.to_csv(index=False).encode('utf-8')
st.download_button(T['export_btn'], csv_data, "production_data.csv", "text/csv")

# Footer
st.markdown(f'<div class="footer-fixed">{T["subtitle_dev"]}: Jhon Lee Quint &nbsp;&nbsp;|&nbsp;&nbsp; {T["subtitle_status"]}: BETA</div>', unsafe_allow_html=True)
