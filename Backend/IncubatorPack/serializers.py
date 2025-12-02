from rest_framework import serializers
from .models import HistoricoIncubadora, ConfiguracaoIncubadora

# Serializador para receber e criar um novo registro de histórico (POST)
class HistoricoIncubadoraSerializer(serializers.ModelSerializer):
    class Meta:
        model = HistoricoIncubadora
        fields = '__all__'
        read_only_fields = ['timestamp']

# Serializador para o setpoint (GET)
class ConfiguracaoIncubadoraSerializer(serializers.ModelSerializer):
    class Meta:
        model = ConfiguracaoIncubadora
        fields = ['setpoint'] # O ESP32 só precisa do campo 'setpoint' na resposta