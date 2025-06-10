/********************************************************************************************************
  TXCONNECTOR versione 3.8
*********************************************************************************************************/
//Libraries
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>    //IDE Standardtunestep
#include <Rotary.h>  //Ben Buxton https://github.com/brianlow/Rotary
#include <si5351.h>  //Etherkit https://github.com/etherkit/Si5351Arduino

#include <EEPROM.h>
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 32  // OLED display height, in pixels

#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define XT_CAL_F 162500  //11.2024
#define XT_CAL_F 158700  //3.2025
#define XT_CAL_F 16400  //06.2025
#define buttonenc 4
#define tunestep 5  //Change the pin used by encoder push button if you want.
#define pintone 7   //
#define pttext A3   //ptt
#define smeter A2
#define beeper A0     //beeper
#define pintoneout 9  //tono x accordo
#define controller 8  //tono x accordo
#define banda10 13
#define banda15 12
#define banda20 11
#define banda40 10
#define PLLB_FREQ 87600000000ULL

Rotary r = Rotary(2, 3);

String fN;
//char dC;
Si5351 si5351(0x60);
String incomingByte;
uint32_t freq;
uint32_t freqc;
uint32_t freqold;
unsigned long fstep, bfo;
byte encoder = 1;
byte stp;
unsigned int period = 100;   //millis display active
unsigned long time_now = 0;  //millis display active
int encB;
int mode;
int p_;
int bcor;  //banda corrente
//int corr1 = -24;
//int corr2 = -18;//25
int corr1 = -3;
int corr2 = -1;  //25

unsigned long tempoInizio;

boolean manual = false;
boolean rx = true;
boolean tx = false;
boolean primafreq = true;
String mode_ = "";
boolean audio_lna = true;
boolean freqbfo = true;
boolean intro = true;

int size = 2100;
float pttv;
boolean ptton = false;
boolean pttonp = false;
boolean isvfoparam = false;
boolean accordo = true;
boolean accordop = true;
boolean bandaKO = false;
//int gain = 4;
//int lna = 10;
unsigned long inizio = 0;
unsigned long fine = 0;

String versione = "v3.9";

ISR(PCINT2_vect) {
  char result = r.process();
  if (result == DIR_CW) set_frequency(1);
  else if (result == DIR_CCW) set_frequency(-1);
}

void set_frequency(short dir) {
  if (encoder == 1) {  //Up/Down frequency
    if (dir == -1) {
      freq = freq - fstep;
    }
    if (dir == 1) {
      freq = freq + fstep;
    }
  }
}

void setup() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }

  display.display();
  Serial.begin(9600);

  display.setTextSize(1);  // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);

  Wire.begin();
  pinMode(2, INPUT);
  pinMode(3, INPUT);

  pinMode(pttext, INPUT);
  pinMode(tunestep, INPUT);
  pinMode(buttonenc, INPUT);
  pinMode(pintone, INPUT);
  pinMode(banda40, INPUT);
  pinMode(banda20, INPUT);
  pinMode(banda15, INPUT);
  pinMode(banda10, INPUT);

  pinMode(pintoneout, OUTPUT);
  pinMode(controller, OUTPUT);
  pinMode(beeper, OUTPUT);

  delay(50);

  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  si5351.set_vcxo(PLLB_FREQ, 40);
  si5351.set_correction(XT_CAL_F, SI5351_PLL_INPUT_XO);
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_4MA);
  si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_4MA);
  si5351.output_enable(SI5351_CLK0, 1);
  si5351.output_enable(SI5351_CLK1, 1);
  si5351.output_enable(SI5351_CLK2, 0);
  si5351.update_status();

  freq = 7100000;
  bcor = 4;

  PCICR |= (1 << PCIE2);
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  sei();

  tunegen();

  stp = 4;
  setstep();
  layout();
  displayfreq();
  mode = 2;

  //recupero il vcalore del bfo
//   EEPROM.put(320, 8995750);
     EEPROM.get(320, bfo);
  //EEPROM.put(600, 4);
     //   EEPROM.get(600, bcor);

}

