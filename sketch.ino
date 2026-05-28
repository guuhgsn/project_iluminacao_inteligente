/*
 * Sistema de Iluminação Pública Inteligente para Cidades Sustentáveis
 * Autor: Gustavo Gonçalves dos Santos Neves
 * Universidade Presbiteriana Mackenzie - FCI
 *
 * Hardware:
 *   - ESP32 DevKit V1
 *   - Sensor LDR (pino GPIO 34 - ADC)
 *   - Módulo Relé 1 canal (pino GPIO 23)
 *   - LED amarelo (representação da lâmpada pública)
 *
 * Comunicação:
 *   - Protocolo MQTT via broker HiveMQ Cloud (público)
 *   - Tópico de publicação: cidade/iluminacao/sensor
 *   - Tópico de assinatura: cidade/iluminacao/comando
 *
 * Compatível com o simulador Wokwi (https://wokwi.com)
 */

#include <WiFi.h>
#include <PubSubClient.h>

// ============================================================
//  CONFIGURAÇÕES DE REDE
// ============================================================
const char* WIFI_SSID     = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// ============================================================
//  CONFIGURAÇÕES DO BROKER MQTT
// ============================================================
const char* MQTT_SERVER   = "broker.hivemq.com";
const int   MQTT_PORT     = 1883;
const char* MQTT_CLIENT   = "esp32_iluminacao_pub_ggsn";

// ============================================================
//  TÓPICOS MQTT
// ============================================================
const char* TOPIC_SENSOR  = "cidade/iluminacao/sensor";
const char* TOPIC_COMANDO = "cidade/iluminacao/comando";

// ============================================================
//  PINOS DO HARDWARE
// ============================================================
const int LDR_PIN   = 34;
const int RELAY_PIN = 23;

// ============================================================
//  PARÂMETROS DE CONTROLE
// ============================================================
const int  LIMIAR_LUMINOSIDADE = 2000;
// Intervalo de leitura e publicação (ms)
const long INTERVALO_MS        = 5000;

// ============================================================
//  VARIÁVEIS GLOBAIS
// ============================================================
WiFiClient    espClient;
PubSubClient  mqttClient(espClient);

bool  modoManual   = false;
bool  estadoRele   = false;
unsigned long ultimoEnvio = 0;

// ============================================================
//  PROTÓTIPOS
// ============================================================
void conectarWifi();
void conectarMQTT();
void callbackMQTT(char* topic, byte* payload, unsigned int length);
void publicarDados(int ldrValor);
void controlarRele(bool ligar);

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println(F("==========================================="));
  Serial.println(F("  Iluminacao Publica Inteligente - ESP32  "));
  Serial.println(F("  Mackenzie - Gustavo G. S. Neves   "));
  Serial.println(F("==========================================="));

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  conectarWifi();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(callbackMQTT);
}

// ============================================================
//  LOOP PRINCIPAL
// ============================================================
void loop() {
  if (!mqttClient.connected()) {
    conectarMQTT();
  }
  mqttClient.loop();

  unsigned long agora = millis();

  if (agora - ultimoEnvio >= INTERVALO_MS) {
    ultimoEnvio = agora;

    unsigned long t0Sensor = millis();
    int ldrValor = analogRead(LDR_PIN);
    unsigned long latenciaSensor = millis() - t0Sensor;

    Serial.print(F("[SENSOR] LDR ADC = "));
    Serial.print(ldrValor);
    Serial.print(F(" | Latencia leitura = "));
    Serial.print(latenciaSensor);
    Serial.println(F(" ms"));

    if (!modoManual) {
      if (ldrValor < LIMIAR_LUMINOSIDADE) {
        Serial.println(F("[AUTO] Ambiente escuro -> Lampada LIGADA"));
        controlarRele(true);
      } else {
        Serial.println(F("[AUTO] Ambiente claro  -> Lampada DESLIGADA"));
        controlarRele(false);
      }
    }

    publicarDados(ldrValor);
  }
}

// ============================================================
//  CONEXÃO WI-FI
// ============================================================
void conectarWifi() {
  Serial.print(F("[WiFi] Conectando a: "));
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(F("."));
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print(F("[WiFi] Conectado! IP: "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("\n[WiFi] ERRO: Falha ao conectar."));
  }
}

