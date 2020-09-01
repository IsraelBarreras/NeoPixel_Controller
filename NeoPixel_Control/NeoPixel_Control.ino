/** INCLUDES ***********************************/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

/** DEFINES ***********************************/

#define UP_BUTTON             A1
#define DOWN_BUTTON           A2
#define RIGHT_BUTTON          A0
#define LEFT_BUTTON           A3
#define OK_BUTTON             2
#define CANCEL_BUTTON         3

#define SCREEN_WIDTH          128
#define SCREEN_HEIGHT         64
#define OLED_RESET            4 

#define NUM_PIXELS_EEPROM_ADD 100
#define GREEN_VAL_EEPROM_ADD  105
#define RED_VAL_EEPROM_ADD    106
#define BLUE_VAL_EEPROM_ADD   107
#define MODE_EEPROM_ADD       108
#define PIN                   11 // On Trinket or Gemma, suggest changing this to 1
#define DELAYVAL              500 // Time (in milliseconds) to pause between pixels
#define KEY_RATE              200 //Time between each keypad read


/*STATIC FUNCTIONS*****************************/
static void page_settings (void);
static void pixels_update (void);
static void shift_pixels (void);


/* static variables****************************/
static byte green = EEPROM.read(GREEN_VAL_EEPROM_ADD);
static byte red = EEPROM.read(RED_VAL_EEPROM_ADD);
static byte blue = EEPROM.read(BLUE_VAL_EEPROM_ADD);
static byte NumPixels = EEPROM.read(NUM_PIXELS_EEPROM_ADD);
static byte mode = EEPROM.read(MODE_EEPROM_ADD);

static byte selValue = 0;
static unsigned long timeUpdate = 0;
static unsigned long keyReadTime = 0;
static boolean blinkStatus = false;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_NeoPixel pixels(NumPixels, PIN, NEO_GRB + NEO_KHZ800);

typedef struct RGB_Struct
{
  byte red = 0;
  byte green = 0;
  byte blue = 0;
  byte colorSel = 0;
  byte pixelIndex = 0;
} RGBstruct;

RGBstruct shiftRGB;


void setup() {
    Serial.begin(9600);
    Wire.begin();
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) 
    {
      Serial.println(F("SSD1306 allocation failed"));
      for(;;); // Don't proceed, loop forever
    }
    
    pinMode(UP_BUTTON, INPUT);
    pinMode(DOWN_BUTTON, INPUT);
    pinMode(RIGHT_BUTTON, INPUT);
    pinMode(LEFT_BUTTON, INPUT);
    pinMode(OK_BUTTON, INPUT);
    pinMode(CANCEL_BUTTON, INPUT);

    display.clearDisplay();
    display.display();
    
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
}

void loop() {

  pixels_update();
  
  page_settings();
}


/***************************************************************************************************************
 * Funcíon: page_settings
 * Descripción: Esta función nos permite navegar entre los diferentes parámetros (red, green, blue, mode, NumPixels) de interes
 * y poder modificarlos por medio de los botones de navegación.
 ***************************************************************************************************************/
static void page_settings(void)
{
  byte numItems = 5;
  char *leyends[numItems] = {"RED", "GREEN", "BLUE", "MODE", "NUMPIXELS"};
  int memAddress[numItems] = {RED_VAL_EEPROM_ADD, GREEN_VAL_EEPROM_ADD, BLUE_VAL_EEPROM_ADD, MODE_EEPROM_ADD, NUM_PIXELS_EEPROM_ADD};
  uint16_t pixelSettings[numItems] = {&red, &green, &blue, &mode, &NumPixels};
  byte *dato = pixelSettings[selValue];

  display.clearDisplay();
  display.setTextSize(2); 
  display.setTextColor(WHITE);        
  display.setCursor(20,0);
  display.print(leyends[selValue]);

  display.setTextSize(4); 
  display.setCursor(10, 25);
  display.print(*dato);
  display.display();

  if(millis() - keyReadTime >= KEY_RATE)
  {
    if(digitalRead(RIGHT_BUTTON))
    {
      selValue = (selValue < numItems - 1) ? selValue + 1 : 0;
    }
    else if(digitalRead(LEFT_BUTTON))
    {
      selValue = (selValue > 0) ? selValue - 1 : 3;
    }
    else if(digitalRead(UP_BUTTON))
    {
      *dato = (*dato < 255) ? *dato + 1 : 0;
    }
    else if(digitalRead(DOWN_BUTTON))
    {
      *dato = (*dato > 0) ? *dato - 1 : 255;
    }
    else if(digitalRead(OK_BUTTON))
    {
      for(byte itemIndex = 0; itemIndex < numItems; itemIndex++)
      {
        dato = pixelSettings[itemIndex];
        EEPROM.write(memAddress[itemIndex], *dato);
      }
    }
    else if(digitalRead(CANCEL_BUTTON))
    {
      red = 0;
      green = 0;
      blue = 0;
    }

    keyReadTime = millis();
  }
}


