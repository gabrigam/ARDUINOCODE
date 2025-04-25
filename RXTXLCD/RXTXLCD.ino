/********************************************************************************************************
  rf rxtxgenerator fot rx e tx radio iu2paq LCD versione LCD
*********************************************************************************************************/
//v4 rotation
//V2.1 11/1 2024
//V2.2 Feb 2024

//Libraries
#include <LiquidCrystal_I2C.h>

#include <Wire.h>    //IDE Standardtunestep

#include <Rotary.h>  //Ben Buxton https://github.com/brianlow/Rotary

#include <si5351.h>  //Etherkit https://github.com/etherkit/Si5351Arduino

#include <EEPROM.h>


LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 16 chars and 2 line display

#define XT_CAL_F 162500 //11.2024

#define tunestep 5 //Change the pin used by encoder push button if you want.
#define buttonb 6
#define buttonenc 4
#define pttext A0 //pttingresso
#define smeter A2 //pttingresso
#define pttrele 10 //accende led se tx
#define pintone 7 //bottone tono
#define pintoneout 9 //tono x accordo
//#define basefreq 7055000;
#define vfotx 8996800

#define PLLB_FREQ 87600000000ULL

Rotary r = Rotary(2, 3);

String fN;
char dC;
Si5351 si5351(0x60);

uint32_t freq;
uint32_t freqbuff;
uint32_t freqold;
uint32_t vfoparam;
unsigned long fstep, cal, calp, vfo, vfop, vfo40, vfo20, vfo11, vfo15, vfo10;
byte encoder = 1;
byte stp;
unsigned int period = 100; //millis display active
unsigned long time_now = 0; //millis display active
int band = 80;
int currB;
int encB;
int mode;
int p_;

int amp1 = 4;
int amp2 = 4;

int corr1 = 18;
int corr2 = 25;
int index = 2;

uint32_t freqA[2] = {
  14140000,
  14270000
};
uint32_t bfoA[2] = {
  8998500,
  8998200
};

int freqAV[2] = {
  20,
  25
};

unsigned long tempoInizio;
const int tempo1 = 1000; // 1 secondo in millisecondi
const int tempo2 = 3000; // 3 secondi in millisecondi
const int tempo3 = 6000; // 6 secondi in millisecondi
const int tempo4 = 1000; // 1.5 secondi in millisecondi

boolean bfon = true; //enable-disbale bfo on CLK1

float pttv;
boolean isRX = true;
boolean ptton = false;
boolean pttonp = false;
boolean swtc = false;
boolean isvfoparam = false;
boolean accordo = true;
boolean accordop = true;

String version = "v1.6";

ISR(PCINT2_vect) {
  char result = r.process();
  if (result == DIR_CW) set_frequency(1);
  else if (result == DIR_CCW) set_frequency(-1);
}

void set_frequency(short dir) {
  if (encoder == 1) { //Up/Down frequency
    if (dir == -1) {
      if (band == 0) {
        cal = cal - fstep;
      } else {
        if (band == 1) {
          vfo = vfo - fstep;
        } else {
          freq = freq - fstep;
          check_freq(-1);
        }
      }
    }
    if (dir == 1) {
      if (band == 0) {
        cal = cal + fstep;
      } else {
        if (band == 1) {
          vfo = vfo + fstep;
          Serial.println(freq);
          Serial.println(vfo);
        } else {
          freq = freq + fstep;
          check_freq(1);
        }
      }
    }
  }
}

void setup() {

  Serial.begin(57600);
  lcd.init(); // initialize the lcd
  lcd.backlight();
  //lcd.backlight();
  Wire.begin();
  //EEPROM.put(0, 8998200);
  //EEPROM.put(100,8998200);

  /*
  EEPROM.put(200,8996200);
  EEPROM.put(220,8996200);
  EEPROM.put(240,8996200);
  EEPROM.put(260,8996200);
  EEPROM.put(280,8996200);
  */

  //EEPROM.get(0, vfo);
  EEPROM.get(200, vfo40);
  EEPROM.get(220, vfo20);
  EEPROM.get(240, vfo15);
  EEPROM.get(260, vfo11);
  EEPROM.get(280, vfo10);

  // EEPROM.put(30, 4);
  // EEPROM.put(60, 4);
  EEPROM.get(30, amp1);
  EEPROM.get(60, amp2);

  pinMode(2, INPUT);
  pinMode(3, INPUT);
  pinMode(pttext, INPUT);
  //pinMode(tunestep, INPUT_PULLUP);
  pinMode(tunestep, INPUT);
  pinMode(buttonb, INPUT);
  pinMode(buttonenc, INPUT);
  pinMode(pttrele, OUTPUT);

  delay(500);

  set_band();

  band = 40;
  freq = 7100000;

  mode = 2; //rx +tx

  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  si5351.set_vcxo(PLLB_FREQ, 40);
  si5351.set_correction(XT_CAL_F, SI5351_PLL_INPUT_XO);

  if (amp1 == 2) si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA);
  if (amp1 == 4) si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_4MA);
  if (amp1 == 8) si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);

  if (amp2 == 2) si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_2MA);
  if (amp2 == 4) si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_4MA);
  if (amp2 == 8) si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_8MA);

  si5351.output_enable(SI5351_CLK0, 1);
  si5351.output_enable(SI5351_CLK1, 1);
  si5351.output_enable(SI5351_CLK2, 0);

  tunegen();

  PCICR |= (1 << PCIE2);
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  sei();

  stp = 4;
  setstep();
  layout();
  displayfreq();
  mode = 2;
  EEPROM.get(200, vfo);
}