// ============================================================
//  CONEXÃO MQTT
// ============================================================
void conectarMQTT() {
  while (!mqttClient.connected()) {
    Serial.print(F("[MQTT] Conectando ao broker "));
    Serial.print(MQTT_SERVER);
    Serial.print(F("..."));

    if (mqttClient.connect(MQTT_CLIENT)) {
      Serial.println(F(" OK!"));

      mqttClient.subscribe(TOPIC_COMANDO);
      Serial.print(F("[MQTT] Inscrito no topico: "));
      Serial.println(TOPIC_COMANDO);

      mqttClient.publish(TOPIC_SENSOR, "{\"status\":\"online\",\"dispositivo\":\"esp32_iluminacao\"}");
    } else {
      Serial.print(F(" FALHA! rc="));
      Serial.print(mqttClient.state());
      Serial.println(F(" Tentando novamente em 5s..."));
      delay(5000);
    }
  }
}

// ============================================================
//  CALLBACK: MENSAGEM RECEBIDA VIA MQTT
// ============================================================
void callbackMQTT(char* topic, byte* payload, unsigned int length) {
  String mensagem = "";
  for (unsigned int i = 0; i < length; i++) {
    mensagem += (char)payload[i];
  }

  Serial.print(F("[MQTT] Msg recebida em '"));
  Serial.print(topic);
  Serial.print(F("': "));
  Serial.println(mensagem);

  unsigned long t0Atuador = millis();

  // Processa comandos
  if (mensagem.equalsIgnoreCase("LIGAR")) {
    modoManual = true;
    controlarRele(true);
    Serial.println(F("[ATUADOR] Comando MANUAL: Lampada LIGADA"));

  } else if (mensagem.equalsIgnoreCase("DESLIGAR")) {
    modoManual = true;
    controlarRele(false);
    Serial.println(F("[ATUADOR] Comando MANUAL: Lampada DESLIGADA"));

  } else if (mensagem.equalsIgnoreCase("AUTO")) {
    modoManual = false;
    Serial.println(F("[CONTROLE] Modo AUTOMATICO restaurado"));

  } else {
    Serial.println(F("[MQTT] Comando desconhecido. Use: LIGAR | DESLIGAR | AUTO"));
  }

  unsigned long latenciaAtuador = millis() - t0Atuador;
  Serial.print(F("[TEMPO] Latencia acionamento atuador: "));
  Serial.print(latenciaAtuador);
  Serial.println(F(" ms"));
}

// ============================================================
//  CONTROLE DO RELÉ
// ============================================================
void controlarRele(bool ligar) {
  estadoRele = ligar;
  digitalWrite(RELAY_PIN, ligar ? HIGH : LOW);
}

// ============================================================
//  PUBLICAÇÃO DOS DADOS NO BROKER MQTT
// ============================================================
void publicarDados(int ldrValor) {
  String payload = "{";
  payload += "\"luminosidade_adc\":" + String(ldrValor) + ",";
  payload += "\"percentual_luz\":" + String(map(ldrValor, 0, 4095, 0, 100)) + ",";
  payload += "\"lampada\":\"" + String(estadoRele ? "LIGADA" : "DESLIGADA") + "\",";
  payload += "\"modo\":\"" + String(modoManual ? "MANUAL" : "AUTOMATICO") + "\",";
  payload += "\"uptime_ms\":" + String(millis());
  payload += "}";

  unsigned long t0Pub = millis();
  bool ok = mqttClient.publish(TOPIC_SENSOR, payload.c_str());
  unsigned long latenciaPub = millis() - t0Pub;

  Serial.print(F("[MQTT] Publicado em '"));
  Serial.print(TOPIC_SENSOR);
  Serial.print(F("': "));
  Serial.println(payload);
  Serial.print(F("[TEMPO] Latencia publicacao MQTT: "));
  Serial.print(latenciaPub);
  Serial.print(F(" ms | Status: "));
  Serial.println(ok ? F("OK") : F("FALHA"));
  Serial.println(F("-------------------------------------------"));
}