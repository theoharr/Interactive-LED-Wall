/*
 * Inspiration and code base borrowed from here
 * https://www.instructables.com/Interactive-LED-Tile-Wall-Easier-Than-It-Looks/
 * 
 * I wanted to add wifi/alexa support to the project so I used a SongHe 2560 w/ wifi
 * https://www.amazon.com/gp/product/B07THDDFSJ/ref=ppx_yo_dt_b_asin_title_o06_s01?ie=UTF8&psc=1
 * 
 * and a screen
 * https://www.amazon.com/gp/product/B075FP83V5/ref=ppx_yo_dt_b_asin_title_o06_s00?ie=UTF8&psc=1
 * 
 * 
 * Programming the 2560 is tricky. I followed this
 * 
 * https://robotdyn.com/mega-wifi-r3-atmega2560-esp8266-flash-32mb-usb-ttl-ch340g-micro-usb.html
 * 
 * USB/CH340 settings
 *                      1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
 * ESP upload sketch  | 0 | 0 | 0 | 0 | 1 | 1 | 1 |   | 
 * ESP serial connect | 0 | 0 | 0 | 0 | 1 | 1 | 0 |   |
 * 2560 upload sketch | 0 | 0 | 1 | 1 | 0 | 0 | 0 |   |
 * 2560+ESP com3      | 1 | 1 | 1 | 1 | 0 | 0 | 0 |   |
 * 2560+ESP           | 1 | 1 | 0 | 0 | 0 | 0 | 0 |   |
 * independent        | 0 | 0 | 0 | 0 | 0 | 0 | 0 |   |
 * 
 * The two module communicate thru com3/Serial3.
 * Serial Basics - https://forum.arduino.cc/index.php?topic=396450.0
 */


#include <Keypad.h>
#include <FastLED.h>
#include <arduinoFFT.h>
#include <LCDWIKI_GUI.h> //Core graphics library
#include <LCDWIKI_KBV.h> //Hardware-specific library
#include <arduino-timer.h>  //https://github.com/contrem/arduino-timer

// LED stuff
#define SAMPLES 8            // Must be a power of 2
#define LED_PIN     8        // Data pin to LEDS
#define NUM_LEDS    384      // 8 rows x 2 strips x 24 leds per strip or 6 LEDs x 64 panels
#define BRIGHTNESS  255      // from 0 to 255
#define LED_TYPE    NEOPIXEL //aka 2812b 
#define COLOR_ORDER GRB
#define NUM_MODES 4
#define ROWS 8
#define COLS 8

double vReal[SAMPLES];
double vImag[SAMPLES] = { };
int Intensity[COLS] = { }; // initialize Frequency Intensity to zero
arduinoFFT FFT = arduinoFFT();  // Create FFT object

int values[ROWS][COLS]; // 2D array to keep track of the current color of each tile
int mem_values[ROWS][COLS]; // 2D array to keep track Memory Colors
int brightness[ROWS][COLS]; // 2D array to keep track of the current brightness of each tile
int MIC_IN = 0;         // Use A0 for mic input

// LCD stuff
LCDWIKI_KBV my_lcd(ILI9486,A3,A2,A1,A0,A4); //model,cs,cd,wr,rd,reset
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

auto timer = timer_create_default();


//-----------------------------Button Set Up-----------------------------------------
const byte rows = 8;
const byte cols = 8;
char keys[rows][cols] = {
  {1,2,3,4,5,6,7,8},
  {11,12,13,14,15,16,17,18},
  {21,22,23,24,25,26,27,28},
  {31,32,33,34,35,36,37,38},
  {41,42,43,44,45,46,47,48},
  {51,52,53,54,55,56,57,58},
  {61,62,63,64,65,66,67,68},
  {71,72,73,74,75,76,77,78}
};
byte rowPins[rows] = {10,9,2,3,4,5,6,7}; //connect to the row pinouts of the keypad
byte colPins[cols] = {38,40,42,44,46,48,50,52}; //connect to the column pinouts of the keypad
Keypad buttons = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );
//-----------------------------------------------------------------------------------

CRGB leds[NUM_LEDS];            // Create LED Object

int mode = 1;                   // Start on the first mode

CRGBPalette16 currentPalette;   // Used for christmas mode

