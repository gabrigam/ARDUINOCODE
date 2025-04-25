/*
Controller
*/

int pulsanteTX = 0; // Sostituisci con il pin a cui è collegato il pulsante

const int pttBASE = 2;
const int pttTX = 3;
const int pttSDR = 4;
boolean stato = false;

const int delaydopoTX = 200; //prima 600
const int delaydopoSDR = 200; //600

// Variabili per il debouncing
unsigned long tempoUltimoCambio = 0;
const unsigned long tempoDebouncing = 50; // Tempo di debouncing in millisecondi

void setup() {
  pinMode(pttBASE, INPUT); // Configuriamo il pin del pulsante come input con pull-up interno
  pinMode(pttTX, OUTPUT);
  pinMode(pttSDR, OUTPUT);
}

void loop() {

  pulsanteTX = digitalRead(pttBASE);

  if (pulsanteTX == HIGH && millis() - tempoUltimoCambio > tempoDebouncing) {
    /*
    Passo da rx a tx
    spengo rx
    delay
    accendo tx
    */
    digitalWrite(pttSDR, LOW);
    delay(delaydopoSDR);
    digitalWrite(pttTX, HIGH);
    tempoUltimoCambio = millis();
  } else if (pulsanteTX == LOW && millis() - tempoUltimoCambio > tempoDebouncing) {
    /*
    Passo da tx a rx
    spengo tx
    delay
    accendo rx
    */
    digitalWrite(pttTX, LOW);
    delay(delaydopoTX);
    digitalWrite(pttSDR, HIGH);
    tempoUltimoCambio = millis();
  }
}