const int analogPin = A3;

void setup() {
  Serial.begin(9600);
}

void loop() {
  Serial.println(analogRead(analogPin));
  delay(10);
}
