from rest_framework import generics
from rest_framework.response import Response
from rest_framework import status
from .models import HistoricoIncubadora, ConfiguracaoIncubadora
from .serializers import HistoricoIncubadoraSerializer, ConfiguracaoIncubadoraSerializer

# -----------------------------------------------------------
# POST: Rota /historico/ -> Salva o histórico (Dados do Wokwi)
# -----------------------------------------------------------
class HistoricoCreateView(generics.CreateAPIView):
    """
    Recebe POST do ESP32 (Wokwi) com dados de telemetria e estado
    e salva no banco de dados.
    """
    queryset = HistoricoIncubadora.objects.all()
    serializer_class = HistoricoIncubadoraSerializer

    def post(self, request, *args, **kwargs):
        serializer = self.get_serializer(data=request.data)

        if serializer.is_valid():
            self.perform_create(serializer)
            return Response(
                {"status": "sucesso", "mensagem": "Histórico salvo com sucesso."},
                status=status.HTTP_201_CREATED
            )

        return Response(serializer.errors, status=status.HTTP_400_BAD_REQUEST)

# -----------------------------------------------------------
# GET: Rota /historico/<id_incubadora>/ -> Lista o histórico para o Dashboard
# -----------------------------------------------------------
class HistoricoListView(generics.ListAPIView):
    """
    Retorna o histórico de telemetria de uma incubadora específica,
    usada para exibir o gráfico no dashboard.
    """
    serializer_class = HistoricoIncubadoraSerializer

    def get_queryset(self):
        # Captura o ID da incubadora da URL
        id_incubadora = self.kwargs['id_incubadora']

        # Filtra o histórico por ID e ordena por timestamp (do mais antigo para o mais recente)
        return HistoricoIncubadora.objects.filter(id_incubadora=id_incubadora).order_by('timestamp')


# -----------------------------------------------------------
# GET: Rota /setpoint/<id_incubadora>/ -> Retorna o setpoint (Dados para o Wokwi)
# -----------------------------------------------------------
class SetpointDetailView(generics.RetrieveAPIView):
    """
    Retorna o setpoint configurado para uma incubadora específica.
    """
    serializer_class = ConfiguracaoIncubadoraSerializer

    def get_object(self):
        id_incubadora = self.kwargs.get('id_incubadora')

        try:
            # Garante que o objeto existe
            return ConfiguracaoIncubadora.objects.get(id_incubadora=id_incubadora)
        except ConfiguracaoIncubadora.DoesNotExist:
            # Se não existe, cria um novo com valor padrão
            return ConfiguracaoIncubadora.objects.create(
                id_incubadora=id_incubadora,
                setpoint=37.0
            )

    def retrieve(self, request, *args, **kwargs):
        instance = self.get_object()
        serializer = self.get_serializer(instance)

        # O ESP32 só precisa do JSON simples com o setpoint
        return Response(serializer.data)