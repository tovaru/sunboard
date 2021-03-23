/*
    Sunboard LED System
    -------------------
    Controla el funcIonamiento de ledes WS2811, recibiendo instrucciones
    por Bluetooth.
    Este programa es para el módulo de control del Sunboard LED System.

    Diseñado para el ESP32 DevKit v1.
*/

#include "FastLED.h" // librería para el control de los ledes

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Se necesita FastLED 3.1 o más nuevo."
#endif

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0])) // calcula el tamaño de un arreglo

#include "BluetoothSerial.h" // librería para las funciones Bluetooth
#define FREC_BT_INFO        30 // la frecuencia con la que se recibe información por BT

#define NUM_LEDS            50 // cantidad de ledes conectados
#define DATA_PIN             4 // pin de datos para la tira de ledes
#define PSU_v                5 // voltaje de la fuente de alimentación
#define PSU_A             3000 // miliamperes de la fuente de alimentación
int BRIGHTNESS =            96;
int FRAMES_PER_SECOND =    120;

CRGB leds[NUM_LEDS]; // arreglo con la información para cada uno de los ledes
BluetoothSerial SerialBT; // variable para el control de la información por medio de BT

void setup() {
  FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS); // inicializar el control de los ledes
  FastLED.setMaxPowerInVoltsAndMilliamps(PSU_v, PSU_A); // ajustar la corriente máxima que se puede consumir para los ledes
  FastLED.setBrightness(BRIGHTNESS);
  leds[0] = CRGB::Red;
  FastLED.show();
  Serial.begin(115200);
  SerialBT.begin("SunBoard"); // inicializar la comunicación BT y asignar el nombre
  //prueba(); // realizar secuencia de prueba inicial
}

// Lista de demos. El primer demo es el de la función normal del dispositivo.
typedef void (*ListaFunciones[])();
ListaFunciones demos = {rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm};
ListaFunciones modos = {sunboard, demo, configurar};

int demoActual = 0; // Índice del demo actual
uint8_t matiz = 0; // rotating "base color" used by many of the patterns
int modoActual = 0; // índice del modo actual

void loop() {
  // cargar el modo que se está ejecutando actualmente
  modos[modoActual]();
}

/*
   Modo de funcionamiento normal del SunBoard en el que recibe instrucciones para
   prender y apagar ledes individuales.
*/
void sunboard() {
  while (true) {
    if (SerialBT.available()) {
      int pos = SerialBT.read();

      // posición 0 significa un cambio de modo
      if (pos == 0) {
        cambiarModo();
        break; // salir del modo actual
      } else {
        int r = SerialBT.read();
        int g = SerialBT.read();
        int b = SerialBT.read();
        Serial.print("Posicion: ");
        Serial.println(pos);
        Serial.print("RGB: ");
        Serial.print(r);
        Serial.print(",");
        Serial.print(g);
        Serial.print(",");
        Serial.println(b);
        if (pos < NUM_LEDS && pos >= 0) leds[pos] = CRGB(r, g, b);
        FastLED.show();
      }
    }
    delay(FREC_BT_INFO);
  }
}

/*
   Determina el próximo modo a ejecutarse.
   0: Modo normal del SunBoard.
   1: Demo.
   2: Configuración.
*/
void cambiarModo() {
  int numRecibido = SerialBT.read();
  if (numRecibido >= 0 && numRecibido < ARRAY_SIZE(modos)) {
    modoActual = numRecibido;
    Serial.print("Modo seleccionado: ");
    Serial.println(modoActual);
    if (modoActual == 0) {
      leds[0] = CRGB::Red;
      FastLED.show();
    }
    if (modoActual == 1) {
      numRecibido = SerialBT.read();
      if (--numRecibido >= 0 && numRecibido < ARRAY_SIZE(demos)) {
          demoActual = numRecibido;
      } else {
        demoActual = 0;
      }
      Serial.print("Demo seleccionado: ");
      Serial.println(demoActual);
    }
  } else {
    Serial.print("Modo inválido: ");
    Serial.println(numRecibido);
  }
  
}

