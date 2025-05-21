#include <MIDIUSB.h>

// SETUP
int padpin = 21;  // Analog pin
unsigned long blinkTimeMicros = 500;  // Blink time in microseconds (e.g., 500000 us = 0.5 seconds)

void setup() {
  // Set the pad pin as an input
  pinMode(padpin, INPUT);
  // Initialize USB Serial for debugging (optional)
  Serial.begin(9600);
  // Disable global interrupts while setting up the timer
  cli();
  // Convert microseconds to seconds
  double blinkTimeSeconds = blinkTimeMicros / 1000000.0;
  // Calculate the timer compare match register value based on blink time
  // F_CPU      = 16 MHz (16,000,000 Hz)
  // Prescaler  = 1024
  // Formula: OCR1A = (F_CPU / Prescaler) * (Blink Time in seconds) - 1
  unsigned long compareMatchValue = (16e6 / 1024) * blinkTimeSeconds - 1;
  // Configure Timer1
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1B |= (1 << WGM12);              // CTC mode
  TCCR1B |= (1 << CS10) | (1 << CS12); // Prescaler 1024
  OCR1A = compareMatchValue;           // Set compare match register value

  // Enable Timer Compare Interrupt
  TIMSK1 |= (1 << OCIE1A);

  // Enable global interrupts
  sei();
}

// INTERRUPTION ROUTINE
int padRead = 0;
int note = 38;
unsigned long aux[2];
uint8_t lsbpad;
uint8_t msbpad;
uint8_t lsbtime;
uint8_t msbtime;
unsigned int ftime;

ISR(TIMER1_COMPA_vect) {
  padRead = analogRead(padpin);  // Reads the input pin
  aux[1] = aux[0];
  aux[0] = micros(); // Reads the time

  lsbpad = padRead & 0x7F; // Extract the least significant 7 bits
  msbpad = (padRead >> 7) & 0x7F; // Extract the most significant 7 bits
  controlChange(0, 43, lsbpad);
  controlChange(0, 11, msbpad);
  
  ftime = aux[0] - aux[1];
  lsbtime = ftime & 0x7F; // Extract the least significant 7 bits
  msbtime = (ftime >> 7) & 0x7F; // Extract the most significant 7 bits
  controlChange(0, 44, lsbtime);
  controlChange(0, 12, msbtime);
  
  MidiUSB.flush();
}

// LOOP ROUTINE (PRINTS)
void loop() {
  // Print the results
  Serial.print("Original value: ");
  Serial.print(padRead);
  Serial.print(" -> LSB 7 bits: ");
  Serial.print(lsbpad);
  Serial.print(" (binary: ");
  Serial.print(lsbpad, BIN);
  Serial.print(")");
  Serial.print(" -> MSB 7 bits: ");
  Serial.print(msbpad);
  Serial.print(" (binary: ");
  Serial.print(msbpad, BIN);
  Serial.println(")");
  
  Serial.print("Original value: ");
  Serial.print(ftime);
  Serial.print(" -> LSB 7 bits: ");
  Serial.print(lsbtime);
  Serial.print(" (binary: ");
  Serial.print(lsbtime, BIN);
  Serial.print(")");
  Serial.print(" -> MSB 7 bits: ");
  Serial.print(msbtime);
  Serial.print(" (binary: ");
  Serial.print(msbtime, BIN);
  Serial.println(")");

  delay(500);
}

// MIDI MESSAGES
// Control Change
void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}
