# Projeto Incubadora
Este projeto, desenvolvido para a disciplina de Controle de Processos por Computador da UERJ, estabelece um Sistema de Controle de Temperatura para uma Incubadora.
O projeto adota uma arquitetura distribuída que integra um microcontrolador ESP32 (executando o controle PID localmente) com um Backend Django (hospedado no PythonAnywhere) para supervisão e monitoramento remoto.

## 🛠️ Arquitetura e Fluxo de Dados

O fluxo é dividido em dois ambientes:

1.  **Backend (Django):** Hospeda a API REST, armazena o histórico de telemetria e permite a configuração remota do *setpoint*.

2.  **ESP32 - Wokwi:** Executa o **Controlador PID local** em tempo real, monitora sensores e botões, e lida com a lógica de estados (Modo Automático, Manual e Alerta).

| **Comunicação** | **Endpoint** | **Uso** | 
| :--- | :--- | :--- |
| **GET** | `/api/v1/setpoint/<id_incubadora>/` | ESP32 busca o setpoint configurado remotamente. | 
| **POST** | `/api/v1/historico/` | ESP32 envia dados de telemetria e estado. | 
| **GET** | `/api/v1/historico/<id_incubadora>/` | Busca o histórico de uma unidade específica. | 

## 💻 Configuração do Backend (Django no PythonAnywhere)

### 1. Preparação do Ambiente

1.  Acesse [PythonAnywhere](https://www.pythonanywhere.com/) e configure seu domínio.

2.  Crie ou utilize a pasta raiz do seu projeto (ex: `~/incubator`)

3.  Na pasta raiz use a confihuração de `settings.py`, `urls.py`.

4.  Garanta que os arquivos da aplicação (`models.py`, `views.py`, etc.) estejam na subpasta **`incubatorPack`**.

### 2. Configuração de Banco de Dados e Rotas

No terminal do PythonAnywhere:

1.  **Navegue** até a pasta do seu projeto principal: `cd ~/incubator`

2.  **Execute as Migrações** para criar as tabelas (`ConfiguracaoIncubadora` e `HistoricoIncubadora`):

    ```bash
    python manage.py makemigrations incubatorPack
    python manage.py migrate
    ```

3.  **Configuração de Rotas (URLs)**: Certifique-se de que o arquivo principal `incubator/urls.py` inclua o prefixo `api/v1` com a função `include` importada.

### 3. Integridade e Imutabilidade (Dashboard)

Para impedir a edição dos dados de telemetria no painel de administração do Django:

* O arquivo `incubatorPack/admin.py` define o modelo `HistoricoIncubadora` com a propriedade `readonly_fields`, bloqueando qualquer alteração manual dos registros de temperatura, potência e estado após serem salvos.

## ⚙️ Uso do Código ESP32 (Wokwi)

O código ESP32 (`incubadora_pid.ino`) implementa um fluxo procedural com alta disponibilidade:

### 1. Componentes Essenciais

O circuito simulado no Wokwi deve incluir:

| **Componente** | **Função** | 
| :--- | :--- |
| **ESP32** | Microcontrolador principal | 
| **Sensor (DS18B20)** | Leitura de temperatura inicial (depois, simulação) | 
| **Buzzer** | Alerta de Tampa Aberta | 
| **LIGA/DESLIGA** | Switch de Parada Imediata | 
| **Tampa** | Switch de Segurança (Alerta) | 
| **Modo** | Switch que alterna entre modo Manual/Automático | 

### 2. Configuração no Código

No `incubadora_pid.ino`, configure o `BASE_URL` para o seu domínio ativo e o nome (ID) da incubadora.

### 3. Projeto do circuito

[Incubadora 1](https://wokwi.com/projects/448975964990308353)

[Incubadora 2](https://wokwi.com/projects/449191392240589825)

### 4. PythonAnywhere 

[Dashboard](https://marianarobaina.pythonanywhere.com/)











