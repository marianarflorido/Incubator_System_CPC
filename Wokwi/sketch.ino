#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// --- CONFIGURAÇÃO GERAL E ROTAS ---
const char* ssid = "Wokwi-GUEST";
const char* pass = "";
// URL base da sua aplicação Django (CORRIGIDO para o seu domínio)
const char* BASE_URL = "https://marianarobaina.pythonanywhere.com";

const String API_PREFIX = "/api/v1";

const char* ID_INCUBADORA = "INC_001"; 

// Endpoints (Usando o prefixo /api/v1/ e o ID para setpoint)
String SETPOINT_BASE_URL = String(BASE_URL) + API_PREFIX + "/setpoint/";
String HISTORICO_ENDPOINT = String(BASE_URL) + API_PREFIX + "/historico/";

// Tolerância para o estado ESTÁVEL (de -1.0 a +1.0 graus)
const float PID_TOLERANCE = 1.0; 

// Intervalos
const unsigned long POLLING_INTERVAL = 15000; // 15s para comunicação com o backend
const unsigned long CONTROL_LOOP_TIME = 500;  // 500ms (0.5s) para o ciclo de controle PID
const unsigned long BUZZER_PULSE_TIME = 250; // Duração do pulso do buzzer (250ms ON/OFF)
unsigned long lastPollingTime = 0;
unsigned long lastControlTime = 0;
unsigned long lastBuzzerToggle = 0; // Para o Buzzer

// Setpoints
float setpoint = 37.0; 
float last_valid_setpoint = 37.0; 

// Variáveis de Timeout e Estado
const int MAX_CONNECT_ATTEMPTS = 5; 
int connectAttempts = 0;
const unsigned long WIFI_RECONNECT_INTERVAL = 3000;
unsigned long lastConnectAttempt = 0;

bool systemIsRunning = true; 
String currentMode = "VERIFICACAO"; 

// Sensores (Simulação)
// DS18B20
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

float current_temperature;
bool buzzerState = false; // Estado do Buzzer

// --- PINAGEM ---
const int LED_VERDE = 27;  
const int LED_AZUL = 26;   
const int LED_VERMELHO = 25; 
const int BUZZER_PIN = 16; 

const int SWITCH_TAMPA = 34;      
const int SWITCH_MODE = 35;       
const int BUTTON_STOP = 32;       
const int BUTTON_MANUAL_HEAT = 19; 
const int BUTTON_MANUAL_COOL = 18; 

float control_output_power = 0.0; 

// --- CLASSE PID ---
class PIDController {
private:
  float Kp, Ki, Kd;
  float integral = 0;
  float previous_error = 0;
  float time_step = (float)CONTROL_LOOP_TIME / 1000.0; 

public:
  PIDController(float p, float i, float d) : Kp(p), Ki(i), Kd(d) {}

  float compute(float setpoint, float current_temp) {
    float error = setpoint - current_temp;

    float proportional = Kp * error;

    integral += error * time_step;
    if (integral > 100) integral = 100;
    if (integral < -100) integral = -100;
    float integral_term = Ki * integral;

    float derivative = (error - previous_error) / time_step;
    float derivative_term = Kd * derivative;

    float output = proportional + integral_term + derivative_term;
    previous_error = error;

    return output;
  }

  void resetIntegral() {
    integral = 0;
    previous_error = 0;
  }
};
PIDController pid(0.5, 0.1, 0.05); 

// --- MODELO DE SIMULAÇÃO DE TEMPERATURA (ESTÁVEL) ---
const float AMBIENT_TEMP = 25.0; 
const float R_THERMAL = 5.0;   
const float C_THERMAL = 5.0;   
const float MAX_POWER = 10.0;  

void updateTemperatureModel(float control_power) {
  control_power = constrain(control_power, -MAX_POWER, MAX_POWER);
  float power_loss = (current_temperature - AMBIENT_TEMP) / R_THERMAL;
  float net_power = control_power - power_loss;
  float delta_T = (net_power / C_THERMAL) * ((float)CONTROL_LOOP_TIME / 1000.0);
  current_temperature += delta_T;
}

