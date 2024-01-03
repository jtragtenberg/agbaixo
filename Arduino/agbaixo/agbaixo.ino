#include <Keypad.h>
#include "MIDIUSB.h"
#include <Bounce2.h>


const byte LINHAS = 4;
const byte COLUNAS = 4;

const int MIDI_CHANNEL = 9;

const byte TEMPO_LEITURA_PIEZO = 10;

const byte pinosColunas[LINHAS] = {2, 3, 4, 5};
const byte pinosLinhas[COLUNAS] = {9, 8, 7, 6};
const byte pinoSensor = A3;
const byte pinoPiezo = A0;
const byte pinoBotaoModo = 15;
const byte pinoLedBotaoModo = A1;

char hexaKeys[LINHAS][COLUNAS] = {
  {'0', '4', '8', 'C'},
  {'1', '5', '9', 'D'},
  {'2', '6', 'A', 'E'},
  {'3', '7', 'B', 'F'}
};

//initialize an instance of class NewKeypad
Keypad teclado = Keypad( makeKeymap(hexaKeys), pinosLinhas, pinosColunas, LINHAS, COLUNAS);

String msg;

int key_order[LINHAS * COLUNAS] = {
  1, 5, 9, 13,
  2, 6, 10,  14,
  3, 7, 11, 15,
  4, 8, 12, 16
};

bool chaves_invertidas[LINHAS * COLUNAS] = {
  true, false,  false, false,
  true,  true,  false, false,
  true,  true,  false, false,
  false,  true, false, false
};

byte notaMidi[LINHAS * COLUNAS] = {
  60, 64, 68, 72,
  61, 65, 69, 73,
  62, 66, 70, 74,
  63, 67, 71, 75
};

byte divisoesDoCompasso[8] = {
  1, 2, 3, 4,
  6, 8, 12, 24
};


byte ccMidiSensor = 11;

int leituraAnalogicaAtual;
int ultimaLeituraAnalogica;
float leituraAnalogicaFiltrada;

unsigned long tempo = 0;
unsigned long tempoAnterior = 0;
int picoPiezo = 0;
byte velocityPiezo = 127;
byte velocityMinimo = 20;
byte sensibilidadePiezo = 600;

int ultimoBotao[2];
int ultimoBaixo;
int bpm = 120;
unsigned long previousMillis = 0;
unsigned long previousMillisMetro = 0;
int ledState = LOW;
Bounce botaoModo = Bounce();
int botoesApertados = 0;
unsigned long tempoBotao = 0;
unsigned long tempoBotaoAnterior = 0;

void setup() {
  Serial.begin(9600);
  pinMode(pinoLedBotaoModo, OUTPUT);
  botaoModo.attach(pinoBotaoModo, INPUT_PULLUP);
  botaoModo.interval(10);
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

            if (notaMidi[teclado.key[i].kcode] > 67) { //só pega velocity se for da mão direita (notas de 68 a 75)
              //começa detecção de velocity com piezo
              botoesApertados++;
              Serial.println(botoesApertados);
              int leituraPiezo = analogRead(pinoPiezo);
              picoPiezo = leituraPiezo;
              tempo = millis();
              tempoAnterior = tempo;
              while (tempo - tempoAnterior <= TEMPO_LEITURA_PIEZO) {
                //        Serial.println(tempo - tempoAnterior);
                leituraPiezo = analogRead(pinoPiezo);
                if (leituraPiezo > picoPiezo) {
                  picoPiezo = leituraPiezo;
                  //                  Serial.println(picoPiezo);
                }
                tempo = millis();
              }
              //              Serial.println(picoPiezo);
              velocityPiezo = map(picoPiezo, 0, sensibilidadePiezo, velocityMinimo, 127);
              if (velocityPiezo > 127) {
                velocityPiezo = 127;
              } if (velocityPiezo < velocityMinimo) {
                velocityPiezo = velocityMinimo;
              }
              ultimoBotao[0] = notaMidi[teclado.key[i].kcode];
              ultimoBotao[1] = velocityPiezo;
              //              Serial.print (ultimoBotao[0]);
              //              Serial.print (", ");
              //              Serial.println (ultimoBotao[1]);
            } else { //se for um baixo (notas de 60 a 67)
              velocityPiezo = 127;
              ultimoBaixo = key_order[teclado.key[i].kcode];
              //              Serial.println(ultimoBaixo);
            }

            //Dispara a nota
            //            noteOn(MIDI_CHANNEL - 1, notaMidi[teclado.key[i].kcode], velocityPiezo);   // Channel 0, middle C, normal velocity
            noteOn(MIDI_CHANNEL - 1, notaMidi[teclado.key[i].kcode], 100);   // Channel 0, middle C, normal velocity
            MidiUSB.flush();
            break;
          case HOLD:
            msg = " SEGURADA.";
            break;
          case RELEASED:
            if (notaMidi[teclado.key[i].kcode] > 67) {
              botoesApertados--;
              Serial.println(botoesApertados);
            }
            msg = " SOLTA.";
            //            MIDI.sendNoteOff(notas_abrindo[key_order[teclado.key[i].kcode]] - 12, 0, MIDI_CHANNEL);
            noteOff(MIDI_CHANNEL - 1, notaMidi[teclado.key[i].kcode], 0);  // Channel 0, middle C, normal velocity
            MidiUSB.flush();
            break;
          case IDLE:
            msg = " SOLTISSIMA.";
        }
        //        Serial.print("Tecla ");
        //        Serial.print(key_order[teclado.key[i].kcode]);
        //        Serial.println(msg);
      }
    }
  }

  botaoModo.update();
  if (botaoModo.fell()) {
    //    digitalWrite(pinoLedBotaoModo, HIGH);
    //    Serial.println("APERTOU MODO");
    tempoBotaoAnterior = tempoBotao;
    tempoBotao = millis();
    unsigned long intervalo = (tempoBotao - tempoBotaoAnterior);
    unsigned long bpmBotao = 1000. / intervalo * 60.;
    //  Serial.println(bpmBotao);
    bpm = bpmBotao;
    previousMillis = millis();
  }

  float interval = (1000 / bpm) * 60;
  unsigned long currentMillisMetro = millis();

  if (currentMillisMetro - previousMillisMetro > interval / 2) {
    previousMillisMetro = currentMillisMetro;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
      digitalWrite(pinoLedBotaoModo, ledState);
    }
    else {
      ledState = LOW;
      digitalWrite(pinoLedBotaoModo, ledState);
      // set the LED with the ledState of the variable:
    }
  }

  unsigned long currentMillis = millis();
  if (botoesApertados > 0) {
    if (currentMillis - previousMillis > interval / divisoesDoCompasso[ultimoBaixo - 1] * 4.) {
      previousMillis = currentMillis;


      noteOn(MIDI_CHANNEL - 1, ultimoBotao[0], ultimoBotao[1]);   // canal_midi, nota_midi, velocity_midi
      MidiUSB.flush();

      noteOff(MIDI_CHANNEL - 1, ultimoBotao[0], 0);   // canal_midi, nota_midi, velocity_midi
      MidiUSB.flush();
      // set the LED with the ledState of the variable:

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
