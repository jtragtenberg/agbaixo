/*
 ____  _____ _        _    
| __ )| ____| |      / \   
|  _ \|  _| | |     / _ \  
| |_) | |___| |___ / ___ \ 
|____/|_____|_____/_/   \_\

http://bela.io

Updated MIDI handling to ensure proper routing to external applications.
*/

#include <Bela.h>
#include <libraries/AudioFile/AudioFile.h>
#include <libraries/Midi/Midi.h>
#include <cmath>
#include <vector>
#include <ctime>

#include "Sampler.h"

const bool debugMode = true;

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

// Devices for handling MIDI messages
Midi gMidiInput;
Midi gMidiOutput;

// Updated MIDI Ports (Ensure these are properly configured)
const char* gMidiInputPort = "hw:1,0,0"; // MIDI input from Arduino
const char* gMidiOutputPort = "default"; // MIDI output to macOS or other connected devices

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
    // Set loop mode and release on note off for specific samples
    samplers[kBassSamplers + 3 - 1].setReleaseOnNoteOff(true);
    samplers[kBassSamplers + 3 - 1].setAdsrParameters(0.01, 0.0, 1.0, 0.1);
    samplers[kBassSamplers + 3 - 1].setLoopMode(true);
    samplers[kBassSamplers + 7 - 1].setReleaseOnNoteOff(true);
    samplers[kBassSamplers + 7 - 1].setAdsrParameters(0.01, 0.0, 1.0, 0.1);
    samplers[kBassSamplers + 7 - 1].setLoopMode(true);
    
    // Initialise the MIDI input device from Arduino
    if(gMidiInput.readFrom(gMidiInputPort) < 0) {
        if (debugMode) rt_printf("Unable to read from MIDI port %s\n", gMidiInputPort);
        return false;
    } else {
        if (debugMode) rt_printf("Successfully opened MIDI input port: %s\n", gMidiInputPort);
    }
    
    // Enable MIDI output to be received by other applications (macOS)
    if(gMidiOutput.writeTo(gMidiOutputPort) < 0) {
        if (debugMode) rt_printf("Unable to write to MIDI port %s\n", gMidiOutputPort);
        return false;
    } else {
        if (debugMode) rt_printf("Successfully opened MIDI output port: %s\n", gMidiOutputPort);
    }

    gMidiInput.enableParser(true);
    gMidiInput.setParserCallback(midiEvent, (void *)gMidiInputPort);
    
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
}

void render(BelaContext *context, void *userData)
{   
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
    if (arg != NULL) {
        if (debugMode) rt_printf("Message from MIDI port: %s\n", (const char*) arg);
    }

    // Display the message
    if (debugMode) {
        rt_printf("MIDI message received!\n");
        message.prettyPrint();  // Print the full message for debugging
    }

    midi_byte_t channel = message.getChannel();  // Extract the MIDI channel

    // Handle Note On and Note Off
    if (message.getType() == kmmNoteOn) {
        int noteNumber = message.getDataByte(0);
        int velocity = message.getDataByte(1);

        // Output the MIDI note information
        rt_printf("Note On received - Note: %d, Velocity: %d\n", noteNumber, velocity);

        // Velocity of 0 is really a note off
        if (velocity == 0) {
            noteOff(noteNumber);
            gMidiOutput.writeNoteOff(channel, noteNumber, velocity);  // Send Note Off
        } else {
            noteOn(noteNumber, velocity);
            gMidiOutput.writeNoteOn(channel, noteNumber, velocity);  // Send Note On
        }
    }
    else if (message.getType() == kmmNoteOff) {
        int noteNumber = message.getDataByte(0);

        // Output the MIDI note off information
        rt_printf("Note Off received - Note: %d\n", noteNumber);

        noteOff(noteNumber);
        gMidiOutput.writeNoteOff(channel, noteNumber, 0);  // Send Note Off
    }
    // Handle Control Change
    else if (message.getType() == kmmControlChange) {
        int ccNumber = message.getDataByte(0);
        int ccValue = message.getDataByte(1);

        // Output Control Change data
        rt_printf("Control Change received - CC#: %d, Value: %d\n", ccNumber, ccValue);

        // Send the Control Change message back out via MIDI
        gMidiOutput.writeControlChange(channel, ccNumber, ccValue);
    }
    // Handle Pitch Bend
    else if (message.getType() == kmmPitchBend) {
        int lsb = message.getDataByte(0);
        int msb = message.getDataByte(1);
        int pitchBend = (msb << 7) | lsb;  // Combine LSB and MSB

        // Output Pitch Bend data
        rt_printf("Pitch Bend received - Value: %d\n", pitchBend);

        // Send the Pitch Bend message back out via MIDI
        gMidiOutput.writePitchBend(channel, pitchBend);
    }
}

void cleanup(BelaContext *context, void *userData)
{
    // Cleanup code if needed
}