//-----------------------------------------------------------------------------------
#define SERIAL_MAX_LEN 128
#define SERIAL_START '<'
#define SERIAL_END '>'
typedef struct SerialXfer {
  boolean recvInProgress;
  boolean completeData;
  byte index;
  char chars[SERIAL_MAX_LEN];
  //char tempChars[SERIAL_MAX_LEN];
} SerialXfer;
SerialXfer serialXfer = {0};


//-----------------------------------------------------------------------------------


bool helloworld(void *)
{
  Serial.println("Hello world from 2560.");
  return true; // keep timer active
}

void showParsedData(char* str) {
    Serial.print("EspMessage: ");
    Serial.println(str);   
}

void parseSerialData() {
  char * strtokIndx;
  char tempChars[SERIAL_MAX_LEN];
  unsigned long brightness;
  unsigned long rgb;
  
  // make a working copy
  memcpy(tempChars, serialXfer.chars, SERIAL_MAX_LEN);
  strtokIndx = strtok(tempChars,":"); 
  if (strncmp(strtokIndx, "log", 3) == 0) {
    // Just show the msg
    showParsedData(strtok(NULL, ":"));
  } else if (strncmp(strtokIndx, "mode", 4) == 0) {
    char buf[64] = {0};
    strtokIndx = strtok(NULL, ":");
    brightness = strtoul(strtokIndx, NULL, 16);
    strtokIndx = strtok(NULL, ":");
    rgb = strtoul(strtokIndx, NULL, 16);
    sprintf(buf, "mode %ld/%lx %ld/%lx", brightness, brightness, rgb, rgb);
    showParsedData(buf);
  } else {
    Serial.println("Unknown message type from ESP");
  }
}


void readEspMessages() {
  char serChar;

  while ( (Serial3.available() > 0) && 
          (serialXfer.completeData == false) ) {
      serChar = Serial3.read();
      //Serial.println(serChar);   
      if (serialXfer.recvInProgress == true) {
          if (serChar != SERIAL_END) {
              serialXfer.chars[serialXfer.index] = serChar;
              serialXfer.index++;
              if (serialXfer.index >= SERIAL_MAX_LEN) {
                  serialXfer.index = SERIAL_MAX_LEN - 1;
              }
          }
          else {
              serialXfer.chars[serialXfer.index] = '\0'; // terminate the string
              serialXfer.recvInProgress = false;
              serialXfer.index = 0;
              serialXfer.completeData = true;
          }
      } else if (serChar == SERIAL_START) {
          serialXfer.recvInProgress = true;
      }
  }
  if (serialXfer.completeData == true) {
    //showParsedData(serialXfer.chars);
    parseSerialData();
    serialXfer.completeData = false;
  }
}
 

void lcd_setup() {
  my_lcd.Init_LCD();
  my_lcd.Fill_Screen(0x0);  
  my_lcd.Set_Rotation(1);
  //void show_string(uint8_t *str,int16_t x,int16_t y,uint8_t csize,uint16_t fc, uint16_t bc,boolean mode)
  // string, x pos, y pos, text size, fg color, bg color, mode
  int height = my_lcd.Get_Height();
  int width = my_lcd.Get_Width();
  char str[50];
  sprintf("Display height is %d, width is %d", height, width);
  show_string("* OMG iz it working!? *",CENTER,3,1,0x07E0, 0,1);
  //show_string(str,CENTER,my_lcd.Get_Display_Height()-11,1,0xFFFF, 0,1);
  //show_string("---> T+L 4eva <---",CENTER,my_lcd.Get_Display_Height()/2 - 11,8,RED, 0,1);
}

void setup(){
  Serial.begin(115200);  // usb
  Serial3.begin(9600);   // internal to esp

  delay(1000); // power-up safety delay

  Serial.println("Starting setup...");

  //pinMode(MIC_IN, INPUT);
  //FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  //FastLED.setBrightness(  BRIGHTNESS );
  
  //buttons.setHoldTime(1000); // require a 1 second hold to change modes
  //clear_display(); //Make sure the display is blank
  timer.every(5000, helloworld);
  lcd_setup();
  Serial.println("Finished setup.");
}