/***************************************************************************************************************************
 * Funcíon: pixels_update
 * Descripción: Aquí se actualiza el color y encendido de cada led individual dependiendo de los parámetros (red, green, blue)
 * y modo de operación (mode) configurados.
 ***************************************************************************************************************************/
static void pixels_update (void)
{
  unsigned long del = (mode != 4) ? DELAYVAL : 10;
  if(millis() - timeUpdate >= del)
  {
    pixels.clear(); // Set all pixel colors to 'off'
    byte r = random(0,255);
    byte g = random(0,255);
    byte b = random(0,255);
    for(int i=0; i<NumPixels; i++) 
    { 
      switch (mode)
      {
        case 0: //Normal operating mode
          pixels.setPixelColor(i, pixels.Color(red, green, blue));
          break;

        case 1: //BLINK MODE
          if(blinkStatus)
          {
            pixels.setPixelColor(i, pixels.Color(red, green, blue));
          }
          else
          {
            pixels.setPixelColor(i, pixels.Color(0, 0, 0));
          }
          break;
  
        case 2: //RAMDOM MODE
          pixels.setPixelColor(i, pixels.Color(r, g, b));
          break;

        case 3: //RAMDOM BLINK MODE
          if(blinkStatus)
          {
            pixels.setPixelColor(i, pixels.Color(r, g, b));
          }
          else
          {
            pixels.setPixelColor(i, pixels.Color(0, 0, 0));
          }
          break;

        case 4: //Modo de Corrimiento de color tira completa
          shift_pixels();
          pixels.setPixelColor(i, pixels.Color(shiftRGB.red, shiftRGB.green, shiftRGB.blue));
          break;

        case 5: //Modo corrimiento de color led por led
          shift_pixels();
          pixels.setPixelColor(shiftRGB.pixelIndex, pixels.Color(shiftRGB.red, shiftRGB.green, shiftRGB.blue));
          shiftRGB.pixelIndex = (shiftRGB.pixelIndex < NumPixels - 1) ? shiftRGB.pixelIndex + 1 : 0;
          i = NumPixels;
          break;
        
        default:
          mode = 0;
          break;
  
      }
      
      pixels.show();
    
    }
    timeUpdate = millis();
    blinkStatus = !blinkStatus;
  }
  
}


/***************************************************************************************************************
 * Funcíon: shift_pixels
 * Descripción: Está función se utiliza para hacer un corrimiento de color y creear un efecto "arcoiris", es
 * usada en los modos de operación 4 y 5. Lo que pretende es hacer un aumento gradual en el valor de cada color individual,
 * mientras los otros dos colores permanecen constantes en un valor aleatorio.
 ***************************************************************************************************************/
static void shift_pixels(void)
{
  if(shiftRGB.colorSel == 0)
  {
    shiftRGB.red += 1;
    if(shiftRGB.red == 255)
    {
      shiftRGB.colorSel = 1;
      shiftRGB.red = random(0,255);
    }
  }
  else if(shiftRGB.colorSel == 1)
  {
    shiftRGB.green += 1;
    if(shiftRGB.green == 255)
    {
      shiftRGB.colorSel = 2;
      shiftRGB.green = random(0,255);
    }
  }
  else if(shiftRGB.colorSel == 2)
  {
    shiftRGB.blue += 1;
    if(shiftRGB.blue == 255)
    {
      shiftRGB.colorSel = 0;
      shiftRGB.blue = random(0,255);
    }
  }
}
