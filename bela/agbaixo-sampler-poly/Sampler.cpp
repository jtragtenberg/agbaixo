#include "Sampler.h"
#include <libraries/AudioFile/AudioFile.h>
#include <cstdlib>
#include <ctime>


Sampler::Sampler() : 
    readPointer(-1), sampleSelector(0),
    attackTime(0.01), decayTime(0.25), sustainLevel(0.0), releaseTime(3.0), 
	midiNote(-1), releaseOnNoteOff(true), loopMode(false)
{
    amplitudeADSR.setSampleRate(44100.0f); // Default sample rate, should be overridden in setup()
    updateADSR();
}

void Sampler::setFilenames(const std::vector<std::string>& filenames) {
    this->filenames = filenames;
}

void Sampler::setup(float sampleRate) {
    amplitudeADSR.setSampleRate(sampleRate);
    sampleBuffers.resize(filenames.size());
    for (size_t i = 0; i < filenames.size(); i++) {
        sampleBuffers[i] = AudioFileUtilities::loadMono(filenames[i]);
        if (sampleBuffers[i].size() == 0) {
            throw std::runtime_error("Error loading audio file '" + filenames[i] + "'");
        } else {
        	rt_printf("Loaded the audio file '%s' with %d frames (%.1f seconds), played in the sample-rate %f\n", 
    			filenames[i].c_str(), sampleBuffers[i].size(),
    			sampleBuffers[i].size() / sampleRate, sampleRate);
        }
    }
    std::srand(std::time(0)); // Seed the random number generator
}

void Sampler::trigger() {
    sampleSelector = std::rand() % sampleBuffers.size();
    readPointer = 0;
    amplitudeADSR.trigger();
    //rt_printf("loaded sample variation #%d\n", sampleSelector);
}

void Sampler::release() {
    amplitudeADSR.release();
    //rt_printf("released\n");
}

void Sampler::setAttackTime(float attackTime) {
    this->attackTime = attackTime;
    updateADSR();
}

void Sampler::setDecayTime(float decayTime) {
    this->decayTime = decayTime;
    updateADSR();
}

void Sampler::setSustainLevel(float sustainLevel) {
    this->sustainLevel = sustainLevel;
    updateADSR();
}

void Sampler::setReleaseTime(float releaseTime) {
    this->releaseTime = releaseTime;
    updateADSR();
}

void Sampler::setAdsrParameters(float attackTime, float decayTime, float sustainLevel, float releaseTime) {
    this->attackTime = attackTime;
    this->decayTime = decayTime;
    this->sustainLevel = sustainLevel;
    this->releaseTime = releaseTime;
    updateADSR();
}

void Sampler::updateADSR() {
    amplitudeADSR.setAttackTime(attackTime);
    amplitudeADSR.setDecayTime(decayTime);
    amplitudeADSR.setSustainLevel(sustainLevel);
    amplitudeADSR.setReleaseTime(releaseTime);
}

void Sampler::setMidiNote(int midiNote) {
    this->midiNote = midiNote;
}

int Sampler::getMidiNote() const {
    return midiNote;
}

void Sampler::setReleaseOnNoteOff(bool releaseOnNoteOff) {
    this->releaseOnNoteOff = releaseOnNoteOff;
}


bool Sampler::getReleaseOnNoteOff() const {
    return releaseOnNoteOff;
}

void Sampler::setLoopMode(bool loop) {
    this->loopMode = loop;
}

bool Sampler::getLoopMode() const {
    return loopMode;
}

float Sampler::process() {
    if (readPointer == -1) {
        return 0.f;
    }

    float out = 0.5 * sampleBuffers[sampleSelector][readPointer] * amplitudeADSR.process();
    readPointer++;
    
    if (readPointer >= static_cast<int>(sampleBuffers[sampleSelector].size())) {
        if (loopMode) {
            readPointer = 0;  // Reset to start of sample if in loop mode
        } else {
        	release();
        	readPointer = -1; // Release if not in loop mode
        }
    }

    return out;
}