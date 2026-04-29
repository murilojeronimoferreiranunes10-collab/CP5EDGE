// ============================================================
//  PROJETO IoT + FIWARE + MONITORAMENTO AMBIENTAL
// ============================================================

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ============================================================
// WIFI
// ============================================================

const char* SSID = "Wokwi-GUEST";
const char* PASSWORD = "";

// ============================================================
// MQTT
// ============================================================

const char* BROKER_MQTT = "136.111.64.236";
const int BROKER_PORT = 1883;

const char* TOPICO_SUBSCRIBE = "/TEF/lamp002/cmd";
const char* TOPICO_PUBLISH_STATUS = "/TEF/lamp002/attrs";
const char* TOPICO_PUBLISH_LUZ = "/TEF/lamp002/attrs/l";
const char* TOPICO_PUBLISH_SENSOR = "/TEF/lamp002/attrs/sensor";

const char* ID_MQTT = "fiware_001";

// ============================================================
// PINOS
// ============================================================

#define LED_ONBOARD 2

#define DHTPIN 4
#define DHTTYPE DHT22

#define LDR_PIN 34

#define RED_PIN 25
#define GREEN_PIN 26
#define BLUE_PIN 27

#define BUZZER_PIN 15

// ============================================================
// OBJETOS
// ============================================================

WiFiClient espClient;
PubSubClient MQTT(espClient);

DHT dht(DHTPIN, DHTTYPE);

// ============================================================
// ENUM ESTADOS
// ============================================================

enum Estado {
  NORMAL,
  ESTADO_OK,
  PERIGO,
  PANICO
};

// ============================================================
// VARIÁVEIS
// ============================================================

char EstadoSaida = '0';

float temperatura = 0;
float umidade = 0;
int luminosidade = 0;

Estado estadoFinal;

String causaAlerta = "OK";

const char* topicPrefix = "lamp001";

// ============================================================
// PROTÓTIPOS
// ============================================================

void initWiFi();
void reconnectMQTT();
void VerificaConexoesWiFIEMQTT();
void EnviaEstadoOutputMQTT();
void mqtt_callback(char* topic, byte* payload, unsigned int length);

bool leitura();

Estado estadoTemp(float t);
Estado estadoUmid(float u);
Estado estadoLuz(int l);

Estado maiorEstado(Estado a, Estado b, Estado c);

void detectarCausa(Estado t, Estado u, Estado l);

void setRGB(bool r, bool g, bool b);

void controlar(Estado e);

void enviarMQTT();

// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);

  // LED onboard
  pinMode(LED_ONBOARD, OUTPUT);

  // RGB
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  // buzzer
  pinMode(BUZZER_PIN, OUTPUT);

  // LDR
  pinMode(LDR_PIN, INPUT);

  dht.begin();

  initWiFi();

  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
  MQTT.setCallback(mqtt_callback);

  reconnectMQTT();

  MQTT.publish(TOPICO_PUBLISH_STATUS, "s|on");
}

// ============================================================
// LOOP
// ============================================================

void loop() {

  VerificaConexoesWiFIEMQTT();

  MQTT.loop();

  EnviaEstadoOutputMQTT();

  if (!leitura()) {
    delay(2000);
    return;
  }

  Estado t = estadoTemp(temperatura);
  Estado u = estadoUmid(umidade);
  Estado l = estadoLuz(luminosidade);

  estadoFinal = maiorEstado(t, u, l);

  detectarCausa(t, u, l);

  controlar(estadoFinal);

  enviarMQTT();

  delay(2000);
}

// ============================================================
// WIFI
// ============================================================

void initWiFi() {

  Serial.println("Conectando no WiFi...");

  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado!");
  Serial.println(WiFi.localIP());
}

// ============================================================
// MQTT CALLBACK
// ============================================================

void mqtt_callback(char* topic, byte* payload, unsigned int length) {

  String msg = "";

  for (int i = 0; i < length; i++) {

    msg += (char) payload[i];
  }

  Serial.print("Mensagem recebida: ");
  Serial.println(msg);

  String onTopic = String(topicPrefix) + "@on|";
  String offTopic = String(topicPrefix) + "@off|";

  if (msg.equals(onTopic)) {

    digitalWrite(LED_ONBOARD, HIGH);

    EstadoSaida = '1';
  }

  if (msg.equals(offTopic)) {

    digitalWrite(LED_ONBOARD, LOW);

    EstadoSaida = '0';
  }
}

// ============================================================
// RECONECTA MQTT
// ============================================================

void reconnectMQTT() {

  while (!MQTT.connected()) {

    Serial.println("Conectando MQTT...");

    if (MQTT.connect(ID_MQTT)) {

      Serial.println("MQTT conectado!");

      MQTT.subscribe(TOPICO_SUBSCRIBE);

    } else {

      Serial.println("Falha MQTT");

      delay(2000);
    }
  }
}

