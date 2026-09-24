#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <TM1637Display.h>


// LCD

LiquidCrystal_I2C lcd(0x27, 16, 2);

// TM1637

#define TM_DIO 18
#define TM_CLK 19

TM1637Display display(TM_CLK, TM_DIO);

// JOYSTICK

#define VRX 34
#define SW 13

// VRY NO SE USA


// LEDS
// =====================================================

// Amarillo
#define LED_AMARILLO 25

// Blanco
#define LED_BLANCO 27

// =====================================================
// DATOS ESP-NOW
// =====================================================

typedef struct {
  int bpm;
} Datos;

Datos datos = {0};

volatile int bpmRecibido = 0;
volatile bool nuevoBPM = false;

// =====================================================
// ESTADOS
// =====================================================

#define MENU 0
#define ENTRENO 1
#define DESCANSO 2

int estado = MENU;

// =====================================================
// TIEMPOS
// =====================================================

int tiempoEntreno = 5;
int tiempoDescanso = 5;

// =====================================================
// TEMPORIZADOR
// =====================================================

unsigned long inicioTemporizador = 0;
unsigned long duracionTemporizador = 0;

bool temporizadorActivo = false;

// =====================================================
// JOYSTICK
// =====================================================

bool puedeMover = true;

// =====================================================
// BOTON
// =====================================================

bool botonAnterior = HIGH;

// =====================================================
// CONTROL LCD
// =====================================================

unsigned long ultimaLCD = 0;

int ultimoBPM = -1;
int ultimoEstado = -1;
int ultimoEntreno = -1;
int ultimoDescanso = -1;

// =====================================================
// ESP-NOW
// =====================================================

void recibirDatos(
  const esp_now_recv_info *info,
  const uint8_t *data,
  int len
) {

  if (len == sizeof(Datos)) {

    Datos recibido;

    memcpy(&recibido, data, sizeof(Datos));

    bpmRecibido = recibido.bpm;

    nuevoBPM = true;
  }
}

// =====================================================
// LEDS
// =====================================================

void ledsApagados() {

  digitalWrite(LED_AMARILLO, LOW);
  digitalWrite(LED_BLANCO, LOW);
}

void ledEntrenamiento() {

  digitalWrite(LED_AMARILLO, HIGH);
  digitalWrite(LED_BLANCO, LOW);
}

void ledDescanso() {

  digitalWrite(LED_AMARILLO, LOW);
  digitalWrite(LED_BLANCO, HIGH);
}

// =====================================================
// LCD
// =====================================================

void actualizarLCD(bool forzar = false) {

  // Evitar actualizar demasiado seguido

  if (!forzar) {

    if (millis() - ultimaLCD < 1000) {
      return;
    }

    if (
      datos.bpm == ultimoBPM &&
      estado == ultimoEstado &&
      tiempoEntreno == ultimoEntreno &&
      tiempoDescanso == ultimoDescanso
    ) {
      ultimaLCD = millis();
      return;
    }
  }

  // =================================================
  // BPM
  // =================================================

  lcd.setCursor(0, 0);

  lcd.print("BPM: ");

  if (datos.bpm > 0) {
    lcd.print(datos.bpm);
  } else {
    lcd.print("--");
  }

  lcd.print("           ");

  // =================================================
  // SEGUNDA LINEA
  // =================================================

  lcd.setCursor(0, 1);

  lcd.print("                ");

  lcd.setCursor(0, 1);

  if (estado == MENU) {

    lcd.print("MENU ");
    lcd.print(tiempoEntreno);
    lcd.print("/");
    lcd.print(tiempoDescanso);
    lcd.print("m");

  }

  else if (estado == ENTRENO) {

    lcd.print("ENTRENANDO");

  }

  else if (estado == DESCANSO) {

    lcd.print("DESCANSO");

  }

  ultimoBPM = datos.bpm;
  ultimoEstado = estado;
  ultimoEntreno = tiempoEntreno;
  ultimoDescanso = tiempoDescanso;

  ultimaLCD = millis();
}

// =====================================================
// TM1637
// =====================================================

void mostrarTiempo(unsigned long segundos) {

  unsigned long minutos = segundos / 60;
  unsigned long seg = segundos % 60;

  if (minutos > 99) {
    minutos = 99;
  }

  int numero = minutos * 100 + seg;

  display.showNumberDecEx(
    numero,
    0b01000000,
    true
  );
}

// =====================================================
// ENTRENAMIENTO
// =====================================================

void comenzarEntrenamiento() {

  estado = ENTRENO;

  temporizadorActivo = true;

  inicioTemporizador = millis();

  duracionTemporizador =
    (unsigned long)tiempoEntreno * 60000UL;

  ledEntrenamiento();

  mostrarTiempo(
    (unsigned long)tiempoEntreno * 60
  );

  actualizarLCD(true);

  Serial.println("ENTRENAMIENTO INICIADO");
}