void loop() {

  //read external ptt
  // if (band >= 10) {
  if (mode == 2) { //this is rxtx mode
    pttv = analogRead(pttext);
    pttv = map(pttv, 0, 1023, 0, 100);
    ptton = false;
    if (pttv < 20) { //no tx on
      digitalWrite(pttrele, LOW); //rele off VRX on
      ptton = false;
    } else {
      digitalWrite(pttrele, HIGH); //rele on VRX off
      ptton = true;
    }
  } else { //this is only rx mode
    digitalWrite(pttrele, LOW);
    ptton = false;
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

  encB = LOW;
  encB = digitalRead(buttonenc);

  if (encB == HIGH) {

    if (!ptton) {
      tempoInizio = millis(); // Salviamo il tempo attuale
      while (digitalRead(buttonenc) == HIGH) { // Aspettiamo che il pulsante venga rilasciato
        //Serial.println("wait..");
      }
      unsigned long tempoTrascorso = millis() - tempoInizio;
      if (tempoTrascorso <= tempo1) {

        if (!swtc) {
          freqbuff = freq;
          //freq = vfo;
          if (band == 40) EEPROM.get(200, freq);
          if (band == 20) EEPROM.get(220, freq);
          if (band == 10) EEPROM.get(240, freq);
          if (band == 11) EEPROM.get(260, freq);
          if (band == 15) EEPROM.get(280, freq);
          swtc = true;
          Serial.println("bfo");
          Serial.println(band);
          Serial.println(freq);
        } else {

          vfo = freq;
          if (band == 40) {
            EEPROM.put(200, vfo);
          }
          if (band == 20) {
            EEPROM.put(220, vfo);
          }
          if (band == 15) {
            EEPROM.put(240, vfo);
          }
          if (band == 11) {
            EEPROM.put(260, vfo);
          }
          if (band == 10) {
            EEPROM.put(280, vfo);
          }
          freq = freqbuff;
          swtc = false;

        }

      } else if (tempoTrascorso <= tempo2) {
        if (amp1 == 2) amp1 = 4;
        else if (amp1 == 4) amp1 = 8;
        else if (amp1 == 8) amp1 = 2;
        if (amp1 == -1) amp1 = 4;
        EEPROM.put(30, amp1);
      } else if (tempoTrascorso <= tempo3) {
        if (amp2 == 2) amp2 = 4;
        else if (amp2 == 4) amp2 = 8;
        else if (amp2 == 8) amp2 = 2;
        if (amp2 == -1) amp2 = 4;
        EEPROM.put(60, amp2);
        Serial.println("amp2");
        Serial.println(amp2);
      } else {
        /*
        niente da fare qui
    */
      }

      layout();
      displayfreq();
      tunegen();
    }
  }

  currB = digitalRead(buttonb);

  if (currB == HIGH & !swtc) {

    delay(600);

    if (band == 80) {
      band = 40;
    } else {
      if (band == 40) {
        band = 20;
      } else {
        if (band == 20) {
          band = 15;
        } else {
          if (band == 15) {
            band = 11;
          } else {
            if (band == 11) {
              band = 10;
            } else {
              if (band == 10) {
                band = 40;
              }
            }
          }
        }
      }
    }

    set_freq();
    layout();
    delay(300);
  }

  if (freqold != freq) {
    time_now = millis();
    tunegen();
    freqold = freq;
  }

  if (vfo != vfop) {
    time_now = millis();
    tunegen();
    vfop = vfo;
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

  if (amp1 == 2) si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA);
  if (amp1 == 4) si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_4MA);
  if (amp1 == 8) si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);

  if (amp2 == 2) si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_2MA);
  if (amp2 == 4) si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_4MA);
  if (amp2 == 8) si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_8MA);

  //verifico se ho un vfo parametrizzato
  if (band == 40) EEPROM.get(200, vfo);
  if (band == 20) EEPROM.get(220, vfo);
  if (band == 10) EEPROM.get(240, vfo);
  if (band == 11) EEPROM.get(260, vfo);
  if (band == 15) EEPROM.get(280, vfo);

  vfoparam = correctBfo(freq, index);
  isvfoparam = false;
  if (vfoparam != 0) {
    vfo = vfoparam;
    isvfoparam = true;
  }
  if (ptton) vfo = vfotx;

  if (band == 80 or band == 40) si5351.set_freq((freq + correctFreq(freq, index) + vfo + corr1) * 100, SI5351_CLK0);
  else si5351.set_freq((((freq + correctFreq(freq, index)) - vfo) + corr1) * 100, SI5351_CLK0);
  si5351.set_freq((vfo + corr2) * 100, SI5351_CLK1); //gab
  si5351.update_status();

  //Serial.println(freq + correctFreq(freq, index));

}

