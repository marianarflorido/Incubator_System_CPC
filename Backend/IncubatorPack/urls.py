from django.urls import path
from . import views

urlpatterns = [
    # Rota para POST de Histórico (Envio de Telemetria do Wokwi): /historico/
    path('historico/', views.HistoricoCreateView.as_view(), name='historico_post'),

    # Rota para GET de Histórico (Dashboard Web - Filtrado por ID): /historico/<id_incubadora>/
    path('historico/<str:id_incubadora>/', views.HistoricoListView.as_view(), name='historico_list'),

    # Rota para GET de Setpoint: /setpoint/<id_incubadora>/
    path('setpoint/<str:id_incubadora>/', views.SetpointDetailView.as_view(), name='setpoint_get'),
]
