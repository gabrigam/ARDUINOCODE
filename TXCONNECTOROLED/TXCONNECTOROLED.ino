
/********************************************************************************************************
  TXCONNECTOR
*********************************************************************************************************/
//Libraries
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>    //IDE Standardtunestep
#include <Rotary.h>  //Ben Buxton https://github.com/brianlow/Rotary
#include <si5351.h>  //Etherkit https://github.com/etherkit/Si5351Arduino


#include <EEPROM.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define XT_CAL_F 162500 //11.2024
#define XT_CAL_F 158700 //3.2025

#define buttonenc 4
#define tunestep 5 //Change the pin used by encoder push button if you want.
#define pintone 6  //
#define pttext A3 //pttingresso
#define smeter A2 //pttingresso
#define pintoneout 9 //tono x accordo

//SoftwareSerial mySerialTwo(rxpin, txpin); // RX, TX

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
String versione = "v2.5";


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


 // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.display();
  Serial.begin(9600);
  
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);

  Wire.begin();
  pinMode(2, INPUT);
  pinMode(3, INPUT);

  pinMode(pttext, INPUT);
  pinMode(tunestep, INPUT);
  pinMode(buttonenc, INPUT);
  pinMode(pintone, INPUT);
  pinMode(pintoneout, OUTPUT);

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
 
  freq = 14100000;

  tunegen();

  PCICR |= (1 << PCIE2);
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  sei();

  stp = 4;
  setstep();
  layout();
  displayfreq();
  mode = 2;
 //EEPROM.put(300, 8995550);
 EEPROM.get(300, bfo);
  //bfo = 8995800;
}

void loop() {

  if (Serial.available() > 0) {
    incomingByte = Serial.readStringUntil('\n');
    if (!manual) freq = incomingByte.toInt(); //se sono in automatico aggiorno la frequenza
  }

  pttv = analogRead(pttext);
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

  encB = LOW;
  encB = digitalRead(buttonenc); //setto la modalita' automatica/manuale

  if (encB == HIGH) {

    tempoInizio = millis(); // Salviamo il tempo attuale
    while (digitalRead(buttonenc) == HIGH) { // Aspettiamo che il pulsante venga rilasciato
    }
    unsigned long tempoTrascorso = millis() - tempoInizio;

    if (tempoTrascorso <= 2000) {
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
    } else if (tempoTrascorso > 2000 & tempoTrascorso <= 4000) {
      if (size == 2100) size = 2700;
      else if (size == 2700) size = 3900;
      else if (size == 3900) size = 2100;
      layout();
    } else if (tempoTrascorso > 4000 & tempoTrascorso <= 6000 ) {
      if (!manual) {
        manual = true;
      } else {
        manual = false;
      }
      layout();
    }

  }

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
}

void tunegen() {

  if (freq <= 7210000) si5351.set_freq((freq + bfo + corr1) * 100, SI5351_CLK0);
  else si5351.set_freq((freq - bfo + corr1) * 100, SI5351_CLK0);
  si5351.set_freq((bfo + corr2) * 100, SI5351_CLK1); //gab
  si5351.update_status();
}

void displayfreq() {
  //display.clearDisplay();
   display.setTextSize(2); 
  fN = String(freq);
  if (freq>=10000000)
  display.setCursor(40- fN.length(), 1);//13
  else
  display.setCursor(50- fN.length(), 1);//13
  display.print(freq);
  display.setCursor(p_, 16); //p_1=116, 10=106,100=94,1000=81,10000=69,100000=56
  display.print("^");
  display.display();
}

void setstep() {
  switch (stp) {

  stp=6;
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
    stp = 1;
    fstep = 100000;
    p_ = 56;
    break;
  }
}

void layout() {
  display.clearDisplay();
  displayfreq();
  display.setTextSize(1);
  display.setCursor(0, 0);
  if (ptton)  display.print("TX");
  else  display.print("RX");
  display.setCursor(17, 0);
    if (manual) display.print("MA");
     else display.print("AU");

  display.setCursor(0, 25);
  display.print("           ");
  display.setCursor(0, 25);
  display.print(size);

   if (accordo) {
    display.setCursor(116, 25);
    //display.print("    ");
    display.setCursor(116, 25);
    display.print("[");
  } else {
    display.setCursor(116, 25);
    display.print(" ");
    accordo=true;
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

//void setRotation(uint8_t rotation);