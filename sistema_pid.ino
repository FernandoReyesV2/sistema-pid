#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int bombaPin1 = 8;          // Bomba de llenado (principal)
const int bombaPin2 = 7;          // Bomba de drenaje (secundario)
const int potPin = A0;            // Potenciómetro para selección de nivel
const int botonPin = 2;           // Botón para confirmar inicio
const int triggerPin = 3;         // Trigger del HC-SR04
const int echoPin = 4;            // Echo del HC-SR04
double alturaSensor;              // Altura del sensor desde el fondo (variable)
const int numLecturas = 10; // Número de lecturas a promediar
const float velocidadSonido = 0.0343; // Velocidad del sonido en cm/μs


LiquidCrystal_I2C lcd(0x27, 16, 2); // Pantalla LCD

// Parámetros PID
double Kp = 1.0, Ki = 0.5, Kd = 0.1; // Coeficientes PID
double errorSum = 0;                 // Acumulador de error para la integral
double lastError = 0;                // Error anterior para la derivada
unsigned long lastTime = 0;          // Último tiempo de cálculo PID

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
  // alturaSensor = 10;

  
  lcd.setCursor(0, 0);
  lcd.print("Sistema listo");     // Mensaje de inicio
  Serial.println("Sistema listo");
  delay(2000);
  lcd.clear();
}

void loop() {
  double nivelAguaActual = alturaSensor - medirDistancia();
  Serial.print("Nivel de agua actual: ");
  Serial.print(nivelAguaActual);
  Serial.println(" cm");
  lcd.setCursor(0, 0);
  lcd.print("actual: ");
  lcd.print(nivelAguaActual);
  lcd.print("cm   ");

  int valorPot = analogRead(potPin);
  double setPoint = map(valorPot, 0, 1023, 0, 10);  // Convertido a centímetros
  
  Serial.print("Nivel seleccionado: ");
  Serial.print(setPoint);
  Serial.println(" cm");
  lcd.setCursor(0, 1);
  lcd.print("sel: ");
  lcd.print(setPoint);
  lcd.print("cm   ");

  int estadoBoton = digitalRead(botonPin);

  if (estadoBoton == LOW) {  // Si el botón está presionado
    Serial.println("Iniciando control de nivel...");
    lcd.setCursor(0, 1);
    lcd.print("Control iniciado ");

    while (true) {
      double distanciaMedida = medirDistancia();
      double nivelAgua = alturaSensor - distanciaMedida;
      double error = setPoint - nivelAgua;
      double output = calcularPID(error);

      Serial.print("Nivel actual: ");
      Serial.print(nivelAgua);
      Serial.print(" cm, Error: ");
      Serial.print(error);
      Serial.print(", Salida PID: ");
      Serial.println(output);

      lcd.setCursor(0, 0);
      lcd.print("Actual: ");
      lcd.print(nivelAgua);
      lcd.print("cm  ");

      if (error > 0) { // Se necesita llenar
        digitalWrite(bombaPin1, HIGH);  // Activa la bomba de llenado
        digitalWrite(bombaPin2, LOW);   // Asegura que la bomba de drenaje esté apagada
        Serial.println("Llenando...");
        lcd.setCursor(0, 1);
        lcd.print("Llenando...     ");
        delay(1000); // La bomba se mantiene encendida por 0.5 segundos
        digitalWrite(bombaPin1, LOW);   // Apaga la bomba de llenado

      } else if (error < 0) { // Se necesita drenar
        digitalWrite(bombaPin1, LOW);   // Asegura que la bomba de llenado esté apagada
        digitalWrite(bombaPin2, HIGH);  // Activa la bomba de drenaje
        Serial.println("Drenando...");
        lcd.setCursor(0, 1);
        lcd.print("Drenando...     ");
        delay(500); // La bomba de drenaje se mantiene encendida por 0.5 segundos
        digitalWrite(bombaPin2, LOW);   // Apaga la bomba de drenaje

      } else { // Nivel correcto
        digitalWrite(bombaPin1, LOW);
        digitalWrite(bombaPin2, LOW);
        Serial.println("Nivel estable.");
        lcd.setCursor(0, 1);
        lcd.print("Nivel estable  ");
        delay(2000); // Pausa para mostrar mensaje de finalización
        break;       // Sal del bucle al alcanzar el nivel deseado
      }

      delay(3000); // Espera 2 segundos para estabilizar la medición
    }

    Serial.println("Proceso finalizado.");
    lcd.setCursor(0, 1);
    lcd.print("Proceso finalizado");
    delay(3000);
    lcd.clear();
  }

  delay(1000); // Actualización cada segundo antes de presionar el botón
}

double medirDistancia() {
    long totalDistancia = 0;
    const int numLecturas = 50; // Cambia a un número menor si es necesario
    for (int i = 0; i < numLecturas; i++) {
        digitalWrite(triggerPin, LOW);
        delayMicroseconds(2);
        digitalWrite(triggerPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(triggerPin, LOW);

        long duracion = pulseIn(echoPin, HIGH);
        long distancia = duracion * 0.0343 / 2;  // Calcula la distancia en cm
        totalDistancia += distancia;
        delay(50);  // Pequeña espera entre lecturas
    }

    double promedioDistancia = totalDistancia / (double)numLecturas;
    Serial.println(promedioDistancia);
    return promedioDistancia;
}
double calcularPID(double error) {
  unsigned long now = millis();
  double timeChange = (double)(now - lastTime) / 1000.0;

  errorSum += error * timeChange;
  double dError = (error - lastError) / timeChange;

  double output = Kp * error + Ki * errorSum + Kd * dError;

  lastError = error;
  lastTime = now;

  return output;
}
