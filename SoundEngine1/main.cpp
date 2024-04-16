#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <math.h>

#include <AL/al.h>
#include <AL/alc.h>

#define SDL_MAIN_HANDLED
#include <SDL/SDL.h>
#include "ALUtilities.h"

using namespace std;

int main() {
	//set up openAL context
	ALCdevice* device;
	ALCcontext* context;
	ALSetup(device, context);

	//set up audio source
	soundFile chirping = createSoundFile("./sounds/chirp.wav");
	initAudioSource(chirping);
	//TODO: load other files
	//test stopping all sounds (even if one is not playing?)
	//assign them to a key

	//set up applications
	//SDL_SetVideoMode has been depricated: https://stackoverflow.com/q/28400401/16858784
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window* window;
	SDL_Surface* screenSurface;
	SDLSetup(window, screenSurface, 480, 640);
	SDL_SetRelativeMouseMode(SDL_TRUE);

	//get ready for rendering loop
	Uint64 start;
	bool running = true;
	float speed = 0.1;
	float sensitivity = 0.005; //in degrees?
	Listener me;

	//set global volume
	float volume = 1;
	alListenerf(AL_GAIN, volume); //appears to only accept values between (0,1)
	//alSourcef(currentSourceID, AL_GAIN, newVolume); //to set volume of a source

	while (running) {
		start = SDL_GetTicks64();

		//process key inputs
		keyInput(running, speed, sensitivity, me, chirping);

		//position of sound source
		alSource3f(chirping.sourceid, AL_POSITION, chirping.x, chirping.y, chirping.z);
		alSourcei(chirping.sourceid, AL_LOOPING, AL_TRUE); // makes the sound continuously loop once initiated
		
		//position of listener
		float playerVec[] = { me.f.x, me.f.y, me.f.z,//forward
							me.up.x, me.up.y, me.up.z };//up
		alListenerfv(AL_ORIENTATION, playerVec);
		
		//necessary for sound playing when moving
		if (1000 / 30 > SDL_GetTicks64() - start) {
			SDL_Delay(1000 / 30 - (SDL_GetTicks64() - start));
		}
	}

	//program termination
	deleteSoundFile(chirping);
	freeContext(device, context);
	return 0;
}