// ============================================================
// VERIFICA CONEXÕES
// ============================================================

void VerificaConexoesWiFIEMQTT() {

  if (WiFi.status() != WL_CONNECTED) {

    initWiFi();
  }

  if (!MQTT.connected()) {

    reconnectMQTT();
  }
}

// ============================================================
// ENVIA STATUS LED
// ============================================================

void EnviaEstadoOutputMQTT() {

  if (EstadoSaida == '1') {

    MQTT.publish(TOPICO_PUBLISH_STATUS, "s|on");

    Serial.println("LED Ligado");

  } else {

    MQTT.publish(TOPICO_PUBLISH_STATUS, "s|off");

    Serial.println("LED Desligado");
  }
}

// ============================================================
// LEITURA DOS SENSORES
// ============================================================

bool leitura() {

  float t = dht.readTemperature();
  float u = dht.readHumidity();

  if (isnan(t) || isnan(u)) {

    Serial.println("Erro DHT");

    return false;
  }

  temperatura = t;
  umidade = u;

  luminosidade = analogRead(LDR_PIN);

  Serial.printf(
    "Temp: %.1f | Umid: %.1f | Luz: %d\n",
    temperatura,
    umidade,
    luminosidade
  );

  return true;
}

// ============================================================
// REGRAS DOS ESTADOS
// ============================================================

Estado estadoTemp(float t) {

  if (t < 25) return NORMAL;
  else if (t < 30) return ESTADO_OK;
  else if (t < 35) return PERIGO;
  else return PANICO;
}

Estado estadoUmid(float u) {

  if (u > 60) return NORMAL;
  else if (u > 40) return ESTADO_OK;
  else if (u > 30) return PERIGO;
  else return PANICO;
}

Estado estadoLuz(int l) {

  if (l > 3000) return NORMAL;
  else if (l > 2000) return ESTADO_OK;
  else if (l > 1000) return PERIGO;
  else return PANICO;
}

// ============================================================
// MAIOR ESTADO
// ============================================================

Estado maiorEstado(Estado a, Estado b, Estado c) {

  Estado m = a;

  if (b > m) m = b;

  if (c > m) m = c;

  return m;
}

// ============================================================
// DETECTA CAUSA
// ============================================================

void detectarCausa(Estado t, Estado u, Estado l) {

  if (t >= PERIGO) {

    causaAlerta = "TEMPERATURA";

  } else if (u >= PERIGO) {

    causaAlerta = "UMIDADE";

  } else if (l >= PERIGO) {

    causaAlerta = "LUMINOSIDADE";

  } else {

    causaAlerta = "OK";
  }
}

// ============================================================
// RGB
// ============================================================

void setRGB(bool r, bool g, bool b) {

  digitalWrite(RED_PIN, !r);
  digitalWrite(GREEN_PIN, !g);
  digitalWrite(BLUE_PIN, !b);
}

// ============================================================
// ALERTAS
// ============================================================

void controlar(Estado e) {

  noTone(BUZZER_PIN);

  switch (e) {

    case NORMAL:

      setRGB(0, 1, 0);

      break;

    case ESTADO_OK:

      setRGB(1, 1, 1);

      break;

    case PERIGO:

      setRGB(0, 0, 1);

      tone(BUZZER_PIN, 1500);

      break;

    case PANICO:

      setRGB(1, 0, 0);

      tone(BUZZER_PIN, 2500);

      break;
  }
}

// ============================================================
// ENVIO MQTT
// ============================================================

void enviarMQTT() {

  // ========================================================
  // PAYLOAD ULTRALIGHT
  // ========================================================

  String payload = "";

  payload += "t|";
  payload += String(temperatura);

  payload += "|u|";
  payload += String(umidade);

  payload += "|l|";
  payload += String(luminosidade);

  MQTT.publish(TOPICO_PUBLISH_STATUS, payload.c_str());

  Serial.println(payload);

  // ========================================================
  // PAYLOAD JSON
  // ========================================================

  String jsonPayload = "{";

  jsonPayload += "\"temperatura\":";
  jsonPayload += String(temperatura);
  jsonPayload += ",";

  jsonPayload += "\"umidade\":";
  jsonPayload += String(umidade);
  jsonPayload += ",";

  jsonPayload += "\"luminosidade\":";
  jsonPayload += String(luminosidade);
  jsonPayload += ",";

  jsonPayload += "\"estado\":";
  jsonPayload += String((int)estadoFinal);
  jsonPayload += ",";

  jsonPayload += "\"alerta\":\"";
  jsonPayload += causaAlerta;
  jsonPayload += "\"";

  jsonPayload += "}";

  MQTT.publish(TOPICO_PUBLISH_SENSOR, jsonPayload.c_str());

  Serial.println(jsonPayload);
}