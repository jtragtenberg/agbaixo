/* @file CustomKeypad.pde
  || @version 1.0
  || @author Alexander Brevig
  || @contact alexanderbrevig@gmail.com
  ||
  || @description
  || | Demonstrates changing the keypad size and key values.
  || #
*/
#include <Keypad.h>
#include "MIDIUSB.h"

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns

const int MIDI_CHANNEL = 10;

byte colPins[ROWS] = {2, 3, 4, 5}; //connect to the row pinouts of the keypad
byte rowPins[COLS] = {9, 8, 7, 6}; //connect to the column pinouts of the keypad
byte pinoSensor = A3;

char hexaKeys[ROWS][COLS] = {
  {'0', '4', '8', 'C'},
  {'1', '5', '9', 'D'},
  {'2', '6', 'A', 'E'},
  {'3', '7', 'B', 'F'}
};

//initialize an instance of class NewKeypad
Keypad teclado = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

String msg;

int key_order[ROWS * COLS] = {
  1, 5, 9, 13,
  2, 6, 10,  14,
  3, 7, 11, 15,
  4, 8, 12, 16
};

byte notaMidi[ROWS * COLS] = {
  60, 64, 68, 72,
  61, 65, 69, 73,
  62, 66, 70, 74,
  63, 67, 71, 75
};

byte ccMidiSensor = 11;

int leituraAnalogicaAtual;
int ultimaLeituraAnalogica;
float leituraAnalogicaFiltrada;

void setup() {
  Serial.begin(9600);
}

void loop() {
  //  controlChange(MIDI_CHANNEL - 1, ccMidiSensor, analogRead(A3)/8);
  leituraAnalogicaAtual = analogRead(pinoSensor);
  leituraAnalogicaFiltrada = filtrar(leituraAnalogicaAtual, leituraAnalogicaFiltrada, 0.9);
  int leituraAnalogicaMapeada = map(leituraAnalogicaFiltrada, 30, 1010, 0, 127);
  if (leituraAnalogicaMapeada != ultimaLeituraAnalogica) {
    //      MIDI.sendControlChange(ccMIDI[i], leituraAnalogicaMapeada, MidiChan);
    controlChange(MIDI_CHANNEL - 1, ccMidiSensor, leituraAnalogicaMapeada);
    MidiUSB.flush();
  }
  ultimaLeituraAnalogica = leituraAnalogicaMapeada;

  if (teclado.getKeys()) {
    for (int i = 0; i < LIST_MAX; i++) {
      if (teclado.key[i].stateChanged) {
        switch (teclado.key[i].kstate) {  // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
          case PRESSED:
            msg = " APERTADA.";
            //            MIDI.sendNoteOn(notas_abrindo[key_order[teclado.key[i].kcode]] - 12, 60, MIDI_CHANNEL);
            noteOn(MIDI_CHANNEL - 1, notaMidi[teclado.key[i].kcode], 64);   // Channel 0, middle C, normal velocity
            MidiUSB.flush();
            break;
          case HOLD:
            msg = " SEGURADA.";
            break;
          case RELEASED:
            msg = " SOLTA.";
            //            MIDI.sendNoteOff(notas_abrindo[key_order[teclado.key[i].kcode]] - 12, 0, MIDI_CHANNEL);
            noteOff(MIDI_CHANNEL - 1, notaMidi[teclado.key[i].kcode], 0);  // Channel 0, middle C, normal velocity
            MidiUSB.flush();
            break;
          case IDLE:
            msg = " SOLTISSIMA.";
        }
        Serial.print("Tecla ");
        Serial.print(key_order[teclado.key[i].kcode]);
        Serial.println(msg);
      }
    }
  }
}

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

int filtrar(int valEntrada, int valFiltrado, float qtdeDeFiltragem) {
  return ((1 - qtdeDeFiltragem) * valEntrada) + (qtdeDeFiltragem * valFiltrado);
}
