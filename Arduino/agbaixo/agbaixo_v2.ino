#include <Keypad.h>
#include <MIDIUSB.h>
#include <Bounce2.h>


//  GENERAL VARIABLES
const int MIDI_CHANNEL = 9;
const byte LINHAS = 4;
const byte COLUNAS = 4;
byte pinosColunas[LINHAS] = {2, 3, 4, 5};
byte pinosLinhas[COLUNAS] = {9, 8, 7, 6};
const byte pinoBotaoModo = 15;
const byte pinoLedBotaoModo = A1;
const byte pinoLooperLED = A2;
const byte pinoLooperPedal = A3;
char hexaKeys[LINHAS][COLUNAS] = {
  {'0', '4', '8', 'C'},
  {'1', '5', '9', 'D'},
  {'2', '6', 'A', 'E'},
  {'3', '7', 'B', 'F'}
};
Keypad teclado = Keypad(makeKeymap(hexaKeys), pinosLinhas, pinosColunas, LINHAS, COLUNAS); //initialize an instance of class NewKeypad
String msg;
int key_order[LINHAS * COLUNAS] = {
  1, 5, 9, 13,
  2, 6, 10,  14,
  3, 7, 11, 15,
  4, 8, 12, 16
};
const byte notaMidi[LINHAS * COLUNAS] = {
  60, 64, 68, 72,
  61, 65, 69, 73,
  62, 66, 70, 74,
  63, 67, 71, 75
};
const byte divisoesDoCompasso[8] = {
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
int velocity = 100;

// TAP TEMPO VARIABLES
bool modoRepeat = false;
bool ttwaspressed = false;
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

// LOOPER VARIABLES
Bounce looperPedal = Bounce();
bool isRecording = false;
bool isPlaying = false;
bool isOverdubbing = false;
bool ledStateLooper = false;
struct MidiMessage {
  byte type;
  //byte channel;
  byte data1;
  //byte data2;
  unsigned long timestamp;
};
const unsigned int loopbuffersize = 64;
const unsigned int maxOverdubLayers = 3;
MidiMessage midiLoop[maxOverdubLayers][loopbuffersize];
byte NloopMsgs[maxOverdubLayers];  // Number of messages in each layer
int currentLayer = 0;
unsigned long loopStartTime = 0;
unsigned long loopDuration = 0;
unsigned long currentLoopTime = 0;
int currentblinkt = 0;
int previousblinkt = 0;
int blinktime = 0;

// SETUP
void setup() {
  Serial.begin(9600);
  pinMode(pinoLedBotaoModo, OUTPUT);
  pinMode(pinoLooperLED, OUTPUT);
  botaoModo.attach(pinoBotaoModo, INPUT_PULLUP);
  botaoModo.interval(10);
  looperPedal.attach(pinoLooperPedal, INPUT_PULLUP);
  looperPedal.interval(10);
}

void loop(){
  //  BUTTON READING & DEBOUNCE
  if (teclado.getKeys()) {
    for (int i = 0; i < LIST_MAX; i++) {
      if (teclado.key[i].stateChanged) {
        switch (teclado.key[i].kstate) {  // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
          case PRESSED:
              msg = " APERTADA.";
              if (key_order[teclado.key[i].kcode] > 8) { //se for botão
                dispararBotao(teclado.key[i].kcode);
              } else { //se for baixo
                dispararBaixo(teclado.key[i].kcode);
              }
              break;

          case HOLD:
            msg = " SEGURADA.";
            break;

          case RELEASED:
              msg = " SOLTA.";
              if (key_order[teclado.key[i].kcode] > 8) {
                soltarBotao(teclado.key[i].kcode);
              } else {
                soltarBaixo(teclado.key[i].kcode);
              }
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

  //  TAP TEMPO
  botaoModo.update();
  if (botaoModo.fell()) {
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

//TODO: não colocar o noteOff logo depois do noteON
//TODO: Corrigir algoritmo de contagem de botões apertados

    if (botoesApertados > 0) {
      if (currentMillis - previousMillis > (interval / divisoesDoCompasso[ultimoBaixo - 1] * 4.)) {
        previousMillis = currentMillis;
        noteOff(MIDI_CHANNEL - 1, ultimoBotao[1], 0);   // canal_midi, nota_midi, velocity_midi
        MidiUSB.flush();

        noteOn(MIDI_CHANNEL - 1, ultimoBotao[0], velocity);   // canal_midi, nota_midi, velocity_midi
        MidiUSB.flush();
        ttwaspressed = true;
      }
    }
    if (botoesApertados <= 0 && ttwaspressed){
        noteOff(MIDI_CHANNEL - 1, ultimoBotao[0], velocity);   // canal_midi, nota_midi, velocity_midi
        MidiUSB.flush();
        ttwaspressed = false;
    }
  } 
  else {
    digitalWrite(pinoLedBotaoModo, LOW);
  }

  //  LOOP
  loopstate();
  loopLED();
}

/////////////////////////////////////////////////////////////////////////////////////

//NOTE ON
void noteOn(byte channel, byte pitch, byte velocity) {
    midiEventPacket_t noteOn = {
    static_cast<uint8_t>(0x09),
    static_cast<uint8_t>(0x90 | channel),
    static_cast<uint8_t>(pitch),
    static_cast<uint8_t>(velocity)
};
  MidiUSB.sendMIDI(noteOn);
  if (isRecording) {
    recordMessage(0x09, pitch);
  }
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
  if (isRecording) {
    recordMessage(0x08, pitch);
  }
}

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

void dispararBotao(int keyCode_) {
  if (modoRepeat) {
    //começa detecção de velocity com piezo
    botoesApertados++;
    Serial.println(botoesApertados);
    chavesApertadas[keyCode_] = true;
    ultimoBotao[1] = ultimoBotao[0];
    ultimoBotao[0] = notaMidi[keyCode_];
  } 
  else {
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
  for (int i=9; i<=15; i++){
    if (chavesApertadas[i]) {
      c++;
    }
  }
  return c;
}



void loopstate(){
  looperPedal.update();
  if (looperPedal.fell()) {
    if (!isRecording && !isPlaying) {
      startRecording();
    } 
    else if (isRecording && !isPlaying) {
      stopRecording();
    } 
    else if (isPlaying && !isOverdubbing) {
      startOverdub();
    }
    else if (isPlaying && isOverdubbing){
      stopOverdub();
    }
  }
  if (isRecording && NloopMsgs[currentLayer]==loopbuffersize){
    stopRecording();
  }
  if (isOverdubbing && ((currentLoopTime%loopDuration)<=0.01 || NloopMsgs[currentLayer]==loopbuffersize)){
    stopOverdub();
  }
  if (isPlaying) {
    int looperpedalstate = looperPedal.read();
    if (looperpedalstate == LOW && looperPedal.currentDuration() > 1000) {
      isRecording = false;
      isPlaying = false;
      isOverdubbing = false;
      ledStateLooper = false;
      for (int i=0;i<loopbuffersize;i++){
        for (int layer=0; layer<maxOverdubLayers; layer++) {
          midiLoop[layer][i].type = 0;
          //midiLoop[layer][i].channel = 0;
          midiLoop[layer][i].data1 = 0;
          //midiLoop[layer][i].data2 = 0;
          midiLoop[layer][i].timestamp = 0;
        }
      }
      for (int layer=0; layer<maxOverdubLayers; layer++) {
        NloopMsgs[layer] = 0;
      }
      currentLoopTime = 0;
      loopStartTime = 0;
      currentLayer = 0;
      currentblinkt = 0;
      previousblinkt = 0;
      blinktime = 0;
      Serial.println("ERASED LOOP");
    }
    else{
      playLoop();
    }
  }
}



void startRecording() {
  Serial.println("Recording Started");
  isRecording = true;
  isPlaying = false;
  NloopMsgs[currentLayer] = 0;  // Reset the current layer's message count
  if (currentLayer==0){
    loopStartTime = millis();
  }
}

void stopRecording() {
  Serial.println("Recording Stopped");
  isRecording = false;
  isPlaying = true;

  NloopMsgs[currentLayer];  // Set loopLength to the current layer's message count

  if (currentLayer==0){
    loopDuration = millis() - loopStartTime;
  }
  previousblinkt = millis();
}



void startOverdub(){
  if (currentLayer < maxOverdubLayers) {
    Serial.println("Overdub Started");
    isOverdubbing = true;
    isRecording = true;
    currentLayer++;
    NloopMsgs[currentLayer] = 0;  // Start a new layer
  }
}

void stopOverdub(){
  Serial.println("Overdub Stopped");
  isOverdubbing = false;
  isRecording = false;
}



void recordMessage(byte type, byte data1) {
  if (NloopMsgs[currentLayer] < loopbuffersize) {
    midiLoop[currentLayer][NloopMsgs[currentLayer]].type = type;
   // midiLoop[currentLayer][NloopMsgs[currentLayer]].channel = channel;
    midiLoop[currentLayer][NloopMsgs[currentLayer]].data1 = data1;
   // midiLoop[currentLayer][NloopMsgs[currentLayer]].data2 = data2;
    midiLoop[currentLayer][NloopMsgs[currentLayer]].timestamp = millis() - loopStartTime;
    NloopMsgs[currentLayer]++;
  }
}

void playLoop() {
  currentLoopTime = millis() - loopStartTime;
  for (int layer = 0; layer <= currentLayer; layer++) {
    for (int i = 0; i < NloopMsgs[layer]; i++) {
      if (midiLoop[layer][i].timestamp <= currentLoopTime) {
        if (midiLoop[layer][i].type == 0x09) {
          noteOn(MIDI_CHANNEL, midiLoop[layer][i].data1, velocity);
          MidiUSB.flush();
        } else if (midiLoop[layer][i].type == 0x08) {
          noteOff(MIDI_CHANNEL, midiLoop[layer][i].data1, 0);
          MidiUSB.flush();
        }
        midiLoop[layer][i].timestamp += loopDuration;
      }
    }
  }
}



void loopLED(){
  if (!isRecording && !isPlaying) {
    digitalWrite(pinoLooperLED,LOW);
    ledStateLooper=false;
  } 
  else if (isRecording) {
    digitalWrite(pinoLooperLED,HIGH);
    ledStateLooper=true;
  } 
  else if (isPlaying) {
    blinkloop();
  }
}

void blinkloop(){
  int blinktime = loopDuration / 4;
  currentblinkt = millis();
  if (currentblinkt - previousblinkt >= blinktime) {
    previousblinkt = currentblinkt;
    if (!ledStateLooper) {
      digitalWrite(pinoLooperLED, HIGH);
      ledStateLooper = true;   
    } else {
      digitalWrite(pinoLooperLED, LOW);
      ledStateLooper = false;
    }
  }
}
