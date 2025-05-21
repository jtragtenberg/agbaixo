/*
 ____  _____ _        _    
| __ )| ____| |      / \   
|  _ \|  _| | |     / _ \  
| |_) | |___| |___ / ___ \ 
|____/|_____|_____/_/   \_\

http://bela.io

C++ Real-Time Audio Programming with Bela - Lecture 2: Playing recorded samples
sample-player: partially finished example that plays a 
               recorded sound file in a loop.
*/

#include <Bela.h>
#include <libraries/AudioFile/AudioFile.h>
#include <libraries/Midi/Midi.h>
// #include <libraries/Gui/Gui.h>
// #include <libraries/GuiController/GuiController.h>
#include <cmath>
#include <vector>
#include <ctime>

#include "Sampler.h"

const bool debugMode = false;

// Constants for setting up the sample banks
const int kPercussionSamplers = 8;
const int kBassSamplers = 8;
const int kMaxSamplers = kBassSamplers + kPercussionSamplers;

Sampler samplers[kMaxSamplers];  // Array of Sampler objects

std::vector<std::string> bassFilenames[kBassSamplers] = {
    {"samples/b1.wav"},
    {"samples/b2.wav"},
    {"samples/b3.wav"},
    {"samples/b4.wav"},
    {"samples/b5.wav"},
    {"samples/b6.wav"},
    {"samples/b7.wav"},
    {"samples/b8.wav"}
};

std::vector<std::string> percussionFilenames[kPercussionSamplers] = {
    {"samples/p1.3.1.wav","samples/p1.3.2.wav","samples/p1.3.3.wav","samples/p1.3.4.wav","samples/p1.3.5.wav"},
    {"samples/p2.2.1.wav"},
    {"samples/p3.1.wav"},
    {"samples/p4.3.1.wav"},
    {"samples/p5.3.1.wav","samples/p5.3.2.wav","samples/p5.3.3.wav"},
    {"samples/p6.3.1.wav","samples/p6.3.2.wav","samples/p6.3.3.wav"},
    {"samples/p7.1.wav"},
    {"samples/p8.3.1.wav"}
};

// Device for handling MIDI messages
Midi gMidi;

const char* gMidiPort0 = "hw:1,0,0";

// Handling for multiple MIDI notes
const int kMaxActiveNotes = 16;
int gActiveNotes[kMaxActiveNotes];
int gActiveNoteCount = 0;

// std::string gFilenames[3] = {"samples/p1_2_1.wav","samples/p1_2_2.wav","samples/p1_2_3.wav"};	// Name of the sound file (in project folder)
// std::string gP2Filenames[1] = {"samples/p2_2_1.wav"};	// Name of the sound file (in project folder)

// std::string gFilenames[3] = {"test_random-01.wav","test_random-02.wav","test_random-03.wav"};	// Name of the sound file (in project folder)
std::vector<std::vector<float>> gSampleBuffers(3);				// Buffer that holds the sound file
int gReadPointer = -1;							// Position of the last frame we played 
int gSampleSelector = 0;

// // Browser-based GUI to adjust parameters
// Gui gGui;
// GuiController gGuiController;

// MIDI callback function
void midiEvent(MidiChannelMessage message, void *arg);

bool setup(BelaContext *context, void *userData)
{
	for (int i = 0; i < kBassSamplers; ++i) {
        samplers[i].setFilenames(bassFilenames[i]);
        samplers[i].setup(context->audioSampleRate);
        samplers[i].setAdsrParameters(0.01, 0.0, 1.0, 1.0);
        samplers[i].setMidiNote(60 + i);  // Bass notes from 60 to 67
    }

    for (int i = 0; i < kPercussionSamplers; ++i) {
        samplers[kBassSamplers + i].setFilenames(percussionFilenames[i]);
        samplers[kBassSamplers + i].setup(context->audioSampleRate);
        samplers[kBassSamplers + i].setAdsrParameters(0.01, 3.0, 0.0, 3.0);
        samplers[kBassSamplers + i].setMidiNote(68 + i);  // Percussion notes from 68 to 75
        samplers[kBassSamplers + i].setReleaseOnNoteOff(false); // Set release on note off if needed
    }
    //Ganzás botões percussão 3 e 7
    samplers[kBassSamplers + 3 - 1].setReleaseOnNoteOff(true);
    samplers[kBassSamplers + 3 - 1].setAdsrParameters(0.01, 0.0, 1.0, 0.1);
    samplers[kBassSamplers + 3 - 1].setLoopMode(true);
    samplers[kBassSamplers + 7 - 1].setReleaseOnNoteOff(true);
    samplers[kBassSamplers + 7 - 1].setAdsrParameters(0.01, 0.0, 1.0, 0.1);
    samplers[kBassSamplers + 7 - 1].setLoopMode(true);
    
	
	// Initialise the MIDI device
	if(gMidi.readFrom(gMidiPort0) < 0) {
		if (debugMode) rt_printf("Unable to read from MIDI port %s\n", gMidiPort0);
		return false;
	}
	
	gMidi.writeTo(gMidiPort0);
	gMidi.enableParser(true);
	gMidi.setParserCallback(midiEvent, (void *)gMidiPort0);
	
	// // Set up the GUI
	// gGui.setup(context->projectName);
	// gGuiController.setup(&gGui, "ADSR Controller");	
	
	// // Arguments: name, minimum, maximum, increment, default value
	// gGuiController.addSlider("Amplitude Attack time", 0.01, 0.001, 3.0, 0);
	// gGuiController.addSlider("Amplitude Decay time", 0.25, 0.001, 3.0, 0);
	// gGuiController.addSlider("Amplitude Sustain level", 0.0, 0, 1, 0);
	// gGuiController.addSlider("Amplitude Release time", 3.0, 0.001, 3.0, 0);

	return true;
}

