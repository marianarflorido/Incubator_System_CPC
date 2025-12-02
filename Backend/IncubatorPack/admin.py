from django.contrib import admin
from .models import HistoricoIncubadora, ConfiguracaoIncubadora 

@admin.register(ConfiguracaoIncubadora)
class ConfiguracaoIncubadoraAdmin(admin.ModelAdmin):
    list_display = ('id_incubadora', 'setpoint', 'ultimo_acesso')
    search_fields = ('id_incubadora',)
    list_editable = ('setpoint',) 

# Personalização da exibição do modelo HistoricoIncubadora
@admin.register(HistoricoIncubadora)
class HistoricoAdmin(admin.ModelAdmin):
    # Campos que serão exibidos na lista
    list_display = ('id_incubadora', 'temperatura_atual', 'sinal_controle', 'estado', 'timestamp')
    
    # BLOQUEIO DE EDIÇÃO
    readonly_fields = [
        'id_incubadora', 
        'temperatura_atual', 
        'sinal_controle', 
        'setpoint', 
        'estado', 
        'modo', 
        'timestamp'
    ]
    
    list_filter = ('id_incubadora', 'estado') 
    date_hierarchy = 'timestamp'