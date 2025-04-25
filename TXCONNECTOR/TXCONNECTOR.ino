
/********************************************************************************************************
  TXCONNECTOR
*********************************************************************************************************/
//Libraries
#include <LiquidCrystal_I2C.h>

#include <Wire.h>    //IDE Standardtunestep

#include <Rotary.h>  //Ben Buxton https://github.com/brianlow/Rotary

#include <si5351.h>  //Etherkit https://github.com/etherkit/Si5351Arduino

#include <Ethernet.h>

#include <EEPROM.h>

LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 16 chars and 2 line display

#define XT_CAL_F 162500 //11.2024
#define XT_CAL_F 158700 //3.2025

#define tunestep 5 //Change the pin used by encoder push button if you want.
#define buttonb 6
#define buttonenc 4
#define pttext A3 //pttingresso
#define smeter A2 //pttingresso
#define pintone 7 //bottone tono
#define pintoneout 9 //tono x accordo

#define PLLB_FREQ 87600000000ULL

Rotary r = Rotary(2, 3);

String fN;
char dC;
Si5351 si5351(0x60);
String incomingByte;
uint32_t freq;
uint32_t freqc;
uint32_t freqbuff;
uint32_t freqold;
uint32_t vfoparam;
unsigned long fstep, cal, calp, bfo, vfop,bfoc;
byte encoder = 1;
byte stp;
unsigned int period = 100; //millis display active
unsigned long time_now = 0; //millis display active
int band = 80;
int currB;
int encB;
int mode;
int p_;

//int corr1 = -24;
//int corr2 = -18;//25
int corr1 = -3;
int corr2 = -1;//25


unsigned long tempoInizio;
const int tempo1 = 8000; // 1 secondo in millisecondi
const int tempo2 = 3000; // 3 secondi in millisecondi
const int tempo3 = 5000; // 6 secondi in millisecondi
const int tempo4 = 1000; // 1.5 secondi in millisecondi
boolean manual = false;
boolean rx = true;
boolean tx = false;
boolean primafreq = true;
String mode_ = "";
boolean audio_lna = true;
boolean freqbfo=true;
int size = 3900;

//boolean bfon = true;  //enable-disbale bfo on CLK1

float pttv;
boolean isRX = true;
boolean ptton = false;
boolean pttonp = false;
boolean swtc = false;
boolean isvfoparam = false;
boolean accordo = true;
boolean accordop = true;
int gain = 4;
int lna = 10;
String versione = "v2.3";

ISR(PCINT2_vect) {
  char result = r.process();
  if (result == DIR_CW) set_frequency(1);
  else if (result == DIR_CCW) set_frequency(-1);
}

void set_frequency(short dir) {
  if (encoder == 1) { //Up/Down frequency
    if (dir == -1) {
      freq = freq - fstep;
    }
    if (dir == 1) {
      freq = freq + fstep;
    }
  }
}

void setup() {

  Serial.begin(9600);
  lcd.init(); // initialize the lcd
  lcd.backlight();

  Wire.begin();
  pinMode(2, INPUT);
  pinMode(3, INPUT);

  pinMode(pttext, INPUT);
  pinMode(tunestep, INPUT);
  pinMode(buttonenc, INPUT);
  pinMode(buttonb, INPUT);

  delay(500);

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

  tunegen();

  PCICR |= (1 << PCIE2);
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  sei();

  stp = 4;
  setstep();
  layout();
  displayfreq();
  mode = 2;
  //bfo = 8995800;
  /*
8996200	-18.31
8996213	-17.37
8996227	-16.55
8996240	-15.62
8996253	-14.72
8996267	-13.81
8996280	-12.98
8996293	-12.02
8996307	-11.25
8996320	-10.54
8996333	-9.79

  */
 // EEPROM.put(300, 8996280);
 EEPROM.get(300, bfo);
  //bfo = 8995800;
}