void loop(){
  /* --------Light tiles one at a time (testing)-----------
  for(int i = 0; i < ROWS; i++){
    for(int j = 0; j < COLS; j++){
      light_tile(i, j, 0, 255);
      FastLED.show();
      delay(10);
      light_tile(i, j, 256, 0);
    }
  }
  -------------------------------------------------------*/

/*  //---------Light tile when pressed (testing)------------
  int location = buttons.getKey();
  if (location){
    light_tile(location/10, (location%10)-1, 0, 255);
    FastLED.show();
    delay(100);
  }
  // else 
    //clear_display();
 // -------------------------------------------------------*/

 //--------------- Actual Code ---------------------------
  readEspMessages();
  //print_menu();
  /*
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(5000);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);       
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(5000);                       // wait for a second
  */
  
  /* 
  int location = buttons.getKey();
  mode = buttons.waitForKey();
  switch(mode){
    case 1:
      Serial.println("1");
      rainbowColors();
      break;
    case 2:
      Serial.println("2");
      paint();
      break;
    case 3:
      Serial.println("3");
      Memory();
      break;
    case 4:
      Serial.println("4");
      Visualizer();
      break;
  }
  */
  timer.tick();
}

void light_tile(int row, int col, int color, int bright){
  if (color == 256)
    bright = 0;
  for(int i=23; i >= 21; i--){
    leds[i - 3*row + 48*col]= CHSV( color, 255, bright);
  }
  for(int i=24; i <= 26; i++){
    leds[i + 3*row + 48*col]= CHSV( color, 255, bright);
  }
  values[row][col] = color;
  brightness[row][col] = bright;
}

void clear_display(){
  for(int i = 0; i < ROWS; i++){
    for(int j = 0; j < COLS; j++){
      light_tile(i, j, 256, 0);
      values[i][j] = 256;
    }
  }
  FastLED.show();
}

void print_menu(){
  clear_display();
  int color = 0;
  for(int i = 1; i <= NUM_MODES; i++){
    light_tile(0, i-1, color, 255);
    color += 32;
  }
  FastLED.show();
}

void rainbowColors(){
  clear_display(); 
  int bow = 0;
  int brt = 255;
  for(int i = 0; i < ROWS; i++){
    for(int j = 0; j < COLS; j++){
      light_tile(i, j, bow, brt);
      bow += 32;
      if(bow > 255)
         bow = 0;
    }
    bow += 32;
    if(bow > 255)
      bow = 0;
  }
  FastLED.show();
  bool tap = 1;
  while(tap){
    int location = buttons.getKey();
    if (location)
      brt -= 50;
    if (brt < 100)
      brt = 255;
    for(int i = 0; i < ROWS; i++){
      for(int j = 0; j < COLS; j++){
        int clr = values[i][j] + 1;
        if (clr > 255)
          clr = 0;
        light_tile(i, j, clr, brt);
        if (buttons.getState() == HOLD)
          tap = 0;
      }
    }
    FastLED.show();
  }
}

void paint(){
  clear_display();
  bool tap = 1;
  int color = 0;
  while(tap){                                               //Loop until a button is held
    int location = buttons.getKey();
    if (location){                                          //if a button is pressed
      if(values[location/10][(location%10)-1] != 256){
        color = values[location/10][(location%10)-1] + 32;  //Update Color if tile is already colored
        if (color > 256)
          color = 0;
      }  
      else if (color == 256)                                //Loop back to red at end of rainbow
        color = 0;
      light_tile(location/10, (location%10)-1, color, 255); //Light tile color
      values[location/10][(location%10)-1] = color;
      FastLED.show();                                       //Update display
      delay(100);                                           //wait 1/10th of a second
    }
    if(buttons.getState() == HOLD)                          //Exit function if tile held
      tap = 0;
  }
}

void Visualizer(){
  bool tap = 1;
  clear_display();
  delay(100);
  while(tap){
    int location = buttons.getKey();
    Serial.println("Loop begin");
    //Collect Samples
    for(int i = 0; i < SAMPLES; i++){
      int aud = analogRead(MIC_IN);
      Serial.println(aud);
      vReal[i] = aud;
      delay(10);
    }
    Serial.println("Loop over");
    //FFT
    FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
    FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);
    Serial.println("FFT Over");
    //Update Intensity Array
    for(int i = 0; i < COLS; i++){
      vReal[i] = constrain(vReal[i],0 ,2000);            // set max value for input data
      vReal[i] = map(vReal[i], 0, 2000, 0, ROWS);        // map data to fit our display
      Serial.print("vReal[i] = ");
      Serial.println(vReal[i]);
      Intensity[i] --;                                  // Decrease displayed value
      if (vReal[i] > Intensity[i])                     // Match displayed value to measured value
        Intensity[i] = vReal[i];
      Serial.print("Intensity[i] = ");
      Serial.println(Intensity[i]);
    }
      update_display();
      if(buttons.getState() == HOLD){
        tap = 0;
      }
    delay(20);
  }
  Serial.println("Tap");
}