// --- FUNÇÕES DE ATUAÇÃO ---
void setLeds(bool red, bool blue, bool green) {
  digitalWrite(LED_VERMELHO, red);
  digitalWrite(LED_AZUL, blue);
  digitalWrite(LED_VERDE, green);
}

// Funções do Buzzer 
void handleBuzzerPulse() {
    if (currentMode == "ALERTA") {
        if (millis() - lastBuzzerToggle >= BUZZER_PULSE_TIME) {
            lastBuzzerToggle = millis();
            buzzerState = !buzzerState;
            digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
        }
    }
}
void startBuzzer() {
    buzzerState = true;
    lastBuzzerToggle = millis();
    digitalWrite(BUZZER_PIN, HIGH);
}
void stopBuzzer() {
    buzzerState = false;
    digitalWrite(BUZZER_PIN, LOW);
}


// --- FUNÇÃO DE COMUNICAÇÃO HTTP: GET Setpoint ---
bool fetchSetpoint() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  HTTPClient http;
  String fullUrl = SETPOINT_BASE_URL + ID_INCUBADORA + "/"; 
  http.begin(fullUrl); 
  
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
    String payload = http.getString();

    DynamicJsonDocument doc(512); 
    DeserializationError error = deserializeJson(doc, payload);

    if (!error && doc.containsKey("setpoint")) {
        setpoint = doc["setpoint"].as<float>();
        last_valid_setpoint = setpoint;
        Serial.print("Setpoint remoto recebido: ");
        Serial.println(setpoint, 2);
        http.end();
        return true; 
    }
    Serial.println("Erro ao decodificar JSON ou 'setpoint' ausente.");
    http.end();
    return false;
  } else {
    Serial.print("Erro HTTP (GET Setpoint): " + String(httpCode));
    Serial.println(". Rota: " + fullUrl);
    http.end();
    return false;
  }
}

// --- FUNÇÃO DE COMUNICAÇÃO HTTP: POST Histórico ---
void sendHistory(String mode) {
  if (WiFi.status() != WL_CONNECTED) {
    return; 
  }
  
  DynamicJsonDocument doc(512);
  doc["id_incubadora"] = ID_INCUBADORA; 
  doc["temperatura_atual"] = current_temperature;
  doc["sinal_controle"] = control_output_power;
  doc["estado"] = mode;
  
  doc["setpoint"] = setpoint;
  doc["modo"] = currentMode;
  
  String jsonBody;
  serializeJson(doc, jsonBody);

  HTTPClient http;
  http.begin(HISTORICO_ENDPOINT); 
  http.addHeader("Content-Type", "application/json");

  // AUMENTADO: Define um timeout de 10 segundos para requisição
  http.setTimeout(5000); 

  int httpCode = http.POST(jsonBody);
  
  if (httpCode > 0) {
    Serial.print("Histórico POST Enviado (Status: "); 
    Serial.print(httpCode);
    Serial.println(")");
    // Leitura da resposta para liberar o buffer HTTP (essencial para evitar travamento)
    http.getString(); 
  } else {
    Serial.print("Erro ao enviar Histórico POST: ");
    Serial.println(httpCode);
  }

  http.end();
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);

  sensors.begin();
  sensors.setResolution(12);
  sensors.setWaitForConversion(true);

  // Configuração dos Pinos
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(LED_AZUL, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Entradas em modo PULLUP
  pinMode(SWITCH_TAMPA, INPUT_PULLUP);
  pinMode(SWITCH_MODE, INPUT_PULLUP);
  pinMode(BUTTON_STOP, INPUT_PULLUP); 
  pinMode(BUTTON_MANUAL_HEAT, INPUT_PULLUP);
  pinMode(BUTTON_MANUAL_COOL, INPUT_PULLUP);

  // Início da simulação de temperatura
  sensors.requestTemperatures();
  float t = sensors.getTempCByIndex(0);

  if (t == DEVICE_DISCONNECTED_C || t == -127.0) {
    Serial.println("ERRO: DS18B20 Desconectado!");
    current_temperature = 25.0; // fallback
  } else {
    current_temperature = t;
  }

  Serial.print("Temperatura inicial (Sensor): ");
  Serial.print(current_temperature, 2);
  Serial.println(" °C");
  Serial.print("ID da Incubadora: ");
  Serial.println(ID_INCUBADORA);

  // Conexão WiFi 
  WiFi.begin(ssid, pass);
  Serial.println("Conectando-se ao WiFi...");
}