void loop() {

 
  if (Serial.available() > 0) {
    incomingByte = Serial.readStringUntil('\n');

    if (!manual) freq = incomingByte.toInt(); //se sono in automatico aggiorno la frequenza

  }

  pttv = analogRead(pttext);
  //Serial.println(pttv);
  //pttv = map(pttv, 0, 1023, 0, 100);

  if (pttv >= 800 & freq <= 29700000) {
    if (tx) {
      Serial.println("u0");
      delay(100);
      Serial.println("u0");
      tx = false;
      rx = true;
      ptton = true;
    }
  } else {
    if (rx) {
      Serial.println("u1");
      delay(100);
      Serial.println("u1");
      rx = false;
      tx = true;
      ptton = false;
    }
  }

  //delay(2000);
  encB = LOW;
  encB = digitalRead(buttonenc); //setto la modalita' automatica/manuale

  if (encB == HIGH) {

    tempoInizio = millis(); // Salviamo il tempo attuale
    while (digitalRead(buttonenc) == HIGH) { // Aspettiamo che il pulsante venga rilasciato
      //Serial.println("wait..");
    }
    unsigned long tempoTrascorso = millis() - tempoInizio;

    
    if (tempoTrascorso <= tempo1) {
           if (freqbfo ) {
         freqc=freq;
         freq=bfo;
         freqbfo=false;
       } else {
        bfo=freq;
        freq=freqc;
        freqbfo=true;
         EEPROM.put(300, bfo
         );
      } 
      layout();
    } else if (tempoTrascorso <= tempo2) {
      if (size == 2100) size = 2700;
      else if (size == 2700) size = 3900;
      else if (size == 3900) size = 2100;
      layout();
    } else if (tempoTrascorso <= tempo3) {
      if (audio_lna) audio_lna = false;
      else audio_lna = true;
      layout();
    }
    else if (tempoTrascorso >= tempo4) {

      if (!manual) {
        manual = true;
        //Serial.println("u0");
      } else {
        manual = false;
        //Serial.println("u1");
      }


      layout();
    }

  }

  /*
      if (Serial.available() > 0) {
      incomingByte = Serial.readStringUntil('\n');

      if (!manual) freq= incomingByte.toInt(); //se sono in automatico aggiorno la frequenza

      }
  */
  if (primafreq) {
    mode_ = get_modeByFreq(size);
    Serial.println(mode_);
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

  if (digitalRead(buttonb) == HIGH) {

    delay(600);
    if (audio_lna) {
      if (gain == -3) gain = -2;
      else if (gain == -3) gain = -2;
      else if (gain == -2) gain = -1;
      else if (gain == -1) gain = 0;
      else if (gain == 0) gain = 1;
      else if (gain == 1) gain = 2;
      else if (gain == 2) gain = 3;
      else if (gain == 3) gain = 4;
      else if (gain == 4) gain = 5;
      else if (gain == 5) gain = 6;
      else if (gain == 6) gain = -3;
      Serial.println("L AF " + String(gain));
      layout();
    }
    if (!audio_lna) {
      if (lna == 0) lna = 1;
      else if (lna == 1) lna = 2;
      else if (lna == 2) lna = 3;
      else if (lna == 3) lna = 4;
      else if (lna == 4) lna = 5;
      else if (lna == 5) lna = 6;
      else if (lna == 6) lna = 7;
      else if (lna == 7) lna = 8;
      else if (lna == 8) lna = 9;
      else if (lna == 9) lna = 10;
      else if (lna == 10) lna = 11;
      else if (lna == 11) lna = 12;
      else if (lna == 12) lna = 13;
      else if (lna == 13) lna = 14;
      else if (lna == 14) lna = 15;
      else if (lna == 15) lna = 16;
      else if (lna == 16) lna = 17;
      else if (lna == 17) lna = 18;
      else if (lna == 18) lna = 19;
      else if (lna == 19) lna = 20;
      else if (lna == 20) lna = 20;
      else if (lna == 21) lna = 22;
      else if (lna == 22) lna = 23;
      else if (lna == 23) lna = 24;
      else if (lna == 24) lna = 25;
      else if (lna == 25) lna = 0;
      Serial.println("L LNA_GAIN " + String(lna));
      layout();
    }
  }

}

void tunegen() {

  if (freq <= 7210000) si5351.set_freq((freq + bfo + corr1) * 100, SI5351_CLK0);
  else si5351.set_freq((freq - bfo + corr1) * 100, SI5351_CLK0);
  si5351.set_freq((bfo + corr2) * 100, SI5351_CLK1); //gab
  si5351.update_status();
}

void displayfreq() {
  lcd.clear();

  fN = String(freq);
  lcd.setCursor(13 - fN.length(), 1);
  lcd.print(freq);
  lcd.setCursor(p_, 2);
  lcd.print("^");
}

void setstep() {
  switch (stp) {
  case 1:
    stp = 2;
    fstep = 1;
    p_ = 12;
    break;
  case 2:
    stp = 3;
    fstep = 10;
    p_ = 11;
    break;
  case 3:
    stp = 4;
    fstep = 100;
    p_ = 10;
    break;
  case 4:
    stp = 5;
    fstep = 1000;
    p_ = 9;
    break;
  case 5:
    stp = 6;
    fstep = 10000;
    p_ = 8;
    break;
  case 6:
    stp = 1;
    fstep = 100000;
    p_ = 7;
    break;
  }
}

void layout() {

  lcd.setCursor(0, 0);
  if (ptton) lcd.print("TX");
  else lcd.print("RX");
  lcd.setCursor(0, 1);
  lcd.print(size);
  lcd.setCursor(3, 0);
  if (manual) lcd.print("MA");
  else lcd.print("AU");

  lcd.setCursor(0, 3);
  lcd.print("BFO");
  lcd.setCursor(4, 3);
  lcd.print(bfo);
  lcd.setCursor(13, 3);
  lcd.print("IU2PAQ");
  lcd.setCursor(16, 0);
  lcd.print(versione);
  lcd.setCursor(17, 1);
  lcd.print("   ");
  lcd.setCursor(17, 1);
  if (audio_lna) lcd.print(gain);
  else lcd.print(lna);
  lcd.setCursor(10, 0);
  lcd.print("     ");
  lcd.setCursor(10, 0);
  if (audio_lna) lcd.print("AUDIO");
  if (!audio_lna) lcd.print("LNA");

  if (accordo) {
    lcd.setCursor(19, 2);
    lcd.print("]");
    lcd.setCursor(19, 3);
    lcd.print("[");
  } else {
    lcd.setCursor(19, 2);
    lcd.print(" ");
    lcd.setCursor(19, 3);
    lcd.print(" ");
  }

}

void set_freq() {

  if (band == 80) freq = 3500000;
  if (band == 40) freq = 7100000;
  if (band == 20) freq = 14110000;
  if (band == 15) freq = 21120000;
  if (band == 11) freq = 26500000;
  if (band == 10) freq = 28200000;

}

String get_modeByFreq(int filter) {
  if (freq <= 7210000) return "M LSB " + String(filter);
  else if (freq > 7210000 & freq < 88000000) return "M USB " + String(filter);
  else if (freq >= 88000000) return "M WFM_ST 160000";
}

//void setRotation(uint8_t rotation);