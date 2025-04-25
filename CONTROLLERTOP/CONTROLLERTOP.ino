/*
Controller
 versione 2.2
*/

int premutoPTT = 0;
const int beeper = 2;
const int pttTX = 3;
const int pttSDR = 4;
const int pttBASE = 5;
const int controller=10;
boolean suonoIntro=true;
boolean suonoSeBandaErrata=true;
const int delaydopoTX = 100; //prima 600
const int delaydopoSDR = 100; //600

const int NOTE_E7 = 2637;
const int NOTE_G7 = 3136;
const int NOTE_A7 = 3520;
const int NOTE_F7 = 2794;
const int NOTE_D7 = 2349;
const int NOTE_C7 = 2093;

// Define note durations (in milliseconds)
const int WHOLE = 800;
const int HALF = WHOLE / 2;
const int QUARTER = HALF / 2;
const int EIGHTH = QUARTER / 2;
const int DOTTED_QUARTER = QUARTER + EIGHTH;

void playNote(int note, int duration) {
 tone(beeper, note, duration);
 delay(duration * 1.3); // Add a slight pause between notes
}

// Variabili per il debouncing
unsigned long tempoUltimoCambio = 0;
const unsigned long tempoDebouncing = 50; // Tempo di debouncing in millisecondi

void setup() {
  Serial.begin(9600);
  
  if (suonoIntro) {
  playNote(NOTE_E7, EIGHTH);
  playNote(NOTE_G7, EIGHTH);
  playNote(NOTE_A7, QUARTER);
  playNote(0, EIGHTH); // Rest
  playNote(NOTE_F7, EIGHTH);
  playNote(NOTE_G7, EIGHTH);
  playNote(NOTE_E7, QUARTER);
  playNote(0, EIGHTH); // Rest
  playNote(NOTE_C7, EIGHTH);
  playNote(NOTE_D7, EIGHTH);
  playNote(NOTE_E7, QUARTER);
  }

  pinMode(pttBASE, INPUT); 
  pinMode(pttTX, OUTPUT);
  pinMode(pttSDR, OUTPUT);
  pinMode(beeper, OUTPUT);
  pinMode(controller, INPUT);
}

void loop() {
  
  premutoPTT = digitalRead(pttBASE);
  if (premutoPTT == HIGH && millis() - tempoUltimoCambio > tempoDebouncing) {
    /*
    Passo da rx a tx
    spengo rx
    delay
    accendo tx
    */
    delay(40);

    if (digitalRead(controller)==HIGH) {  //se HIGH, significa che il selettore banda è in posizione sbagliata.
      
      digitalWrite(pttSDR, LOW);
      delay(100);
      digitalWrite(pttTX, LOW);
      if (suonoSeBandaErrata) {
         tone(beeper, 800,1000);
        delay(500);
      }
       tempoUltimoCambio = millis();
    }
    else {
    delay(60);
    digitalWrite(pttSDR, LOW);
    delay(delaydopoSDR);
    delay(100);
    digitalWrite(pttTX, HIGH);
    tempoUltimoCambio = millis();
    }
  } else if (premutoPTT == LOW && millis() - tempoUltimoCambio > tempoDebouncing) {
    /*
    Passo da tx a rx
    spengo tx
    delay
    accendo rx
    */
    delay(60);
    digitalWrite(pttTX, LOW);
    delay(delaydopoTX);
    digitalWrite(pttSDR, HIGH);
    delay(100);
    tempoUltimoCambio = millis();
  }
}