// --- LOOP PRINCIPAL ---
void loop() {
  
  // Lógica de Pulso do Buzzer (Roda em TODOS os ciclos)
  handleBuzzerPulse();

  // 1. Lógica do Switch LIGA/DESLIGA (Prioridade 0: Parada Imediata)
  if (digitalRead(BUTTON_STOP) == LOW) { 
    if (systemIsRunning) {
      Serial.println("DESLIGAR: Sistema Parado.");
      sendHistory("STOP");
      systemIsRunning = false;
      control_output_power = 0.0;
      setLeds(false, false, false);
      stopBuzzer();
    }
    return;
  } else { 
    if (!systemIsRunning) {
      Serial.println("LIGA: Reiniciando o ciclo.");
      systemIsRunning = true;
      pid.resetIntegral();
      currentMode = "VERIFICACAO";
      lastPollingTime = 0; 
    }
  }

  // Se o sistema estiver desligado, retorna
  if (!systemIsRunning) {
      return;
  }

  // 3. Verificação de Segurança (Prioridade 1: ALERTA - Tampa Aberta)
  if (digitalRead(SWITCH_TAMPA) == LOW) { 
    if (currentMode != "ALERTA") {
      Serial.println("ALERTA: Tampa Aberta! Buzzer ON.");
      currentMode = "ALERTA";
      control_output_power = 0.0; 
      setLeds(false, false, false);
      startBuzzer(); // Inicia o pulso do buzzer
    }
    
    return;
  } else { 
    if (currentMode == "ALERTA") {
       Serial.println("ALERTA: Resolvido. Retornando ao Hub.");
       stopBuzzer(); // Para o pulso do buzzer
       currentMode = "VERIFICACAO"; 
       lastPollingTime = 0;
    }
  }

  // Modelo de Temperatura 
  if (millis() - lastControlTime >= CONTROL_LOOP_TIME) {
    lastControlTime = millis();
    updateTemperatureModel(control_output_power);
    
    Serial.print("T: ");
    Serial.print(current_temperature, 2);
    Serial.print("°C | Set: ");
    Serial.print(setpoint, 2);
    Serial.print("°C | Pot: ");
    Serial.println(control_output_power, 2);
  }
  
  // 4. Lógica de Conexão e Polling (Hub Central)

  // A. Lógica de Reconexão (Roda em 3s, sempre que o Wi-Fi cair)
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastConnectAttempt >= WIFI_RECONNECT_INTERVAL) {
      lastConnectAttempt = millis();
      connectAttempts++;
      Serial.print("Tentativa de reconexão Wi-Fi #");
      Serial.println(connectAttempts);
      WiFi.reconnect();
    }
    
    // Se a conexão cair, MUDAMOS PARA FALLBACK IMEDIATAMENTE 
    if (currentMode != "FALLBACK" && currentMode != "MANUAL") { 
        Serial.println("WiFi perdido. Entrando em FALLBACK (Controle Local Imediato).");
        currentMode = "FALLBACK";
        pid.resetIntegral();
        lastPollingTime = millis(); 
    }
  } 

  // B. Lógica de Polling (Roda a cada 15s)
  if (millis() - lastPollingTime >= POLLING_INTERVAL) {
    lastPollingTime = millis();

    // Se estiver conectado, tenta comunicar
    if (WiFi.status() == WL_CONNECTED) {
        connectAttempts = 0; 
        
        // Tenta obter o setpoint (Sempre necessário para sair do FALLBACK ou iniciar o ciclo)
        if (fetchSetpoint()) {
          // Sucesso na comunicação -> vai para a Seleção de Modo (sai do FALLBACK)
          currentMode = "VERIFICACAO_BOTOES";
        } else {
          // Falha na API. Se estiver em AUTOMATICO/VERIFICACAO, vai para FALLBACK.
          if (currentMode == "AUTOMATICO" || currentMode == "VERIFICACAO") {
             Serial.println("Backend falhou na API. MODO AUTOMATICO -> FALLBACK.");
             currentMode = "FALLBACK";
          }
          // Se estiver em MANUAL, permanece em MANUAL
        }
    } else {
        // Se o polling disparar e NÃO houver Wi-Fi (FALLBACK_NO_WIFI)
        sendHistory("FALLBACK_NO_WIFI");
    }
  }
  
  // 5. Lógica de Modos (Executa o controle: FALLBACK, MANUAL, AUTOMATICO)
  
  if (currentMode == "FALLBACK") {
      // do/Rodar PID localmente.
      control_output_power = pid.compute(last_valid_setpoint, current_temperature);
      
      // Atuação LED (Usa a tolerância PID_TOLERANCE)
      if (abs(last_valid_setpoint - current_temperature) <= PID_TOLERANCE) { 
        setLeds(false, false, true); // ESTÁVEL
      } else if (last_valid_setpoint > current_temperature) { 
        setLeds(true, false, false); // AQUECER
      } else { 
        setLeds(false, true, false); // RESFRIAR
      }
      
  } else if (currentMode == "VERIFICACAO_BOTOES") {
      // 5a. Seleção de Modo
      if (digitalRead(SWITCH_MODE) == LOW) { // LOW = MANUAL
        Serial.println("Modo selecionado: MANUAL.");
        currentMode = "MANUAL"; 
        pid.resetIntegral();
      } else { // HIGH = AUTOMATICO
        Serial.println("Modo selecionado: AUTOMATICO.");
        currentMode = "AUTOMATICO";
        pid.resetIntegral();
      }

  } else if (currentMode == "MANUAL") {
      // 5b. CONTROLE MANUAL (Sem PID)
      bool heating = digitalRead(BUTTON_MANUAL_HEAT) == LOW;
      bool cooling = digitalRead(BUTTON_MANUAL_COOL) == LOW;

      if (heating) { 
        setLeds(true, false, false); 
        control_output_power = MAX_POWER; 
      } else if (cooling) { 
        setLeds(false, true, false); 
        control_output_power = -MAX_POWER; 
      } else {
        // Potência zero quando o botão é solto
        setLeds(false, false, false); 
        control_output_power = 0.0;
      }
      
      // Fim do ciclo e Retorno ao Hub
      if (millis() - lastPollingTime >= POLLING_INTERVAL) { 
        sendHistory("MANUAL_IDLE");
        currentMode = "VERIFICACAO";
      }

  } else if (currentMode == "AUTOMATICO") {
      // 5c. CONTROLE AUTOMÁTICO (PID)
      
      control_output_power = pid.compute(setpoint, current_temperature);
      float error_abs = abs(setpoint - current_temperature);

      String state_name;
      // Lógica ESTÁVEL usando PID_TOLERANCE
      if (error_abs <= PID_TOLERANCE) { 
        state_name = "ESTAVEL";
        setLeds(false, false, true); 
      } else if (setpoint > current_temperature) { 
        state_name = "AQUECER";
        setLeds(true, false, false); 
      } else { 
        state_name = "RESFRIAR";
        setLeds(false, true, false); 
      }
      
      if (millis() - lastPollingTime >= POLLING_INTERVAL) {
        sendHistory(state_name); 
        currentMode = "VERIFICACAO"; 
      }
  }
}