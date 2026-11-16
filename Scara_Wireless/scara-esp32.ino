/*
   ESP32 - Ponte WebSocket (HTML) para Serial (Arduino Uno)
   Ligação Física Unidirecional Segura:
   - ESP32 TX2 (GPIO 17) -> Arduino Uno RX (Pino 0) [Direto]
   - ESP32 GND           -> Arduino Uno GND
*/
#include <WiFi.h>
#include <WebSocketsServer.h>

#define RXD2 16 // Não utilizado (sem conexão com TX do Uno)
#define TXD2 17 // Conecta direto no Pino 0 (RX) do Arduino Uno
#define LED_PIN 2

const char* ssid = "Robo_SCARA";
const char* password = "senhadorobo";

IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

HardwareSerial SerialArduino(2);
WebSocketsServer webSocket = WebSocketsServer(81);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      digitalWrite(LED_PIN, LOW);
      Serial.println("Navegador desconectado.");
      break;
      
    case WStype_CONNECTED:
      digitalWrite(LED_PIN, HIGH);
      Serial.println("Navegador HTML conectado!");
      break;
      
    case WStype_TEXT:
      SerialArduino.write(payload, length);
      break;
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(115200);
  SerialArduino.begin(115200, SERIAL_8N1, RXD2, TXD2);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  
  Serial.println("Wi-Fi Robo_SCARA ativo! Aguardando pagina HTML...");
}

void loop() {
  webSocket.loop();
}