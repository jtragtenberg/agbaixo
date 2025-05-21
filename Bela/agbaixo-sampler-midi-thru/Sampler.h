#ifndef SAMPLER_H
#define SAMPLER_H

#include <vector>
#include <string>
#include "ADSR.h"

class Sampler {
public:
    Sampler();
    void setFilenames(const std::vector<std::string>& filenames);
    void setup(float sampleRate);
    void trigger();
    void release();
    float process();
    
    ADSR amplitudeADSR;
    
    // Setter methods for ADSR parameters
    void setAttackTime(float attackTime);
    void setDecayTime(float decayTime);
    void setSustainLevel(float sustainLevel);
    void setReleaseTime(float releaseTime);
    
    //method to set all ADSR parameters at once
    void setAdsrParameters(float attackTime, float decayTime, float sustainLevel, float releaseTime);
    
    // Setter and getter for MIDI note
    void setMidiNote(int midiNote);
    int getMidiNote() const;
    
    // Setter and getter for release on note off
    void setReleaseOnNoteOff(bool releaseOnNoteOff);
    bool getReleaseOnNoteOff() const;
    
    // Setter and getter for loop mode
    void setLoopMode(bool loop);
    bool getLoopMode() const;

    
private:
    std::vector<std::vector<float>> sampleBuffers;  // Buffer that holds the sound files
    std::vector<std::string> filenames; // Names of the sound files
    int readPointer;
    int sampleSelector;
    
    float attackTime;
    float decayTime;
    float sustainLevel;
    float releaseTime;
    
    int midiNote;  // Variable to hold the MIDI note associated with the sampler
    bool releaseOnNoteOff;  // Variable to determine if release is triggered on note off
    bool loopMode;  // Variable to enable/disable loop mode
    
    void updateADSR();
};

#endif // SAMPLER_H