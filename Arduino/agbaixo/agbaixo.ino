#include <Keypad.h>
#include <MIDIUSB.h>
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
  2, 3, 4,
   6, 8,
  12, 16, 32
};

bool chavesApertadas [LINHAS * COLUNAS] = {
  false, false, false, false,
  false, false, false, false,
  false, false, false, false,
  false, false, false, false
};

byte ccMidiSensor = 11;

int leituraAnalogicaAtual;
int ultimaLeituraAnalogica;
float leituraAnalogicaFiltrada;

unsigned long tempo = 0;
unsigned long tempoAnterior = 0;
int picoPiezo = 0;
int velocityPiezo = 127;
int velocity = 100;
int velocityMinimo = 20;
int sensibilidadePiezo = 600;

int ultimoBotao[2];
int ultimoBaixo = 3;
int bpm = 120;
unsigned long previousMillis = 0;
unsigned long previousMillisMetro = 0;
int ledState = LOW;
Bounce botaoModo = Bounce();
int botoesApertados = 0;
unsigned long tempoBotao = 0;
unsigned long tempoBotaoAnterior = 0;
bool modoRepeat = false;

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
            if (chaves_invertidas[teclado.key[i].kcode]) {  //chaves dos baixos na real são soltas,
              msg = " SOLTA.";
              soltarBaixo(notaMidi[teclado.key[i].kcode]);
              break;
            } else {
              msg = " APERTADA.";
              if (key_order[teclado.key[i].kcode] > 8) { //se for botão
                dispararBotao(teclado.key[i].kcode);
              } else { //se for baixo
                dispararBaixo(teclado.key[i].kcode);
              }
              break;
            }
          case HOLD:
            msg = " SEGURADA.";
            break;
          case RELEASED:
            if (chaves_invertidas[teclado.key[i].kcode]) {
              //chaves na real são presas nesse caso
              msg = " APERTADA.";
              dispararBaixo(teclado.key[i].kcode);
              break;
            } else {
              msg = " SOLTA.";
              if (key_order[teclado.key[i].kcode] > 8) {
                soltarBotao(teclado.key[i].kcode);
              } else {
                soltarBaixo(teclado.key[i].kcode);
              }
              break;
            }
          case IDLE:
            msg = " SOLTISSIMA.";
        }
//        Serial.print("Tecla ");
//        Serial.print(key_order[teclado.key[i].kcode]);
//        Serial.println(msg);
      }
    }
  }
  //  }
  botaoModo.update();
  if (botaoModo.fell()) {
    //    digitalWrite(pinoLedBotaoModo, HIGH);
    Serial.println("APERTOU MODO");
    tempoBotaoAnterior = tempoBotao;
    tempoBotao = millis();
    unsigned long intervalo = (tempoBotao - tempoBotaoAnterior);
    unsigned long bpmBotao = 1000. / intervalo * 60.;
    Serial.println(bpmBotao);
    bpm = bpmBotao;
    modoRepeat = true;
    Serial.println("MODO REPEAT");
    previousMillis = millis();
  }

  int botaoModoState = botaoModo.read();
  if (botaoModoState == LOW  && botaoModo.currentDuration() > 1000) {
    modoRepeat = false;
    Serial.println("MODO NOTA");
  }

  if (modoRepeat) {
    float interval = (1000 / bpm) * 60;
    unsigned long currentMillisMetro = millis();

    if (currentMillisMetro - previousMillisMetro > (interval / 2)) {
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
//    int botoesApertados_ = contagemDeBotoesApertados();
    //Serial.println(botoesApertados);
    if (botoesApertados > 0) {
      if (currentMillis - previousMillis > (interval / divisoesDoCompasso[ultimoBaixo - 1] * 4.)) {
        previousMillis = currentMillis;
        //      noteOn(MIDI_CHANNEL - 1, ultimoBotao[0], ultimoBotao[1]);   // canal_midi, nota_midi, velocity_midi
        noteOn(MIDI_CHANNEL - 1, ultimoBotao[0], velocity);   // canal_midi, nota_midi, velocity_midi
        MidiUSB.flush();

        noteOff(MIDI_CHANNEL - 1, ultimoBotao[0], 0);   // canal_midi, nota_midi, velocity_midi
        MidiUSB.flush();

      }
    }
  } else {
    digitalWrite(pinoLedBotaoModo, LOW);
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

void dispararBotao(int keyCode_) {
  if (modoRepeat) {
    //começa detecção de velocity com piezo
    botoesApertados++;
    Serial.println(botoesApertados);
    chavesApertadas[keyCode_] = true;
    ultimoBotao[0] = notaMidi[keyCode_];
    ultimoBotao[1] = velocity;
  } else {
    noteOn(MIDI_CHANNEL - 1, notaMidi[keyCode_], velocity);   // Channel 0, middle C, normal velocity
    MidiUSB.flush();
  }
}

void soltarBotao(int keyCode_) {
  if (modoRepeat) {
    botoesApertados--;
    Serial.println(botoesApertados);
    chavesApertadas[keyCode_] = false;
  } else {
    noteOff(MIDI_CHANNEL - 1, notaMidi[keyCode_], 0);  // Channel 0, middle C, normal velocity
    MidiUSB.flush();
  }
}

void dispararBaixo(int keyCode_) {
  if (modoRepeat) {
    velocityPiezo = 127;
    ultimoBaixo = key_order[keyCode_];
    chavesApertadas[keyCode_] = true;
      
  } else {
    //Dispara a nota
    noteOn(MIDI_CHANNEL - 1, notaMidi[keyCode_], velocity);   // Channel 0, middle C, normal velocity
    MidiUSB.flush();
  }
}

void soltarBaixo(int keyCode_) {
  if (modoRepeat) {
  chavesApertadas[keyCode_] = false;
  } else {
    noteOff(MIDI_CHANNEL - 1, notaMidi[keyCode_], 0);  // Channel 0, middle C, normal velocity
    MidiUSB.flush();
  }
}

int contagemDeBotoesApertados(){
  int c = 0;
  for (int i=9; i<=18; i++){
    if (chavesApertadas[i]) {
      c++;
    }
  }
  return c;
}
