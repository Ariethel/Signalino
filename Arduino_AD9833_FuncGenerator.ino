#include <AD9833.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define CLK 4
#define DT 3
#define SW 2 // Bottone Encoder Rotativo

volatile int currentStateCLK;
volatile int lastStateCLK;
volatile int currentStateDT;
volatile int lastStateDT;
volatile bool rotationDetected = false;

// Definisco le variabili per lo schermo in accordo al Datasheet
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


AD9833 AD(10); // Istanza di AD9833 su pin 10

// Variabili per controllare il bottone che definisce la velocita' di incremento/decremento 
const int buttonPin = 5;
int lastButtonState = LOW;
int buttonState;
int buttonCount = 1;


int i = 0; // Contatore per definire il tipo di onda generata
int increment = 10; // Variabile di incremento frequenza
float Freq = 1000; // 1000Hz
String waveSelected;

void setup() {
  pinMode(CLK, INPUT_PULLUP);
  pinMode(DT, INPUT_PULLUP);
  pinMode(SW, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);

  //Interrupt per lo switch rotativo
  attachInterrupt(digitalPinToInterrupt(CLK), onEncoderChange, CHANGE);
  attachInterrupt(digitalPinToInterrupt(DT), onEncoderChange, CHANGE);

  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // 0x3C Indirizzo del display che utilizzo, potrebbe essere diverso per altri modelli
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.setTextColor(WHITE);
  SPI.begin();

  AD.begin();
  AD.setFrequency(Freq, 0);
  AD.setWave(AD9833_OFF);
}

void loop() {

  // Logica di controllo del bottone integrato nell'encoder rotativo
  int btnState = digitalRead(SW);
  if (btnState == LOW) { // Invia un segnale LOW quando viene premuto
    i++;
    switch (i) {
      case 1:
        AD.setWave(AD9833_SINE);
        waveSelected = "Sine";
        break;
      case 2:
        AD.setWave(AD9833_SQUARE1);
        waveSelected = "Square";
        break;
      case 3:
        AD.setWave(AD9833_TRIANGLE);
        waveSelected = "Triangular";
        break;
      default:
        i = 0;
        AD.setWave(AD9833_OFF);
        waveSelected = "Off";
        break;
    }
    delay(300); // Debouncing
  }


  // Logica di controllo del pulsante per la velocita' di Increment
  buttonState = digitalRead(buttonPin);
  if (buttonState == HIGH && lastButtonState == LOW){
    buttonCount++;
    if (buttonCount > 3) buttonCount = 1;
    delay(50); // Debouncing
  }
  if (buttonCount == 1) increment = 10;
  if (buttonCount == 2) increment = 1000;
  if (buttonCount == 3) increment = 10000;
  lastButtonState = buttonState;



  // Logica di controllo per la rotazione dell'encoder rotativo
  if (rotationDetected) {
    handleEncoderRotation();
    rotationDetected = false;
  }

  // Logica di controllo per il display oled
  oledDisplayRefresh(i, Freq);
}


// Quando viene registrata una rotazione viene chiamato l'interrupt definito in setup. In questo modo posso gestire la rotazione dandole priorita'.
// Necessario per prevenire problemi di sincronizzazione dati dalla presenza di un display oled.
void onEncoderChange() {
  currentStateCLK = digitalRead(CLK);
  currentStateDT = digitalRead(DT);

  if (currentStateCLK != lastStateCLK || currentStateDT != lastStateDT) {
    rotationDetected = true; // Segnalo che sta avvenendo una rotazione
  }

  lastStateCLK = currentStateCLK;
  lastStateDT = currentStateDT;
}

void handleEncoderRotation() {

  if (currentStateCLK == currentStateDT) {
    // Rotazione in senso orario
    if (Freq < 12499999) {
      Freq += increment;
      AD.setFrequency(Freq, 0);
    }
  } else {
    // Rotazione in senso antiorario
    if (Freq > increment) {
      Freq -= increment;
      AD.setFrequency(Freq, 0);
    }
  }
}

void oledDisplayRefresh(int i, int Freq) {
  int16_t x1;
  int16_t y1;
  uint16_t width;
  uint16_t height;
  String displayInfo = " Wave: ";
  displayInfo += waveSelected;
  displayInfo += "\n";
  displayInfo += " Freq:";
  displayInfo += AD.getFrequency();
  displayInfo += "\n";
  displayInfo += " Incr:";
  displayInfo += increment;

  // Definisco i limiti per il testo
  display.getTextBounds(displayInfo, 0, 0, &x1, &y1, &width, &height);

  display.clearDisplay();
  display.setCursor((SCREEN_WIDTH - width) / 2, (SCREEN_HEIGHT - height) / 2);
  display.println(displayInfo);
  display.display();
}
