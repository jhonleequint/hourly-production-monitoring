"""
Charts module (Plotly).
Author: Jhon Lee Quint
"""

import plotly.graph_objects as go
import plotly.express as px

COLORS = {
    "bar": "#008FA3",       
    "line": "#E74C3C",      
    "text": "#2C3E50",
    "bg": "#FFFFFF"
}

def create_combo_chart(df, x_col, y_real, y_meta, title, y_label, period_mode="Semanal"):
    """
    Combined chart: actual as bars, target as a line.
    The Y axis starts at 0 to avoid visual distortion.
    """
    fig = go.Figure()

    # 1. Actual: bars
    fig.add_trace(go.Bar(
        x=df[x_col], 
        y=df[y_real],
        name="Real",
        marker_color=COLORS["bar"],
        marker_pattern_shape="",
        hovertemplate="<b>%{x}</b><br>Real: %{y:.1f}<extra></extra>"
    ))

    # 2. Target: line
    fig.add_trace(go.Scatter(
        x=df[x_col], 
        y=df[y_meta],
        name="Meta",
        mode='lines',
        line=dict(color=COLORS["line"], width=3, dash='dashdot'),
        hovertemplate="Meta: %{y:.1f}<extra></extra>"
    ))

    # Y range: start at 0 with 15% headroom
    max_val = max(df[y_real].max(), df[y_meta].max()) if not df.empty else 10
    y_range = [0, max_val * 1.15]

    # Layout
    fig.update_layout(
        title={'text': title.upper(), 'y':0.95, 'x':0.5, 'xanchor': 'center', 'yanchor': 'top', 'font': {'size': 14, 'color': '#7F8C8D'}},
        yaxis_title=dict(text=y_label, font=dict(size=12)),
        xaxis=dict(showgrid=False, linecolor='#E0E0E0'),
        yaxis=dict(showgrid=True, gridcolor='#F0F0F0', zeroline=False, range=y_range),
        template="plotly_white",
        height=350,
        legend=dict(orientation="h", y=-0.15, x=0.5, xanchor="center"),
        margin=dict(l=40, r=20, t=60, b=50),
        hovermode="x unified",
        font=dict(family="Segoe UI, sans-serif", color=COLORS["text"])
    )
    
    return fig

def create_gauge(value, title, max_val=150):
    fig = go.Figure(go.Indicator(
        mode = "gauge+number",
        value = value,
        title = {'text': title, 'font': {'size': 14}},
        gauge = {
            'axis': {'range': [0, max_val], 'tickwidth': 1},
            'bar': {'color': COLORS["bar"]},
            'bgcolor': "white",
            'threshold': {
                'line': {'color': "red", 'width': 4},
                'thickness': 0.75,
                'value': 100
            }
        }
    ))
    fig.update_layout(height=200, margin=dict(l=30, r=30, t=40, b=20))
    return fig
