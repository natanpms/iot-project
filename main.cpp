/*
 * ====================================
 * ESP8266 + DHT11/22 + MQTT (HiveMQ)
 * ====================================
 * 
 * Envia dados de temperatura e umidade via MQTT
 * para o broker público HiveMQ
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ===== CONFIGURAÇÕES WiFi =====
const char* ssid = "";           // Nome da sua rede WiFi
const char* password = "";      // Senha do WiFi

// ===== CONFIGURAÇÕES MQTT (HiveMQ Público) =====
const char* mqtt_server = "broker.hivemq.com";  // Broker público HiveMQ
const int mqtt_port = 1883;                     // Porta padrão MQTT

// Como o HiveMQ é público, outras pessoas podem ver seus dados
// Use um identificador único (url do trabalho)
const char* topic_temperature = "graduacao/iot/grupo_3/temperatura";
const char* topic_humidity = "graduacao/iot/grupo_3/umidade";
const char* topic_status = "graduacao/iot/grupo_3/status";

// ===== CONFIGURAÇÕES DHT =====
#define DHTPIN 2        // GPIO2 = D4
#define DHTTYPE DHT11   // Ou DHT11

// ===== VARIÁVEIS GLOBAIS =====
DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
const long interval = 10000;  // Envia dados a cada 10 segundos

// ===== FUNÇÃO: Conectar WiFi =====
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.println("========================================");
  Serial.print("Conectando ao WiFi: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    tentativas++;
    
    if (tentativas > 40) {  // 20 segundos
      Serial.println();
      Serial.println("Falha ao conectar no WiFi!");
      Serial.println("Verifique o nome da rede e senha.");
      Serial.println("Reiniciando em 5 segundos...");
      delay(5000);
      ESP.restart();
    }
  }
  
  Serial.println();
  Serial.println("✓ WiFi conectado!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Intensidade do sinal: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  Serial.println("========================================");
  Serial.println();
}

// ===== FUNÇÃO: Reconectar MQTT =====
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao HiveMQ (broker público)... ");
    
    // Gera um ID único para o cliente
    String clientId = "ESP8266-";
    clientId += String(random(0xffff), HEX);
    
    // Tenta conectar (HiveMQ público não precisa de usuário/senha)
    if (client.connect(clientId.c_str())) {
      Serial.println("✓ Conectado!");
      Serial.println("Broker: broker.hivemq.com:1883");
      
      // Publica mensagem de status
      client.publish(topic_status, "online", true);
      
      Serial.println();
      Serial.println("Tópicos MQTT:");
      Serial.print("   Temperatura: ");
      Serial.println(topic_temperature);
      Serial.print("   Umidade:     ");
      Serial.println(topic_humidity);
      Serial.print("   Status:      ");
      Serial.println(topic_status);
      Serial.println();
      
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" | Tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println("\n\n\n");
  Serial.println("========================================");
  Serial.println("  ESP8266 + DHT + MQTT (HiveMQ)");
  Serial.println("========================================");
  Serial.println();
  
  // Inicia sensor DHT
  Serial.println("Iniciando sensor DHT...");
  dht.begin();
  delay(2000);
  Serial.println("✓ Sensor DHT iniciado!");
  Serial.println();
  
  // Conecta WiFi
  setup_wifi();
  
  // Configura servidor MQTT
  client.setServer(mqtt_server, mqtt_port);
  
  Serial.println("Sistema pronto!");
  Serial.println("========================================");
  Serial.println();
}

// ===== LOOP =====
void loop() {
  // Mantém conexão MQTT
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  // Envia dados a cada intervalo definido
  unsigned long now = millis();
  if (now - lastMsg > interval) {
    lastMsg = now;
    
    // Lê sensor
    Serial.println(" Lendo sensor DHT...");
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    
    // Verifica se leitura foi bem sucedida
    if (isnan(h) || isnan(t)) {
      Serial.println(" Erro ao ler sensor DHT!");
      Serial.println();
      return;
    }
    
    // Exibe valores no Serial
    Serial.println();
    Serial.println("╔════════════════════════════════════╗");
    Serial.println("║      DADOS COLETADOS               ║");
    Serial.println("╚════════════════════════════════════╝");
    Serial.print("  Temperatura: ");
    Serial.print(t, 1);
    Serial.println(" °C");
    Serial.print(" Umidade:      ");
    Serial.print(h, 1);
    Serial.println(" %");
    Serial.println();
    
    // Converte valores para String
    char tempString[8];
    char humString[8];
    dtostrf(t, 4, 2, tempString);
    dtostrf(h, 4, 2, humString);
    
    // Publica no MQTT
    Serial.println(" Publicando no MQTT (HiveMQ)...");
    
    if (client.publish(topic_temperature, tempString)) {
      Serial.print(" Temperatura publicada: ");
      Serial.print(tempString);
      Serial.println(" °C");
    } else {
      Serial.println(" Falha ao publicar temperatura");
    }
    
    if (client.publish(topic_humidity, humString)) {
      Serial.print("Umidade publicada: ");
      Serial.print(humString);
      Serial.println(" %");
    } else {
      Serial.println(" Falha ao publicar umidade");
    }
    
    Serial.println();
    Serial.println("----------------------------------------");
    Serial.println("Aguardando 10 segundos...");
    Serial.println();
  }
}
