#include <cstring>
#include <string>
#include <vector>
#include <math.h>
#include<windows.h>

#include <AL/al.h>
#include <AL/alc.h>

#define SDL_MAIN_HANDLED
#include <SDL/SDL.h>
#include "ALUtilities.h"

using namespace std;

template <typename T> int sgn(T val) {
	return (T(0) < val) - (val < T(0));
}

struct sineW { //https://stackoverflow.com/questions/5469030/c-play-back-a-tone-generated-from-a-sinusoidal-wave
	unsigned int sourceid, bufferid;//required at post-initialization
	float freq;						//frequency of sine wave
	int seconds;					//how long it is active (ideally for one phase)
	unsigned int sample_rate;		//sample rate of the sine wave
	size_t buf_size;				//calculated by: seconds * sample_rate

	short* samples;					//array: new short[buf_size]

	sineW(float _freq = 440, int _seconds = 1, unsigned int _sample_rate = 22050)
		: freq(_freq), seconds(_seconds), sample_rate(_sample_rate)
	{
		alGenBuffers(1, &bufferid);
		alGenSources(1, &sourceid);
		buf_size = seconds * sample_rate;
		samples = new short[buf_size];

		//populate sample buffer
		for(int i=0; i<buf_size; ++i)
			//samples[i] = 32768 * 2 * (i * freq - int((i * freq) + 0.5)) / sample_rate;//sawtooth - does not work
			//samples[i] = 32768 * (2/M_PI) * asin(sin((2 * M_PI * freq * i) / sample_rate));//triangle
			//samples[i] = 32768 * sgn(cos((2 * M_PI * freq * i) / sample_rate));			//square
			//samples[i] = 32768 * cos( (2*M_PI * freq * i)/sample_rate);					//sin or cos
		// 32760 because we use mono16, and 16 bits is 32768 (ignoring last digit cuz lazy)

		/* Download buffer to OpenAL */
		alBufferData(bufferid, AL_FORMAT_MONO16, samples, buf_size, sample_rate);
	}
};

int main() {
	//set up openAL context
	ALCdevice* device;
	ALCcontext* context;
	ALSetup(device, context);

	//set up audio sources
	//set up sources loaded from files
	//each sound is assigned to keys [1-9]. Press ["] to make them all stop
	std::vector<std::string> soundFiles(
										{	"./sounds/chirp.wav",
											"./sounds/sine.wav",
											"./sounds/sine_beeping.wav",
											"./sounds/whitenoise.wav" 
										}
									   );
	std::vector<soundFile*> soundsFiles = createSounds(soundFiles);


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

	sineW mySine;
	alSourcei(mySine.sourceid, AL_BUFFER, mySine.bufferid);
	alSourcePlay(mySine.sourceid);

	while (running) {
		start = SDL_GetTicks64();

		//process key inputs
		keyInput(running, speed, sensitivity, me, soundsFiles);

		//position of sound source for sounds loaded from file
		for (int i = 0; i < soundsFiles.size(); i++) {
			alSource3f(soundsFiles[i]->sourceid, AL_POSITION, soundsFiles[i]->x, soundsFiles[i]->y, soundsFiles[i]->z);
			alSourcei(soundsFiles[i]->sourceid, AL_LOOPING, AL_TRUE); // makes the sound continuously loop once initiated
		}

		alSourcei(mySine.sourceid, AL_LOOPING, AL_TRUE);
		
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
	alDeleteSources(1, &(mySine.sourceid));
	alDeleteBuffers(1, &(mySine.bufferid));
	delete[] mySine.samples;

	deleteSoundFiles(soundsFiles);
	freeContext(device, context);
	return 0;
}