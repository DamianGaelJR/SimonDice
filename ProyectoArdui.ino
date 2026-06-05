// =============================================
// SIMÓN DICE IoT - Versión Calibrada TCS3200
// SIN SERVO - Solo componentes que tienes
// Color detectado con NOMBRE COMPLETO
// =============================================

String sequence = "";
int roundNumber = 1;
int gameSpeed = 800;
unsigned long reactionStart = 0;
bool gameActive = false;

// ================== PINES ==================
const int trigPin = 9;
const int echoPin = 10;
const int pirPin = 2;

const int TCS_S0 = 3;
const int TCS_S1 = 4;
const int TCS_S2 = 5;
const int TCS_S3 = 6;
const int TCS_OUT = 7;

const int LED_R = 11;
const int LED_G = 12;
const int LED_B = 13;

// ================== FUNCIONES ==================

long getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  return duration * 0.034 / 2;
}

char readColor() {
  digitalWrite(TCS_S0, HIGH);
  digitalWrite(TCS_S1, LOW);   // 20% escala

  // Lecturas RAW
  digitalWrite(TCS_S2, LOW);  digitalWrite(TCS_S3, LOW);
  int rawRed = pulseIn(TCS_OUT, LOW, 15000);

  digitalWrite(TCS_S2, HIGH); digitalWrite(TCS_S3, HIGH);
  int rawGreen = pulseIn(TCS_OUT, LOW, 15000);

  digitalWrite(TCS_S2, LOW);  digitalWrite(TCS_S3, HIGH);
  int rawBlue = pulseIn(TCS_OUT, LOW, 15000);

  // Calibración según tus mediciones
  if (rawRed < 45 && rawGreen > 90 && rawBlue > 60) return 'R';  // Rojo
  if (rawGreen < 45 && rawBlue > 50) return 'G';                 // Verde
  if (rawRed > 65 && rawGreen < 50 && rawBlue < 40) return 'B';  // Azul

  return 'X';  // Ninguno
}

String getColorName(char color) {
  if (color == 'R') return "Rojo";
  if (color == 'G') return "Verde";
  if (color == 'B') return "Azul";
  return "Ninguno";
}

void showColor(char color) {
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, LOW);
  
  if (color == 'R') digitalWrite(LED_R, HIGH);
  else if (color == 'G') digitalWrite(LED_G, HIGH);
  else if (color == 'B') digitalWrite(LED_B, HIGH);
  
  delay(400);
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, LOW);
  delay(200);
}

void generateSequence() {
  const char colors[] = {'R', 'G', 'B'};
  sequence += colors[random(0, 3)];
}

void sendDataRow(int distance, int motion, char detectedChar, int r, int g, int b,
                 unsigned long reactionTime, int success, int speed) {
  
  String colorName = getColorName(detectedChar);

  Serial.print(millis()); Serial.print(",");
  Serial.print(distance); Serial.print(",");
  Serial.print(motion); Serial.print(",");
  Serial.print(colorName); Serial.print(",");
  Serial.print(r); Serial.print(",");
  Serial.print(g); Serial.print(",");
  Serial.print(b); Serial.print(",");
  Serial.print(reactionTime); Serial.print(",");
  Serial.print(roundNumber); Serial.print(",");
  Serial.print(success); Serial.print(",");
  Serial.print(speed); Serial.print(",");
  Serial.print(sequence.length()); Serial.println();
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0));

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(pirPin, INPUT);
  pinMode(TCS_S0, OUTPUT);
  pinMode(TCS_S1, OUTPUT);
  pinMode(TCS_S2, OUTPUT);
  pinMode(TCS_S3, OUTPUT);
  pinMode(TCS_OUT, INPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  Serial.println("timestamp_ms,distancia_cm,movimiento,color_detectado,r,g,b,tiempo_reaccion_ms,ronda,acierto,velocidad_ms,longitud_secuencia");
  Serial.println("=== SIMON DICE IOT INICIADO - Calibración TCS3200 Activa ===");
}

// ================== LOOP ==================
void loop() {
  int distance = getDistance();
  int motion = digitalRead(pirPin);

  if (!gameActive && motion == HIGH && distance < 80 && distance > 5) {
    delay(1200);
    gameActive = true;
    roundNumber = 1;
    sequence = "";
    Serial.println("=== NUEVA SESIÓN INICIADA ===");
  }

  if (!gameActive) {
    delay(100);
    return;
  }

  generateSequence();
  gameSpeed = constrain(800 - (roundNumber * 28), 280, 800);

  // Mostrar secuencia
  for (char c : sequence) {
    showColor(c);
  }

  // Esperar respuestas del jugador
  for (int i = 0; i < sequence.length(); i++) {
    reactionStart = millis();
    char userColor = 'X';
    unsigned long timeout = millis();

    while (millis() - timeout < 6000) {
      userColor = readColor();
      if (userColor != 'X') break;
      delay(50);
    }

    unsigned long reactionTime = millis() - reactionStart;
    int success = (userColor == sequence[i]) ? 1 : 0;

    // RGB aproximados
    int r = (userColor == 'R') ? 255 : 40;
    int g = (userColor == 'G') ? 255 : 40;
    int b = (userColor == 'B') ? 255 : 40;

    sendDataRow(distance, motion, userColor, r, g, b, reactionTime, success, gameSpeed);

    if (success == 0) {
      for (int j = 0; j < 6; j++) {
        digitalWrite(LED_R, HIGH); delay(120);
        digitalWrite(LED_R, LOW);  delay(120);
      }
      gameActive = false;
      Serial.println("=== SESIÓN TERMINADA - FALLO ===");
      return;
    }
    delay(150);
  }

  roundNumber++;
  if (roundNumber > 25) {
    gameActive = false;
    Serial.println("=== SESIÓN TERMINADA - MÁXIMO RONDAS ===");
  }

  delay(600);
}