// MIDI note on received
void noteOn(int noteNumber, int velocity) 
{
    for (int i = 0; i < kMaxSamplers; ++i) {
        if (noteNumber == samplers[i].getMidiNote()) {
            samplers[i].trigger();
        }
    }

	// Check if we have any note slots left
	// if(gActiveNoteCount < kMaxActiveNotes) {
	// 	// Keep track of this note, then play it
	// 	gActiveNotes[gActiveNoteCount++] = noteNumber;
		
	// 	// Map velocity to amplitude on a decibel scale
	// 	// float decibels = map(velocity, 1, 127, -40, 0);
	// 	// gAmplitude = powf(10.0, decibels / 20.0);
	
	// 	// Start the ADSR if this was the first note pressed
	// 	// if(gActiveNoteCount == 1) {
	// 	// }
		
	// 	//trigger a note
		
	// }
}

// MIDI note off received
void noteOff(int noteNumber)
{
    for (int i = 0; i < kMaxSamplers; ++i) {
        if (noteNumber == samplers[i].getMidiNote() && samplers[i].getReleaseOnNoteOff()) {
        	if (debugMode) rt_printf("Note Off\n");
            samplers[i].release();
        }
    }
	// bool activeNoteChanged = false;
	
	// // Go through all the active notes and remove any with this number
	// for(int i = gActiveNoteCount - 1; i >= 0; i--) {
	// 	if(gActiveNotes[i] == noteNumber) {
	// 		// Found a match: is it the most recent note?
	// 		if(i == gActiveNoteCount - 1) {
	// 			activeNoteChanged = true;
	// 		}
	
	// 		// Move all the later notes back in the list
	// 		for(int j = i; j < gActiveNoteCount - 1; j++) {
	// 			gActiveNotes[j] = gActiveNotes[j + 1];
	// 		}
	// 		gActiveNoteCount--;
	// 	}
	// }
	
	// if(gActiveNoteCount == 0) {
	// 	// No notes left: stop the ADSR
    
	// 	// gFilterADSR.release();
	// }
	// else if(activeNoteChanged) {
	// 	// Update the frequency but don't retrigger
	// 	int mostRecentNote = gActiveNotes[gActiveNoteCount - 1];
		
	// 	// gCentreFrequency.rampTo(calculateFrequency(mostRecentNote), gPortamentoTime);
	// }
}



void render(BelaContext *context, void *userData)
{
    // // Retrieve values from the sliders and set the ADSR parameters
    // sampler1.setAdsrParameters(
    //   gGuiController.getSliderValue(0),
    //   gGuiController.getSliderValue(1),
    //   gGuiController.getSliderValue(2),
    //   gGuiController.getSliderValue(3)
    // );
    // sampler2.setAdsrParameters(
    //   gGuiController.getSliderValue(0),
    //   gGuiController.getSliderValue(1),
    //   gGuiController.getSliderValue(2),
    //   gGuiController.getSliderValue(3)
    // );
	
    for (unsigned int n = 0; n < context->audioFrames; n++) {
        float out = 0;
        
        for (int i = 0; i < kMaxSamplers; ++i) {
            out += samplers[i].process();
        }
 
        // Write the sample to every audio output channel
        for (unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
            audioWrite(context, n, channel, out);
        }
    }
}

// This callback function is called every time a new MIDI message is available
// This happens on a different thread than the audio processing

void midiEvent(MidiChannelMessage message, void *arg) {
	// Display the port, if available
	if(arg != NULL) {
		if (debugMode) rt_printf("Message from midi port %s ", (const char*) arg);
	}
	
	// Display the message
	if (debugMode) message.prettyPrint();
		
	// A MIDI "note on" message type might actually hold a real
	// note onset (e.g. key press), or it might hold a note off (key release).
	// The latter is signified by a velocity of 0.
	if(message.getType() == kmmNoteOn) {
		int noteNumber = message.getDataByte(0);
		int velocity = message.getDataByte(1);
		
		// Velocity of 0 is really a note off
		if(velocity == 0) {
			noteOff(noteNumber);
		}
		else {
			noteOn(noteNumber, velocity);
		}
	}
	else if(message.getType() == kmmNoteOff) {
		// We can also encounter the "note off" message type which is the same
		// as "note on" with a velocity of 0.
		int noteNumber = message.getDataByte(0);
		
		noteOff(noteNumber);
	}
	// else if (message.getType() == kmmPitchBend) {
	// 	int pitchBend = message.getDataByte(0) + (message.getDataByte(1) << 7);
		
	// 	pitchWheel(pitchBend);
	// }
	// else if (message.getType() == kmmControlChange) {
	// 	int ccNumber = message.getDataByte(0);
	// 	int ccValue = message.getDataByte(1);
		
	// 	controlChange(ccNumber, ccValue);
	// }
}

void cleanup(BelaContext *context, void *userData)
{

}