void loop() {

  if (intro) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(2, 0);
    display.print("IRIS RXTX");
    display.setTextSize(1);
    display.setCursor(26, 20);
    display.print("IU2PAQ - ALF");
    display.display();
    intro = false;
    delay(3000);
  }

  if (Serial.available() > 0) {
    incomingByte = Serial.readStringUntil('\n');
    if (!manual) freq = incomingByte.toInt();  //se sono in automatico aggiorno la frequenza
  }

  pttv = analogRead(pttext);
  if (pttv >= 800 & freq <= 29700000) {

    bandaKO = bandaErrata(freq);

    if (bandaKO) digitalWrite(controller, HIGH);
    else digitalWrite(controller, LOW);

    delay(40);
    if (tx) {
      Serial.println("u0"); //spendo SDR
      delay(100);
      Serial.println("u0");
      tx = false;
      rx = true;
      ptton = true;
    }
  } else {
    if (rx) {
      digitalWrite(controller, LOW);
      Serial.println("u1");  //accendo SDR
      delay(100);
      Serial.println("u1");
      rx = false;
      tx = true;
      ptton = false;
    }
  }

  encB = LOW;
  encB = digitalRead(buttonenc);

  if (encB == HIGH) {

    tempoInizio = millis();                   // Salviamo il tempo attuale
    while (digitalRead(buttonenc) == HIGH) {  // Aspettiamo che il pulsante venga rilasciato
    }
    unsigned long tempoTrascorso = millis() - tempoInizio;

    if (tempoTrascorso >= 3000) {
      if (manual) {
      if (freqbfo) {
        freqc = freq;
        freq = bfo;
        freqbfo = false;
      } else {
        bfo=freq;
        freq=freqc;
        freqbfo=true;
         EEPROM.put(320, bfo
         );
      }
      layout();
      }
    } else if (tempoTrascorso <= 2000) {
      tone(beeper, 300, 400);
      if (size == 2100) size = 2700;
      else if (size == 2700) size = 3900;
      else if (size == 3900) size = 2100;
      layout();
    } else if (tempoTrascorso > 4000 & tempoTrascorso <= 8000) {
    }
  }

  if (primafreq) {
    mode_ = get_modeByFreq(size);
    Serial.println(mode_);             //cambia modo (LSB/SSB) su SDR
    primafreq = false;
  } else {
    if (get_modeByFreq(size) != mode_) {
      mode_ = get_modeByFreq(size);
      Serial.println(mode_);
    }
  }

  if (digitalRead(pintone) == HIGH & ptton) {
    tone(pintoneout, 1000);
    accordo = true;
    accordop = true;
    layout();
  } else {
    accordo = false;
    if (accordop) {
      noTone(pintoneout);
      accordop = false;
      layout();
    }
  }

   if (digitalRead(pintone) == HIGH & !ptton) {

      if (!manual) {
        tone(beeper, 400, 1000);
        manual = true;
      } else {
        manual = false;
        tone(beeper, 1600, 1000);
      }
      delay(800);
      layout();
   
    }

  if (freqold != freq) {
    time_now = millis();
    tunegen();
    freqold = freq;
  }

  if (ptton != pttonp) {
    time_now = millis();
    tunegen();
    pttonp = ptton;
  }
  if (digitalRead(tunestep) == HIGH) {
    time_now = (millis() + 300);
    setstep();
    delay(300);
  }

  if ((time_now + period) > millis()) {
    displayfreq();
    layout();
  }
}

void tunegen() {

  if (freq <= 7210000) si5351.set_freq((freq + bfo + corr1) * 100, SI5351_CLK0);
  else si5351.set_freq((freq - bfo + corr1) * 100, SI5351_CLK0);
  si5351.set_freq((bfo + corr2) * 100, SI5351_CLK1);  //gab
  si5351.update_status();
}

void displayfreq() {

  display.setTextSize(2);
  fN = String(freq);
  if (freq >= 10000000)
    display.setCursor(40 - fN.length(), 1);  //13
  else
  display.setCursor(50 - fN.length(), 1);  //13
  display.print(freq);
  display.setCursor(p_, 16);  //p_1=116, 10=106,100=94,1000=81,10000=69,100000=56
  display.print("^");
  display.display();
}

void setstep() {
  switch (stp) {

      // stp=6;
    case 1:
      stp = 2;
      fstep = 1;
      p_ = 116;
      break;
    case 2:
      stp = 3;
      fstep = 10;
      p_ = 106;
      break;
    case 3:
      stp = 4;
      fstep = 100;
      p_ = 94;
      break;
    case 4:
      stp = 5;
      fstep = 1000;
      p_ = 81;
      break;
    case 5:
      stp = 6;
      fstep = 10000;
      p_ = 69;
      break;
    case 6:
      stp = 7;
      fstep = 100000;
      p_ = 56;
      break;
    case 7:
      stp = 1;
      fstep = 1000000;
      p_ = 43;
      break;
  }
}

void layout() {
  display.clearDisplay();
  displayfreq();
  display.setTextSize(1);
  display.setCursor(0, 0);
  if (ptton) display.print("TX");

  else display.print("RX");
  display.setCursor(17, 0);
  if (manual) display.print("MA");
  else display.print("AU");

  display.setCursor(0, 25);
  display.print("           ");
  display.setCursor(0, 25);
  display.print(size);

  if (accordo) {
    display.setCursor(116, 25);
    display.setCursor(116, 25);
    display.print("[");
  } else {
    display.setCursor(116, 25);
    display.print(" ");
    accordo = true;
  }
  display.setCursor(0, 8);
  display.print(versione);
  display.setCursor(0, 16);
  display.print(bfo);
  display.display();
}

String get_modeByFreq(int filter) {
  if (freq <= 7210000) return "M LSB " + String(filter);
  else if (freq > 7210000 & freq < 88000000) return "M USB " + String(filter);
  else if (freq >= 88000000) return "M WFM_ST 160000";
}

boolean bandaErrata(uint32_t freq) {
  int b40 = digitalRead(banda40);
  int b20 = digitalRead(banda20);
  int b15 = digitalRead(banda15);
  int b10 = digitalRead(banda10);

  if (!(b40 | b20 | b15 | b10)) return true;

  if (freq >= 7000000 & freq <= 7200000 & b40 == HIGH) return false;
  else if (freq >= 14000000 & freq <= 14350000 & b20 == HIGH) return false;
  else if (freq >= 21000000 & freq <= 21450000 & b15 == HIGH) return false;
  else if (freq >= 28000000 & freq <= 29700000 & b10 == HIGH) return false;
  else return true;
}

int bandaCorrente(uint32_t freq) {

  int b40 = digitalRead(banda40);
  int b20 = digitalRead(banda20);
  int b15 = digitalRead(banda15);
  int b10 = digitalRead(banda10);

  if (b40) return 4;
  if (b20) return 2;
  if (b15) return 1;
  if (b10) return 5;
}



//void setRotation(uint8_t rotation);