void update_display(){
  int color = 0;
  for (int i = 0; i < COLS; i++){
    for (int j = 0; j < ROWS; j++){
      if(Intensity[i] >= 8 - j )
        light_tile(j, i, color, 255);
      else
        light_tile(j, i, 256, 255);
    }
    color += 32;
    if (color == 256)
      color = 0;
  }
  FastLED.show();
}

void Memory(){
  clear_display();                    //Clear display
  for(int i = 2; i < 6; i++){         //Print Verticle lines of box
    light_tile(i, 1, 192, 255);
    light_tile(i, 6, 192, 255);
  }
  for(int i = 1; i < 7; i++){         //Print Horizontal lines of box
    light_tile(1, i, 192, 255);       
    light_tile(6, i, 192, 255);       
  }
  FastLED.show();
  Set_Colors();                       //Set Random Color Locations
  bool tap = 1;
  int color;
  int tile1;
  int tile2;
  int end = 0;
  while(tap){                         //Loop until BUTTON is held
    int location1 = buttons.waitForKey();
    if(buttons.getState() == HOLD)    //Exit if button is held
      tap = 0;
    if (location1){                    //When a button is pushed
      color = mem_values[location1/10][(location1%10)-1];         //Update the color of the clicked tile
      light_tile(location1/10, (location1%10)-1, color, 255);
      tile1 = color;
      FastLED.show();
    }
    int location2 = buttons.waitForKey();
    if(buttons.getState() == HOLD)   //Exit if held
      tap = 0;
    if (location2){                  //When a button is pushed
      color = mem_values[location2/10][(location2%10)-1];         //Update the color of the clicked tile
      light_tile(location2/10, (location2%10)-1, color, 255);
      tile2 = color;
      FastLED.show();
    }
    if(tile1 != tile2 || location1 == location2){              //If tiles are not the same color
        light_tile(location1/10, (location1%10)-1, 256, 255);   //Light the tiles black
        light_tile(location2/10, (location2%10)-1, 256, 255);
    }
    else
      end++;
    delay(1000);
    FastLED.show();
    if(end == 8)
      tap = 0;
  }
}

void Set_Colors(){  //Makes a 4x4 grid of colored pairs in random locations for memory game
  int color = 0;
  for(int i = 0; i < ROWS; i++)
    for(int j = 0; j < COLS; j++)
      mem_values[i][j] = 256;
  for(int j = 0; j < 8; j++){         //Place 8 colors
    for(int i = 0; i< 2; i++){        //place each color twice
      bool tile_not_empty = 1;
      while(tile_not_empty){          //don't place color over another color
        int row = (rand()%4)+2;       //Get random tile
        int col = (rand()%4)+2;
        if(mem_values[row][col] == 256){  //Check if tile is empty
          mem_values[row][col] = color;   //Place color
          tile_not_empty = 0;         //Exit loop
        }
      }
    }
    color += 32;                      //Increment Color
  }
}

void tic(){
  clear_display();
  for(int i = 2; i < ROWS; i+=3){
    for(int j = 0; j < COLS; j++){
      light_tile(i, j, 192, 255);
    }
  }
  for(int i = 0; i < ROWS; i++){
    for(int j = 2; j < COLS; j+=3){
      light_tile(i, j, 192, 255);
    }
  }
  FastLED.show();
  bool tap = 1;
  while(tap){
    int location = buttons.getKey();
    if (location){
       int color = values[location/10][(location%10)-1] + 32;
       if (color > 256)
          color = 0;
      light_tile(location/10, (location%10)-1, color, 255);
      FastLED.show();
      delay(100);
    }
    if(buttons.getState() == HOLD)
      tap = 0;
  }
}

// LCD specific stuff
void show_string(uint8_t *str,int16_t x,int16_t y,uint8_t csize,uint16_t fc, uint16_t bc,boolean mode)
{
    my_lcd.Set_Text_Mode(mode);
    my_lcd.Set_Text_Size(csize);
    my_lcd.Set_Text_colour(fc);
    my_lcd.Set_Text_Back_colour(bc);
    my_lcd.Print_String(str,x,y);
}
