#include <cstring>
#include <string>
#include <vector>
#include <math.h>
#include <windows.h>
#include <algorithm>

#include <AL/al.h>
#include <AL/alc.h>

#include <glm.hpp>

#define SDL_MAIN_HANDLED
#include <SDL/SDL.h>
#include "ALUtilities.h"

using namespace std;

int main() {
	//set up openAL context
	ALCdevice* device;
	ALCcontext* context;
	ALSetup(device, context);

	//set up audio sources
	//set up sources loaded from files
	//each sound is assigned to keys [1-9]. Press ["] to make them all stop
	std::vector<std::string> soundFiles({	"./sounds/chirp.wav",
											"./sounds/sine.wav",
											"./sounds/sine_beeping.wav",
											"./sounds/whitenoise.wav" 
										});
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
	//Sphere newSphere;
	//spheres[0] = newSphere;
	int rayCount = 10;

	//set global volume
	float volume = 1;
	alListenerf(AL_GAIN, volume); //appears to only accept values between (0,1)
	//alSourcef(currentSourceID, AL_GAIN, newVolume); //to set volume of a source

	//DONE: try to load in sound one unit at a time
	//TODO: use this to do raytracing
	//			- Draw rays. If they hit a surface, note it, calulate absorption, and reflect as many times as needed
	//			- For every reflection, calculate total absorption and play a sound source at that location (sound * total absorption)
	//REQUIREMENTS: - Reflection getter (from incoming angle + normal vector of surface): Returns the reflected ray's vector
	//				- Ray Thrower (from Listener position, throws N rays everywhere): Returns a list of ray vectors
	//				- HitPos (from ray vector + all surfaces): Returns which surface was hit and where
	//				- CollisionsGetter: Combines all of these. Returns a list of all collisions' locations and the attenuation at that location
	//				- SoundSourceMaker (from soundBit, attenuation score, and location): Creates a new sound source at the collision spot
	//																					Sources must be deleted as soon as they finish playing!!!!
	//																					Check in like 2-3 frames --> causes noticable tearing
	
	sineW mySine;
	//alBufferData(mySine.bufferid, AL_FORMAT_MONO16, mySine.samples, mySine.buf_size, mySine.sample_rate);

	//testing on playing sound bits :P
	float segmentIncrement = 0;
	float segmentLen = 0.2;
	short* soundSegment = mySine.sineSegment(segmentLen, 0, mySine.sample_rate);
	alBufferData(mySine.bufferid, AL_FORMAT_MONO16, soundSegment, int(segmentLen * mySine.sample_rate), mySine.sample_rate);

	alSourcei(mySine.sourceid, AL_BUFFER, mySine.bufferid);
	alSourcePlay(mySine.sourceid);

	//Ray ray;
	std::vector<reflectInfo> reflectedRays;
	std::vector<reflectInfo> allReflections;

	/**
	* Idea: do not play one bit!
	* play the entire array but for one frame
	* Then move the buffer elements left by one, calculate the rays, and play the buffer for one frame again
	*/

	while (running) {
		start = SDL_GetTicks64();
		
		/*
		#pragma region RayTracing
		//compute all valid rays and reflections
		for (int i = 0; i < rayCount; i++) {
			reflectedRays = RayTracer(GetRandomRay(me)); //compute valid reflections of one ray
			allReflections.insert(allReflections.end(), reflectedRays.begin(), reflectedRays.end()); //bunch up all reflections
			reflectedRays.clear();
		}
		//TODO: create sounds at these locations. At the end of the while loop, delete them all. (stored in allReflections)
		//cout << "--------------- size: " << allReflections.size() << " ----------------" << endl;
		for (int i = 0; i < allReflections.size(); i++){
			//if(allReflections[i].sound.getState() == AL_STOPPED || allReflections[i].sound.getState() == AL_INITIAL){
				//copy_n(mySine.samples + sineSize, sampleSize, samplesInstance);
				//samplesInstance[0] = mySine.samples[sineSize] * allReflections[i].totalAbsorbed;
				//samplesInstance[1] = mySine.samples[sineSize + 1] * allReflections[i].totalAbsorbed;

				alBufferData(allReflections[i].sound.bufferid, AL_FORMAT_MONO16, samplesInstance, sampleSize, sampleSize);
				alSourcef(allReflections[i].sound.sourceid, AL_GAIN, allReflections[i].totalAbsorbed); //to set volume of a source
				alSourcei(allReflections[i].sound.sourceid, AL_BUFFER, allReflections[i].sound.bufferid);
				alSource3f(allReflections[i].sound.sourceid, AL_POSITION, allReflections[i].hit.position.x, allReflections[i].hit.position.y, allReflections[i].hit.position.z);
				alSourcePlay(allReflections[i].sound.sourceid);
			//}
		}
		#pragma endregion RayTracing
		*/

		//process key inputs
		keyInput(running, speed, sensitivity, me, soundsFiles);

		//position of sound source for sounds loaded from file
		for (int i = 0; i < soundsFiles.size(); i++) {
			alSource3f(soundsFiles[i]->sourceid, AL_POSITION, soundsFiles[i]->pos.x, soundsFiles[i]->pos.y, soundsFiles[i]->pos.z);
			alSourcei(soundsFiles[i]->sourceid, AL_LOOPING, AL_TRUE); // makes the sound continuously loop once initiated
		}

		// This attempts to play one bit of the sine wave
		// While it technically works, the process can cause some audio tearing
		// Overall, it works as well as playing the audio non-stop
		#pragma region playBit
		if (mySine.getState() == AL_STOPPED) {
			segmentIncrement += segmentLen;
			soundSegment = mySine.sineSegment(segmentLen, segmentIncrement, mySine.sample_rate);
			
			alDeleteSources(1, &(mySine.sourceid));
			alBufferData(mySine.bufferid, AL_FORMAT_MONO16, soundSegment, int(segmentLen * mySine.sample_rate), mySine.sample_rate);
			alGenSources(1, &(mySine.sourceid));
			alSourcei(mySine.sourceid, AL_BUFFER, mySine.bufferid);

			alSourcePlay(mySine.sourceid);
		}
		#pragma endregion playBit

		// activate if playing a whole array (!playBit)
		//alSourcei(mySine.sourceid, AL_LOOPING, AL_TRUE);
		
		//position of listener
		float playerVec[] = { me.f.x, me.f.y, me.f.z,//forward
							me.up.x, me.up.y, me.up.z };//up
		alListenerfv(AL_ORIENTATION, playerVec);
		
		//clean up sound sources - handle memory leaks
		for (int i = 0; i < allReflections.size(); i++) {
			alDeleteSources(1, &(allReflections[i].sound.sourceid));
			alDeleteBuffers(1, &(allReflections[i].sound.bufferid));
			delete[] allReflections[i].sound.samples;
		}
		allReflections.clear(); //clean rays buffer
		
		//necessary for sound playing when moving
		if (1000 / 30 > SDL_GetTicks64() - start) {
			SDL_Delay(1000 / 30 - (SDL_GetTicks64() - start));
		}
	}

	//program termination
	alDeleteSources(1, &(mySine.sourceid));
	alDeleteBuffers(1, &(mySine.bufferid));
	delete[] mySine.samples;
	delete[] soundSegment;

	deleteSoundFiles(soundsFiles);
	freeContext(device, context);
	return 0;
}