/*
   Recibe información para cambiar variables del programa.
   Si se recibe 0, se cambia de modo, de otra forma los datos recibidos en orden son
   1ro: Brillo
   2do: FPS
*/
void configurar() {
  while (true) {
    if (SerialBT.available()) {
      int tipoConfig = SerialBT.read();
      if (tipoConfig == 0) {
        cambiarModo();
        break; // salir del modo actual
      } else {
        Serial.print("Tipo de config ");
        Serial.print(tipoConfig);
        Serial.print(", valor = ");
        switch (tipoConfig) {
          case 1:
            BRIGHTNESS = SerialBT.read();
            Serial.println(BRIGHTNESS);
            FastLED.setBrightness(BRIGHTNESS);
            break;
          case 2:
            FRAMES_PER_SECOND = SerialBT.read();
            Serial.println(FRAMES_PER_SECOND);
            break;
        }
      }
    }
    delay(FREC_BT_INFO);
  }
}

void demo() {
  while (true) {
    // cargar el demo que se está ejecutando actualmente
    demos[demoActual]();

    // actualizar los ledes
    FastLED.show();

    // aplicar el retroceso adecuada a la cantidad de FPS deseados
    FastLED.delay(1000 / FRAMES_PER_SECOND);

    // actualizaciones períodicas
    EVERY_N_MILLISECONDS(20) {
      matiz++; // actualiza el matiz (color base), esto da el aspecto del arcoíris
    }

    /*
      Determina el próximo modo a ejecutarse.
      0: Cambiar de modo.
      1: Arcoíris.
      2: Arcoíris con brillos.
      3. Confetti.
      4: Serpiente.
      5: Malabares.
      6: BMP.
    */
    EVERY_N_MILLISECONDS(FREC_BT_INFO) {
      // recibir instrucciones por BT para cambiar modo (si hay)
      if (SerialBT.available()) {
        int numRecibido = SerialBT.read();
        if (numRecibido == 0) {
          cambiarModo();
          break;
        }
        else if (--numRecibido >= 0 && numRecibido < ARRAY_SIZE(demos)) {
          demoActual = numRecibido;
          Serial.print("Demo seleccionado");
          Serial.println(demoActual);
        }
      }
    }
  }
}

/*
   Determina el próximo modo a ejecutarse.
   0: Cambiar de modo.
   1: Arcoíris.
   2: Arcoíris con brillos.
   3. Confetti.
   4: Serpiente.
   5: Malabares.
   6: BMP.

   NOTA: Esta función ya no se usa.
*/
void cambiarDemo() {
  int numRecibido = SerialBT.read();
  if (numRecibido == 0) {
    cambiarModo();
    demoActual = 0;
  } else if (--demoActual < 0 || demoActual >= ARRAY_SIZE(demos)) {
    demoActual = 0;
    Serial.print("Demo invalido: ");
    Serial.println(numRecibido);
  } else {
    demoActual = numRecibido;
    Serial.print("Demo seleccionado: ");
    Serial.println(demoActual);
  } 
}

/*
   Generador de arcoíris nativo de la librería FastLED
*/
void rainbow()
{
  fill_rainbow(leds, NUM_LEDS, matiz, 7);
}

/*
   Generador de arcoíris más brillos aleatorios
*/
void rainbowWithGlitter()
{
  rainbow();
  addGlitter(80);
}

/*
   Agrega un color blanco a un led aleatorio
*/
void addGlitter(fract8 chanceOfGlitter)
{
  if ( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

/*
   Luces de colores aleatorios que parpadean y se desvanecen lentamente.
*/
void confetti()
{
  fadeToBlackBy(leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV(matiz + random8(64), 200, 255);
}

/*
   Una línea que recorre toda la tira de ledes.
*/
void sinelon()
{
  fadeToBlackBy(leds, NUM_LEDS, 20);
  int pos = beatsin16(13, 0, NUM_LEDS - 1);
  leds[pos] += CHSV( matiz, 255, 192);
}

/*
   Tiras coloridas que pulsan en tiempos definidos (BPM).
*/
void bpm()
{
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for ( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, matiz + (i * 2), beat - matiz + (i * 10));
  }
}

/*
   Ocho puntos coloridos que se deslizan una y otra vez, como malabares.
*/
void juggle() {
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for ( int i = 0; i < 8; i++) {
    leds[beatsin16( i + 7, 0, NUM_LEDS - 1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

void randomDemo() {
  // add one to the current pattern number, and wrap around at the end
  demoActual = (demoActual + 1) % ARRAY_SIZE( demos);
}
