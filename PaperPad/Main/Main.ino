#include <MIDIUSB.h>
#include <MIDI.h>

// SETUP
const int PadPin = 21;

//MIDI settings
MIDI_CREATE_DEFAULT_INSTANCE();
int note = 38;
int channel = 1;

unsigned long ISRtime = 500;  // interruption time in microseconds (e.g., 500000 us = 0.5 seconds)
void setup() {
  // Set the pads and buttons
  pinMode(PadPin,INPUT);
  // Initialize USB Serial for debugging (optional)
  Serial.begin(31250);
  MIDI.begin(channel);
  // Disable global interrupts while setting up the timer
  cli();
  // Convert microseconds to seconds
  double ISRtime_sec = ISRtime / 1000000.0;
  // Calculate the timer compare match register value based on blink time
  // F_CPU      = 16 MHz (16,000,000 Hz)
  // Prescaler  = 1024
  // Formula: OCR1A = (F_CPU / Prescaler) * (Blink Time in seconds) - 1
  unsigned long compareMatchValue = (16e6 / 1024) * ISRtime_sec - 1;
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
float pressure = 0;
int s = 0;
ISR(TIMER1_COMPA_vect) {
  padRead = analogRead(PadPin);  // reads the input pin [0 : 1023]
  pressure = filter(padRead,pressure,0.6,0.9);
  key(pressure, 15, 5);
  //s = switchMode(map(pressure,0,750,0,127), note);
}



// LOOP ROUTINE (IN CASE OF TESTING)
void loop() {
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//SWITCH MODE
const int switcht = 20; //ms
const int switchon = 25; // pressure threshold to activate the switch
const int switchoff = 15; // pressure threshold to deactivate the switch
int switchbuffer[2];
int switchoutput = 0; //switch velocity [0 -> 127]

int switchMode(int input, int note) {
  unsigned long t0 = 0; //ms
  switchbuffer[1] = switchbuffer[0];
  switchbuffer[0] = input;
  if (switchbuffer[0] >= switchon){
    if (switchoutput == 0) {
      noteOn(channel, 38, 127);
      MidiUSB.flush();
      t0 = millis(); // Start debounce timer
    }
    if (switchbuffer[0] != switchbuffer[1]){
      controlChange(0, 11, map(switchbuffer[0],switchon,127,0,127)); //Control Change 11 -> Expression Controller (mapped to pressure aftertouch)
      MidiUSB.flush();
    }
    switchoutput = 127;
  } 
  else if (switchbuffer[0] <= switchoff && (millis() - t0) >= switcht) {     // Debounce period has passed
    if (switchoutput == 127) {
      controlChange(channel,11,0);
      noteOff(channel, 38, 0);
      MidiUSB.flush();

    }
    switchoutput = 0;
  }
  return switchoutput;
}



// KEY MODE
int index = 0;
int debounce = 0;
const int winsize = 5;
float detection_buffer[winsize];
bool noteon = false;

void key(float pressure, float derthresh, float pthresh) {
  // auxiliar parameters which can be fine tuned
  const int derscale = 3;
  const int debouncesmps = 150;
  float der = 0;

  // allow readings of pressure according to the derivative scale chosen
  index++;
  if(index==derscale){

    //calculate the derivative of the pressure each derscale samples
    der = filter(Derivative(pressure),der,0.1,0.3);

    Serial.print("ref1, ");
    Serial.print(100);
    Serial.print("ref2, ");
    Serial.print(-30);
    Serial.print(", pressure, ");
    Serial.print(map(pressure,0,700,0,100));
    Serial.print(", derivative, ");
    Serial.println(der);

    // Update the deteection buffer
    for (int j = winsize - 1; j > 0; j--) {
      detection_buffer[j] = detection_buffer[j - 1];
    }
    detection_buffer[0] = der;
    //read over the velocity buffer to check if the peak is in the middle and all the samples are greater than the threshold 
    static int middle_index = winsize / 2;
    int threshaux = 0;
    float maxv = 0;
    bool threshcheck = false;
    bool peakcheck = false;
    for (int l = 0; l < winsize; l++){
      // check where is the peak of the buffer
      if (detection_buffer[l] > maxv){
        maxv = detection_buffer[l];
        } 
    }

    //if the peak is in the middle of the buffer and if it is greater than the threshold
    //Check conditions to trigger velocity and send MIDI message
    if (maxv == detection_buffer[middle_index] && maxv >= derthresh && debounce >= debouncesmps && !noteon) {
      //int v = value_buffer[0]-value_buffer[vwinsize-1];
      //noteOn(channel,note[0],map(v,0,400,0,127));
      noteOn(channel,note,maxv);
      MidiUSB.flush();
      debounce = 0;
      noteon = true;
    } 
    // If the pressure decays, turn the note off
    else if (pressure <= pthresh && noteon) {
      noteOff(channel,note,0);
      MidiUSB.flush();
      noteon = false;
    }
    index = 0;
  }
  debounce++;
}



//DERIVATIVE
float derivativeBuffer[5];  
float Derivative(int input) {
  for (int k = 1; k < 5; k++) {
    derivativeBuffer[k] = derivativeBuffer[k - 1];
  }
  derivativeBuffer[0] = input;
  return (float) 2.08 * derivativeBuffer[0] - 4 * derivativeBuffer[1] + 3 * derivativeBuffer[2] - 1.33 * derivativeBuffer[3] + 0.25 * derivativeBuffer[4];
  //Backwards Finite difference (http://en.wikipedia.org/wiki/Finite_difference_coefficients)
}



// EXPONENTIAL AVERAGING AND FORMATTING ADSR
float EWMA(float input, float filteredin, float alpha) {
  return ((1-alpha)*input) + (alpha * filteredin);
  //https://www.itl.nist.gov/div898/handbook/pmc/section4/pmc431.htm
}
float filter_buffer[2];
float filter(float input, float output, float alpha1, float alpha2){ // filters the input signal (exponencial average) dpeending if it is rising or decaying
  filter_buffer[1] = filter_buffer[0];
  filter_buffer[0] = input;
  if (filter_buffer[1] < filter_buffer[0]){ 
    output = (float) EWMA(filter_buffer[0],output,alpha1);
    } 
  else {
    output = (float) EWMA(filter_buffer[0],output,alpha2);
    }
  return output;
}



//MIDI MESSAGES

//CC
void controlChange(byte channel, byte control, byte value) {
    midiEventPacket_t event = {
    static_cast<uint8_t>(0x0B),
    static_cast<uint8_t>(0xB0 | channel),
    static_cast<uint8_t>(control),
    static_cast<uint8_t>(value)
};
  MidiUSB.sendMIDI(event);
}
//  in the case of sending midi using the serial port remeber to Initialize serial communication at MIDI stadard baud rate: 31250 -> Serial.begin(31250);
void sendControlChange(byte channel, byte control, byte value) {
  MIDI.sendControlChange(control, value, channel);
}

//NOTE ON
void noteOn(byte channel, byte pitch, byte velocity) {
    midiEventPacket_t noteOn = {
    static_cast<uint8_t>(0x09),
    static_cast<uint8_t>(0x90 | channel),
    static_cast<uint8_t>(pitch),
    static_cast<uint8_t>(velocity)
};
  MidiUSB.sendMIDI(noteOn);
}
void serialNoteOn(byte channel, byte pitch, byte velocity){
  MIDI.sendNoteOn(pitch, velocity, channel);
}

//NOTE OFF
void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {
    static_cast<uint8_t>(0x08),
    static_cast<uint8_t>(0x80 | channel),
    static_cast<uint8_t>(pitch),
    static_cast<uint8_t>(velocity)
};
  MidiUSB.sendMIDI(noteOff);
}
void serialNoteOff(byte channel, byte pitch, byte velocity){
  MIDI.sendNoteOff(pitch, velocity, channel); 
}