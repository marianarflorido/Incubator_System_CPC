from django.db import models
from django.utils import timezone

# Modelo para armazenar o histórico de leituras e atuações
class HistoricoIncubadora(models.Model):
    # Identificador único da incubadora (ex: 'INC_001')
    id_incubadora = models.CharField(max_length=50, db_index=True)

    # Dados de Telemetria
    temperatura_atual = models.FloatField()
    sinal_controle = models.FloatField()  # Potência de atuação (-10.0 a 10.0)
    setpoint = models.FloatField()        # Setpoint usado no momento do registro

    # Dados de Estado
    estado = models.CharField(max_length=50) # Ex: 'AQUECER', 'ESTAVEL', 'FALLBACK', 'ALERTA'
    modo = models.CharField(max_length=50)   # Ex: 'AUTOMATICO', 'MANUAL', 'FALLBACK'

    timestamp = models.DateTimeField(default=timezone.now) # Usaremos 'timestamp' para data e hora

    def __str__(self):
        return f"[{self.timestamp.strftime('%Y-%m-%d %H:%M:%S')}] {self.id_incubadora}: {self.temperatura_atual}°C"

    class Meta:
        verbose_name_plural = "Histórico da Incubadora"
        ordering = ['-timestamp']


# Modelo para armazenar o Setpoint configurado remotamente
class ConfiguracaoIncubadora(models.Model):
    # Identificador único da incubadora (Chave primária)
    id_incubadora = models.CharField(max_length=50, primary_key=True)

    # Valor do setpoint
    setpoint = models.FloatField(default=37.0)

    # Campo para rastrear o último acesso
    ultimo_acesso = models.DateTimeField(auto_now=True) # Usaremos 'ultimo_acesso'

    def __str__(self):
        return f"Configuração de {self.id_incubadora}: Setpoint = {self.setpoint}°C"

    class Meta:
        verbose_name_plural = "Configurações da Incubadora"