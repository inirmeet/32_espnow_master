 //Libs for espnow and wifi
#include <esp_now.h>
#include <WiFi.h>


#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Channel used in the connection
#define CHANNEL 1
#define JOY_L_X 33
#define JOY_L_Y 34

#define JOY_R_X 35
#define JOY_R_Y 36

uint32_t cnt = 0;

//int JLX =0 ,JLY = 0, JRX = 0, JRY = 0;

//Gpios that we are going to read (analogRead) and send to the Slaves
//It's important that the Slave source code has this same array
//with the same gpios in the same order
uint8_t gpios[] = {JOY_L_X,JOY_L_Y,JOY_R_X,JOY_R_Y};

//In the setup function we'll calculate the gpio count and put in this variable,
//so we don't need to change this variable everytime we change
//the gpios array total size, everything will be calculated automatically
//on setup function
int gpioCount;

//Slaves Mac Addresses that will receive data from the Master
//If you want to send data to all Slaves, use only the broadcast address {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
//If you want to send data to specific Slaves, put their Mac Addresses separeted with comma (use WiFi.macAddress())
//to find out the Mac Address of the ESPs while in STATION MODE)
uint8_t macSlaves[][6] = {
  //To send to specific Slaves
  //{0x24, 0x0A, 0xC4, 0x0E, 0x3F, 0xD1}, {0x24, 0x0A, 0xC4, 0x0E, 0x4E, 0xC3}
  //Or to send to all Slaves
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
};


void setup() {


  Serial.begin(115200);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
  }
  
   display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();

  //Calculation of gpio array size:
  //sizeof(gpios) returns how many bytes "gpios" array points to.
  //Elements in this array are of type uint8_t.
  //sizeof(uint8_t) return how many bytes uint8_t type has.
  //Therefore if we want to know how many gpios there are,
  //we divide the total byte count of the array by how many bytes
  //each element has.
  gpioCount = sizeof(gpios)/sizeof(uint8_t);

  //Puts ESP in STATION MODE
  WiFi.mode(WIFI_STA);

  //Shows on the Serial Monitor the STATION MODE Mac Address of this ESP
  Serial.print("Mac Address in Station: ");
  Serial.println(WiFi.macAddress());

  displayText("Mac Address:" +String(WiFi.macAddress()));

  //Calls the function that will initialize the ESP-NOW protocol
  InitESPNow();

  //Calculation of the size of the slaves array:
  //sizeof(macSlaves) returns how many bytes the macSlaves array points to.
  //Each Slave Mac Address is an array with 6 elements.
  //If each element is sizeof(uint8_t) bytes
  //then the total of slaves is the division of the total amount of bytes
  //by how many elements each MAc Address has
  //by how much bytes each element has.
  int slavesCount = sizeof(macSlaves)/6/sizeof(uint8_t); 

  //For each Slave
  for(int i=0; i<slavesCount; i++){
    //We create a variable that will store the slave information
    esp_now_peer_info_t slave;
    //We inform the channel
    slave.channel = CHANNEL;
    //0 not to use encryption or 1 to use
    slave.encrypt = 0;
    //Copies the array address to the structure
    memcpy(slave.peer_addr, macSlaves[i], sizeof(macSlaves[i]));
    //Add the slave
    esp_now_add_peer(&slave);
  }

  //Registers the callback that will give us feedback about the sent data
  //The function that will be executed is called OnDataSent
  esp_now_register_send_cb(OnDataSent);

  //For each gpio
  //For each GPIO pin in array
//  for(int i=0; i<gpioCount; i++){
//    //We put in read mode
//    pinMode(gpios[i], INPUT);
//  }
}

void InitESPNow() {
  //If the initialization was successful
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
    
  }
  //If there was an error
  else {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
  }
}

//Function that will read the gpios and send
//the read values to the others ESPs
void send(){
  //Array that will store the read values
  uint32_t values[gpioCount];

  //For each gpio
  for(int i=0; i<gpioCount; i++){
    //Reads the value (HIGH or LOW) of the gpio
    //and stores the value on the array
      values[i] = analogRead(gpios[i]);
    // values[i] = random(1, 100);
    Serial.println(String(i)+" : "+values[i]);
  }
  Serial.println("**************************************************");
  //In this example we are going to use the broadcast address {0xFF, 0xFF,0xFF,0xFF,0xFF,0xFF}
  //to send the values to all Slaves.
  //If you want to send to a specific Slave, you have to put its Mac Address on macAddr.
  //If you want to send to more then one specific Slave you will need to create
  //a "for loop" and call esp_now_send for each mac address on the macSlaves array
  uint8_t macAddr[] = {0xFF, 0xFF,0xFF,0xFF,0xFF,0xFF};
  esp_err_t result = esp_now_send(macAddr, (uint8_t*) &values, sizeof(values));
  Serial.print("Send Status: ");
  //If it was successful
  if (result == ESP_OK) {
    Serial.println("Success");
  }
  //if it failed
  else {
    Serial.println("Error");
  }
}

//Callback function that gives us feedback about the sent data
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  //Copies the receiver Mac Address to a string
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  //Prints it on Serial Monitor
  Serial.print("Sent to: ");
  Serial.println(macStr);
  //Prints if it was successful or not
  Serial.print("Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");

}

//We don't do anything on the loop.
//Every time we receive feedback about the last sent data,
//we'll be calling the send function again,
//therefore the data is always being sent

void loop() {
  //Sends again
  send();
  delay(1000);
  cnt++;
  displayText("Sent: " + String(cnt));
 }


 void displayText(String text) {
  display.clearDisplay();

  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(text);
  display.display();
}