void displayfreq() {

  lcd.clear();
  fN = String(freq);
  lcd.setCursor(15 - fN.length(), 0);
  lcd.print(freq);
  lcd.setCursor(p_, 1);
  lcd.print("^");
}

void setstep() {
  switch (stp) {
  case 1:
    stp = 2;
    fstep = 1;
    p_ = 14;
    break;
  case 2:
    stp = 3;
    fstep = 10;
    p_ = 13;
    break;
  case 3:
    stp = 4;
    fstep = 100;
    p_ = 12;
    break;
  case 4:
    stp = 5;
    fstep = 1000;
    p_ = 11;
    break;
  case 5:
    stp = 6;
    fstep = 10000;
    p_ = 10;
    break;
  case 6:
    stp = 1;
    fstep = 100000;
    p_ = 9;
    break;
  }
}

void layout() {

  lcd.setCursor(0, 0);
  lcd.print("RX");
  if (ptton) {
    lcd.setCursor(0, 0);
    lcd.print("TX");
  } else {

    if (swtc) {
      lcd.setCursor(0, 0);
      lcd.print("RXIF=");
      lcd.setCursor(5, 0);
      lcd.print(band);
    }
  }

  lcd.setCursor(0, 3);
  lcd.print("VFO=");
  lcd.setCursor(4, 3);
  lcd.print(amp1);
  lcd.setCursor(6, 3);
  lcd.print("BFO=");
  lcd.setCursor(10, 3);
  lcd.print(amp2);
  lcd.setCursor(16, 3);
  lcd.print(version);
  lcd.setCursor(8, 2);
  lcd.setCursor(14, 2);
  lcd.print("IU2PAQ");
  lcd.setCursor(0, 2);
  lcd.print("             ");
  if (!ptton) {
    if (isvfoparam) {
      lcd.setCursor(0, 2);
      lcd.print("*");
      lcd.setCursor(1, 2);
      lcd.print(vfo);
    } else {
      lcd.setCursor(0, 2);
      lcd.print(vfo);
    }
  }
  if (ptton) {
    lcd.setCursor(0, 2);
    lcd.print(vfotx);
  }

  if (correctFreq(freq, index) != 0) {
    lcd.setCursor(9, 2);
    lcd.print("C=");
    lcd.setCursor(10, 2);
    lcd.print(correctFreq(freq, index));
  }

  if (accordo) {
    lcd.setCursor(12, 2);
    lcd.print("]");
    lcd.setCursor(12, 3);
    lcd.print("[");
  } else {
    lcd.setCursor(12, 2);
    lcd.print(" ");
    lcd.setCursor(12, 3);
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

void set_band() {
  if (freq >= 3500000 and freq <= 3580000) band = 80;
  if (freq >= 7000000 and freq <= 7200000) band = 40;
  if (freq >= 14000000 and freq <= 14350000) band = 20;
  if (freq >= 21000000 and freq <= 21450000) band = 15;
  if (freq >= 26500000 and freq <= 27500000) band = 11;
  if (freq >= 28000000 and freq <= 28700000) band = 10;
}

void check_freq(int dir) {

  unsigned long fmi_ = 0;
  unsigned long fma_ = 0;

  if (band == 80) {
    fmi_ = 3500000;
    fma_ = 3580000;
  }
  if (band == 40) {
    fmi_ = 7000000;
    fma_ = 7350000;
  }
  if (band == 20) {
    fmi_ = 14000000;
    fma_ = 14380000;
  }
  if (band == 15) {
    fmi_ = 21000000;
    fma_ = 21650000;
  }
  if (band == 11) {
    fmi_ = 26500000;
    fma_ = 27500000;
  }
  if (band == 10) {
    fmi_ = 28000000;
    fma_ = 28800000;
  }

  if (dir == 1 & !swtc)
    if (freq > fma_) freq = freq - fstep;

  if (dir == -1 & !swtc)
    if (freq < fmi_) freq = freq + fstep;
}

int correctFreq(uint32_t freq, int index) {
  for (int i = 0; i < index; i++) {
    if (freqA[i] == freq) return freqAV[i];
  }
  return 0;
}

uint32_t correctBfo(uint32_t freq, int index) {
  for (int i = 0; i < index; i++) {
    if (freqA[i] == freq) return bfoA[i];
  }
  return 0;
}
//void setRotation(uint8_t rotation);