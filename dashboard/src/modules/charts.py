"""
Charts module (Plotly).
Author: Jhon Lee Quint
"""

import plotly.graph_objects as go
import plotly.express as px

# Palette
COLORS = {
    "bar": "#008FA3",  # primary
    "line": "#E74C3C",  # target
    "text": "#2C3E50",
    "bg": "#FFFFFF"
}

def create_combo_chart(df, x_col, y_bar, y_line, title, y_label, lang='PT'):
    """
    Combined chart: actual as bars, target as a line.
    """
    fig = go.Figure()

    # Bars: actual
    fig.add_trace(go.Bar(
        x=df[x_col], 
        y=df[y_bar],
        name="Real",
        marker_color=COLORS["bar"],
        hovertemplate="<b>%{x}</b><br>Real: %{y:.0f}<extra></extra>"
    ))

    # Line: target
    fig.add_trace(go.Scatter(
        x=df[x_col], 
        y=df[y_line],
        name="Meta",
        mode='lines+markers',
        line=dict(color=COLORS["line"], width=3, dash='dot'),
        hovertemplate="Meta: %{y:.0f}<extra></extra>"
    ))

    # Layout
    fig.update_layout(
        title={'text': title, 'y':0.95, 'x':0.5, 'xanchor': 'center', 'yanchor': 'top'},
        yaxis_title=y_label,
        template="plotly_white",
        height=380,
        legend=dict(orientation="h", y=1.1, x=0.5, xanchor="center"),
        margin=dict(l=20, r=20, t=60, b=20),
        hovermode="x unified",  # Shared hover
        font=dict(family="Segoe UI, sans-serif")
    )
    
    return fig

def create_gauge(value, title, max_val=150):
    fig = go.Figure(go.Indicator(
        mode = "gauge+number",
        value = value,
        title = {'text': title, 'font': {'size': 16}},
        gauge = {
            'axis': {'range': [0, max_val], 'tickwidth': 1},
            'bar': {'color': COLORS["bar"]},
            'bgcolor': "white",
            'borderwidth': 2,
            'bordercolor': "gray",
            'steps': [
                {'range': [0, 90], 'color': '#f9f9f9'},
                {'range': [90, 100], 'color': '#eefbfb'}
            ],
            'threshold': {
                'line': {'color': "red", 'width': 4},
                'thickness': 0.75,
                'value': 100
            }
        }
    ))
    fig.update_layout(height=250, margin=dict(l=30, r=30, t=40, b=20))
    return fig
