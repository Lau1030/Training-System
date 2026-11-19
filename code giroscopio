#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <math.h>

#define MPU6050_ADDR 0x68

#define SDA_PIN 21
#define SCL_PIN 22

// MAC DEL RECEPTOR PRINCIPAL: EC:E3:34:99:CD:18
uint8_t receptor[] = {
  0xEC, 0xE3, 0x34, 0x99, 0xCD, 0x18
};

// PAQUETE COMUN
typedef struct {
  uint8_t tipo;
  uint8_t reservado[3];
  int32_t bpm;
  uint32_t golpes;
  float velocidad;
  float temperatura;
} Paquete;

Paquete datos;

const uint8_t TIPO_MOVIMIENTO = 2;

// DETECCION DE GOLPES
const float UMBRAL_GOLPE = 2.5;
const unsigned long TIEMPO_ENTRE_GOLPES = 400;

uint32_t contadorGolpes = 0;
unsigned long ultimoGolpe = 0;
unsigned long ultimoEnvio = 0;

// ESCRIBIR REGISTRO
void escribirRegistro(uint8_t registro, uint8_t valor) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(registro);
  Wire.write(valor);
  Wire.endTransmission();
}

// LEER DATOS DE 16 BITS
int16_t leer16() {
  uint8_t alto = Wire.read();
  uint8_t bajo = Wire.read();

  return (int16_t)((alto << 8) | bajo);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("EMISOR MPU6050");

  // I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  // COMPROBAR SENSOR
  Wire.beginTransmission(MPU6050_ADDR);

  if (Wire.endTransmission() != 0) {
    Serial.println("ERROR: MPU6050 NO DETECTADO");

    while (true) {
      delay(1000);
    }
  }

  // DESPERTAR SENSOR
  escribirRegistro(0x6B, 0x00);

  // CONFIGURAR ACELEROMETRO +/- 2g
  escribirRegistro(0x1C, 0x00);

  // CONFIGURAR GIROSCOPIO +/- 250 grados/s
  escribirRegistro(0x1B, 0x00);

  delay(100);

  Serial.println("MPU6050 DETECTADO");

  // WIFI
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.print("MAC DEL SENSOR: ");
  Serial.println(WiFi.macAddress());

  // ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR ESP-NOW");

    while (true) {
      delay(1000);
    }
  }

  esp_now_peer_info_t peer = {};

  memcpy(peer.peer_addr, receptor, 6);

  peer.channel = 0;
  peer.encrypt = false;

  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("ERROR AGREGANDO RECEPTOR");

    while (true) {
      delay(1000);
    }
  }

  memset(&datos, 0, sizeof(datos));

  datos.tipo = TIPO_MOVIMIENTO;

  Serial.println("EMISOR MPU6050 LISTO");
  Serial.println("ENVIANDO AL RECEPTOR PRINCIPAL");
}

void loop() {

  // PEDIR LOS 14 BYTES DEL SENSOR
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x3B);

  if (Wire.endTransmission(false) != 0) {
    delay(100);
    return;
  }

  uint8_t cantidad = Wire.requestFrom(
    (uint8_t)MPU6050_ADDR,
    (uint8_t)14
  );

  if (cantidad != 14) {
    delay(100);
    return;
  }

  // ACELEROMETRO
  int16_t ax = leer16();
  int16_t ay = leer16();
  int16_t az = leer16();

  // TEMPERATURA
  int16_t tempRaw = leer16();

  // GIROSCOPIO
  int16_t gx = leer16();
  int16_t gy = leer16();
  int16_t gz = leer16();

  // CONVERTIR ACELERACION A G
  float axG = ax / 16384.0;
  float ayG = ay / 16384.0;
  float azG = az / 16384.0;

  float aceleracionTotal = sqrt(
    axG * axG +
    ayG * ayG +
    azG * azG
  );

  // VELOCIDAD ANGULAR EN GRADOS/SEGUNDO
  float gxDPS = gx / 131.0;
  float gyDPS = gy / 131.0;
  float gzDPS = gz / 131.0;

  float velocidad = sqrt(
    gxDPS * gxDPS +
    gyDPS * gyDPS +
    gzDPS * gzDPS
  );

  // TEMPERATURA INTERNA DEL MPU6050
  float temperatura = (tempRaw / 340.0) + 36.53;

  // DETECTAR GOLPE
  if (
    aceleracionTotal >= UMBRAL_GOLPE &&
    millis() - ultimoGolpe >= TIEMPO_ENTRE_GOLPES
  ) {
    contadorGolpes++;
    ultimoGolpe = millis();

    Serial.print("GOLPE DETECTADO: ");
    Serial.println(contadorGolpes);
  }

  // ENVIAR DATOS CADA 100 MS
  if (millis() - ultimoEnvio >= 100) {

    datos.tipo = TIPO_MOVIMIENTO;
    datos.bpm = 0;
    datos.golpes = contadorGolpes;
    datos.velocidad = velocidad;
    datos.temperatura = temperatura;

    esp_err_t resultado = esp_now_send(
      receptor,
      (uint8_t *)&datos,
      sizeof(datos)
    );

    ultimoEnvio = millis();

    Serial.print("Aceleracion: ");
    Serial.print(aceleracionTotal, 2);

    Serial.print(" g | Velocidad angular: ");
    Serial.print(velocidad, 1);

    Serial.print(" grados/s | Golpes: ");
    Serial.print(contadorGolpes);

    Serial.print(" | Temperatura: ");
    Serial.print(temperatura, 1);

    Serial.print(" | ENVIO: ");
    Serial.println(resultado == ESP_OK ? "OK" : "ERROR");
  }

  delay(10);
}(Mpu6050)