// =====================================================
// DESCANSO
// =====================================================

void comenzarDescanso() {

  estado = DESCANSO;

  temporizadorActivo = true;

  inicioTemporizador = millis();

  duracionTemporizador =
    (unsigned long)tiempoDescanso * 60000UL;

  ledDescanso();

  mostrarTiempo(
    (unsigned long)tiempoDescanso * 60
  );

  actualizarLCD(true);

  Serial.println("DESCANSO INICIADO");
}

// =====================================================
// MENU
// =====================================================

void volverMenu() {

  estado = MENU;

  temporizadorActivo = false;

  ledsApagados();

  mostrarTiempo(0);

  actualizarLCD(true);

  Serial.println("MENU");
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  // =================================================
  // LEDS
  // =================================================

  pinMode(LED_AMARILLO, OUTPUT);
  pinMode(LED_BLANCO, OUTPUT);

  ledsApagados();

  // =================================================
  // JOYSTICK
  // =================================================

  pinMode(SW, INPUT_PULLUP);

  // =================================================
  // I2C
  // =================================================

  Wire.begin(21, 22);

  // Velocidad I2C estándar

  Wire.setClock(100000);

  // =================================================
  // LCD
  // =================================================

  lcd.init();

  lcd.backlight();

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Training");

  lcd.setCursor(0, 1);
  lcd.print("Sistema");

  delay(1500);

  lcd.clear();

  // =================================================
  // TM1637
  // =================================================

  display.setBrightness(5);

  display.clear();

  mostrarTiempo(
    (unsigned long)tiempoEntreno * 60
  );

  // =================================================
  // WIFI
  // =================================================

  WiFi.mode(WIFI_STA);

  Serial.print("MAC receptor: ");
  Serial.println(WiFi.macAddress());

  // =================================================
  // ESP-NOW
  // =================================================

  if (esp_now_init() != ESP_OK) {

    Serial.println("ERROR ESP-NOW");

    lcd.clear();

    lcd.print("ERROR ESP-NOW");

    return;
  }

  esp_now_register_recv_cb(recibirDatos);

  delay(500);

  actualizarLCD(true);

  Serial.println("RECEPTOR LISTO");
}

// =====================================================
// LOOP
// =====================================================

void loop() {

  // =================================================
  // JOYSTICK X
  // =================================================

  int x = analogRead(VRX);

  if (estado == MENU) {

    // DERECHA: aumentar entrenamiento

    if (x > 3000 && puedeMover) {

      tiempoEntreno++;

      if (tiempoEntreno > 99) {
        tiempoEntreno = 99;
      }

      puedeMover = false;

      actualizarLCD(true);
    }

    // IZQUIERDA: disminuir entrenamiento

    if (x < 1000 && puedeMover) {

      if (tiempoEntreno > 1) {
        tiempoEntreno--;
      }

      puedeMover = false;

      actualizarLCD(true);
    }
  }

  // Zona central

  if (x > 1600 && x < 2400) {

    puedeMover = true;
  }

  // =================================================
  // BOTON
  // =================================================

  bool botonActual = digitalRead(SW);

  if (
    botonAnterior == HIGH &&
    botonActual == LOW
  ) {

    delay(30);

    if (digitalRead(SW) == LOW) {

      if (estado == MENU) {

        comenzarEntrenamiento();
      }

      else if (estado == ENTRENO) {

        comenzarDescanso();
      }

      else if (estado == DESCANSO) {

        volverMenu();
      }

      // Esperar a soltar

      while (digitalRead(SW) == LOW) {
        delay(10);
      }
    }
  }

  botonAnterior = botonActual;

  // =================================================
  // BPM RECIBIDO
  // =================================================

  if (nuevoBPM) {

    datos.bpm = bpmRecibido;

    nuevoBPM = false;

    // NO actualizamos inmediatamente.
    // La LCD se actualizará como máximo cada 1 segundo.
  }

  // =================================================
  // LCD CADA 1 SEGUNDO
  // =================================================

  if (millis() - ultimaLCD >= 1000) {

    actualizarLCD();
  }

  // =================================================
  // TEMPORIZADOR
  // =================================================

  if (temporizadorActivo) {

    unsigned long transcurrido =
      millis() - inicioTemporizador;

    if (transcurrido >= duracionTemporizador) {

      if (estado == ENTRENO) {

        comenzarDescanso();
      }

      else if (estado == DESCANSO) {

        volverMenu();
      }

    }

    else {

      unsigned long restante =
        (duracionTemporizador - transcurrido) / 1000;

      mostrarTiempo(restante);
    }
  }

  delay(20);
}
