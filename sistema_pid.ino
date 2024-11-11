#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int bombaPin1 = 8;          // Bomba de llenado (principal)
const int bombaPin2 = 7;          // Bomba de drenaje (secundario)
const int potPin = A0;            // Potenciómetro para selección de nivel
const int botonPin = 2;           // Botón para confirmar inicio
const int triggerPin = 3;         // Trigger del HC-SR04
const int echoPin = 4;            // Echo del HC-SR04
double alturaSensor;               // Altura del sensor desde el fondo (variable)

LiquidCrystal_I2C lcd(0x27, 16, 2); // Pantalla LCD

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.backlight();
  pinMode(bombaPin1, OUTPUT);     // Configura pin como salida (llenado)
  pinMode(bombaPin2, OUTPUT);     // Configura pin como salida (drenaje)
  pinMode(botonPin, INPUT_PULLUP);// Configura el botón como entrada con pull-up
  pinMode(triggerPin, OUTPUT);    // Trigger del HC-SR04
  pinMode(echoPin, INPUT);        // Echo del HC-SR04
  
  // Medir la altura del sensor al inicio
  alturaSensor = medirDistancia();
  
  lcd.setCursor(0, 0);
  lcd.print("Sistema listo");     // Mensaje de inicio
  Serial.println("Sistema listo");
  delay(2000);
  lcd.clear();
}

void loop() {
  // Muestra el nivel de agua actual antes de iniciar el proceso
  double nivelAguaActual = alturaSensor - medirDistancia();
  Serial.print("Nivel de agua actual: ");
  Serial.print(nivelAguaActual);
  Serial.println(" cm");
  lcd.setCursor(0, 0);
  lcd.print("actual: ");
  lcd.print(nivelAguaActual);
  lcd.print("cm   ");

  // Lee el nivel de llenado deseado a partir del potenciómetro (1-10)
  int valorPot = analogRead(potPin);
  double setPoint = map(valorPot, 0, 1023, 1, 10);  // Convertido a centímetros
  
  // Muestra el nivel de llenado seleccionado
  Serial.print("Nivel seleccionado: ");
  Serial.print(setPoint);
  Serial.println(" cm");
  lcd.setCursor(0, 1);
  lcd.print("sel: ");
  lcd.print(setPoint);
  lcd.print("cm   ");

  // Lee el estado del botón
  int estadoBoton = digitalRead(botonPin);

  if (estadoBoton == LOW) {  // Si el botón está presionado
    Serial.println("Iniciando control de nivel...");
    lcd.setCursor(0, 1);
    lcd.print("Control iniciado ");

    // Inicia el control de nivel en el tanque
    while (true) {
      // Mide el nivel de agua en el tanque y ajusta según la altura del sensor
      double distanciaMedida = medirDistancia();
      double nivelAgua = alturaSensor - distanciaMedida; // Nivel de agua en el recipiente
      double error = setPoint - nivelAgua;

      // Imprime en la consola y la LCD el nivel actual
      Serial.print("Nivel actual: ");
      Serial.print(nivelAgua);
      Serial.print(" cm, Error: ");
      Serial.println(error);
      lcd.setCursor(0, 0);
      lcd.print("Actual: ");
      lcd.print(nivelAgua);
      lcd.print("cm  ");

      // Control de bombas según el nivel de error
      if (error > 0) { // Se necesita llenar
        digitalWrite(bombaPin1, HIGH);  // Activa la bomba de llenado
        digitalWrite(bombaPin2, LOW);   // Asegura que la bomba de drenaje esté apagada
        Serial.println("Llenando...");
        lcd.setCursor(0, 1);
        lcd.print("Llenando...     ");
      } else if (error < 0) { // Se necesita drenar
        digitalWrite(bombaPin1, LOW);   // Asegura que la bomba de llenado esté apagada
        digitalWrite(bombaPin2, HIGH);  // Activa la bomba de drenaje
        Serial.println("Drenando...");
        lcd.setCursor(0, 1);
        lcd.print("Drenando...     ");
      } else { // Nivel correcto
        digitalWrite(bombaPin1, LOW);
        digitalWrite(bombaPin2, LOW);
        Serial.println("Nivel estable.");
        lcd.setCursor(0, 1);
        lcd.print("Nivel estable  ");
        delay(2000); // Pausa para mostrar mensaje de finalización
        break;       // Sal del bucle al alcanzar el nivel deseado
      }

      delay(500); // Pausa para estabilizar el sistema
    }

    // Mensaje final cuando el proceso termina
    Serial.println("Proceso finalizado.");
    lcd.setCursor(0, 1);
    lcd.print("Proceso finalizado");
    delay(3000);  // Pausa antes de limpiar la pantalla
    lcd.clear();
  }

  delay(1000); // Actualización cada segundo antes de presionar el botón
}

// Función para medir distancia con el sensor HC-SR04 (promediado)
// Función para medir distancia con un filtro de mediana
double medirDistancia() {
  long distancias[5];
  for (int i = 0; i < 5; i++) {
    digitalWrite(triggerPin, LOW);
    delayMicroseconds(2);
    digitalWrite(triggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(triggerPin, LOW);

    long duracion = pulseIn(echoPin, HIGH);
    distancias[i] = duracion * 0.0343 / 2;  // Calcula la distancia en cm
    delay(50);  // Pequeña espera entre lecturas
  }

  // Ordena las distancias y selecciona la mediana
  for (int i = 0; i < 4; i++) {
    for (int j = i + 1; j < 5; j++) {
      if (distancias[i] > distancias[j]) {
        long temp = distancias[i];
        distancias[i] = distancias[j];
        distancias[j] = temp;
      }
    }
  }

  // La mediana está en la posición central del array ordenado
  double distanciaMediana = distancias[2];

  Serial.print("Mediana: ");
  Serial.println(distanciaMediana);  // Muestra la mediana en el serial

  return distanciaMediana;
}

