from django.contrib import admin
from django.urls import path, include

urlpatterns = [
    # Rota padrão de administração
    path('admin/', admin.site.urls),
    # Todas as rotas do incubatorPack.urls começarão com /api/v1/
    path('api/v1/', include('incubatorPack.urls')),
]