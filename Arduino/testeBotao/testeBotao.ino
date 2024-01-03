const int botaoPino = 15;
const int ledPin =  A1;

void setup() {
  Serial.begin(9600);

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  digitalWrite(ledPin,!digitalRead(buttonPin));
}
