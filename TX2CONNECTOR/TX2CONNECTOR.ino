#include <SPI.h>

#include <Ethernet.h>

// Replace with your network settings
byte mac[] = {
  0xDE,
  0xAD,
  0xAA,
  0xEF,
  0xFE,
  0xED
};
IPAddress ip(192, 168, 100, 82);
IPAddress server(192, 168, 100, 66);
IPAddress gateway(192,168,100,1);
IPAddress subnet(255,255,255,0);


unsigned int serverPort = 7356; 


EthernetClient client;

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(9600);

  // Initialize Ethernet interface
  Ethernet.begin(mac, ip);
}

void loop() {

if (client.connect(server, serverPort)) {

}

delay(2000);
Serial.println(frequenzaSDR());

/*
  // Connect to the Telnet server
  if (Serial.available() > 0) { // Check if there's incoming data
    char incomingByte = Serial.read(); // Read the incoming byte

String msg="";

client.setTimeout(10000);
    
    if (incomingByte == 'f') {  //get current frequency
      if (client.connect(server, serverPort)) {
       client.println("f");
       delay(100);
       client.println("f");
       delay(100);
       client.println("f");
       delay(100);
       client.println("f"); 
      delay(600);
        // Read and print the server's response
        while (client.available()) {
          char c = client.read();
          msg +=c;      
        }
       client.stop();
        Serial.print(msg); 
        Serial.println("Disconnected from server");
      } else {
        Serial.println("Connection failed");
      }
    } 
  }
  */
}


uint32_t  frequenzaSDR() {

String msg="";
long num=0;



       client.println("f");
       delay(200);
        while (client.available()) {
          char c = client.read();
          msg +=c;     
        }

        if (msg !="") return msg.toInt();       
       client.println("f");
       delay(200);
        while (client.available()) {
          char c = client.read();
          msg +=c;     
        }
        if (msg !="") return msg.toInt();
      client.println("f");
       delay(200);
       client.println("f"); 
       delay(200);
       client.println("f"); 
       delay(200);
       client.println("f"); 
       delay(200);
       client.println("f"); 
       delay(200);
       client.println("f"); 
       delay(200);
       client.println("f"); 
       delay(200);

        while (client.available()) {
          char c = client.read();
          msg +=c;     
        }

        if (msg !="") return msg.toInt();
        //Serial.print(msg); 
        //Serial.println("Disconnected from server");
 
      return num;     
}

uint32_t  allineFrequenza() {

  long freq=0;

 for (int i = 0; i < 10; i++) { 
   
   freq=frequenzaSDR();

   if( freq !=0) break;
  }

  return freq;

}

