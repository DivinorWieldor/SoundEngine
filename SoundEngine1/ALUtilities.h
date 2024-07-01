#pragma once
#ifndef ALUTIL
#define ALUTIL
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <math.h>
#include <vector>
#include <random>

#include <AL/al.h>
#include <AL/alc.h>

#include <glm.hpp>

#define SDL_MAIN_HANDLED
#include <SDL/SDL.h>

// WAV_loaders
bool isBigEndian();
int charToInt(char* buffer, int len);
char* loadWAV(std::string filename, int& channel, int& sampleRate, int& bps, int& size);

#pragma region structs
struct Listener {
	glm::vec3 f, up, pos, vel;

	Listener() {
		f = glm::vec3(0, 0, 1);
		up = glm::vec3(0, 1, 0);
		pos = glm::vec3(0, 0, 0);
		vel = glm::vec3(0, 0, 0);
	}
};

struct soundFile {
	std::string name;
	int channel, sampleRate, bps, size;
	char* wavFile;
	glm::vec3 pos = glm::vec3(0, 0, 0); // sound position
	unsigned int sourceid, bufferid; // post-initialization

	soundFile(){} // default constructor
	soundFile(std::string _name) : name(_name) {}
};

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

	int getState() {
		int State;
		alGetSourcei(sourceid, AL_SOURCE_STATE, &State);
		return State;
	}

	sineW(float _freq = 440, int _seconds = 1, unsigned int _sample_rate = 22050, bool generateBuffer = true)
		: freq(_freq), seconds(_seconds), sample_rate(_sample_rate)
	{
		alGenBuffers(1, &bufferid);
		alGenSources(1, &sourceid);

		buf_size = seconds * sample_rate;
		//user may want to not create this now
		if(generateBuffer){
			samples = new short[buf_size];
			//populate sample buffer
			for (int i = 0; i < buf_size; ++i)
				//samples[i] = 32768 * 2 * (i * freq - int((i * freq) + 0.5)) / sample_rate;		//sawtooth - does not work
				//samples[i] = 32768 * (2/M_PI) * asin(sin((2 * M_PI * freq * i) / sample_rate));	//triangle
				//samples[i] = 32768 * sgn(cos((2 * M_PI * freq * i) / sample_rate));				//square
				samples[i] = 32768 * cos((2 * M_PI * freq * i) / sample_rate);					//sin or cos
			// 32760 because we use mono16, and 16 bits is 32768 (ignoring last digit cuz lazy)
			samples[0] = samples[buf_size - 1]; //i=0 causes issues, set it to one previous

			/* Download buffer to OpenAL ---- don't do this immediately, user may want to do it later! */
			alBufferData(bufferid, AL_FORMAT_MONO16, samples, buf_size, sample_rate);
		}
		//alSourcei(sourceid, AL_BUFFER, bufferid);
	}

	short* sineSegment(float seconds, int segmentStart, unsigned int _sample_rate){
		short* samplesSegment = new short[int(seconds * _sample_rate)];

		for (int i = 0; i < int(seconds * _sample_rate); ++i)
			samplesSegment[i] = 32768 * cos((2 * M_PI * freq * (int(seconds * _sample_rate) +i)) / sample_rate);	//sin or cos
		samplesSegment[0] = samplesSegment[int(seconds * _sample_rate) - 1]; //i=0 causes issues, set it to one previous
	
		return samplesSegment;
	}

	/*~sineW() {
		alDeleteSources(1, &sourceid);
		alDeleteBuffers(1, &bufferid);
		delete[] samples;
	}*/
};
#pragma endregion structs

#pragma region prototypes
//device set up and termination
void deviceExists(ALCdevice* device);
void contextExists(ALCcontext* context);
void ALSetup(ALCdevice*& device, ALCcontext*& context);
void SDLSetup(SDL_Window*& window, SDL_Surface*& screenSurface, int height, int width);
void freeContext(ALCdevice* device, ALCcontext* context);
void deleteSoundFile(soundFile &sf);
void deleteSoundFiles(std::vector<soundFile*> &soundFiles);

//utilities
void getAudioFormat(int channel, int bps, unsigned int& format);
void initAudioSource(soundFile& sFile);
void keyInput(bool& running, float speed, float sensitivity, Listener& player, std::vector<soundFile*>& sounds);
soundFile createSoundFile(std::string fileName);
std::vector<soundFile*> createSounds(std::vector<std::string>& files);
void setListenerAngle(float angle, Listener& player);
void moveListener(glm::vec3 position, Listener& player);
#pragma endregion prototypes

class ALUtilities
{

};

/////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////  Ray Tracing Code  //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

class Ray{
private:
	glm::vec3 origin;    // M
	glm::vec3 direction; // n, must be a unit vector
public:
	Ray() {};
	Ray(glm::vec3 orig, glm::vec3 dir) : origin(orig), direction(dir) {}
	//~Ray() {}
	const glm::vec3& getOrig() const { return origin; }
	const glm::vec3& getDir() const { return direction; } // if you don't init get functions like this, accessing this method from const reference objects would be impossible
	void setOrig(glm::vec3 _origin) { origin = _origin; }
	void setDir(glm::vec3 _direction) { direction = _direction; }

	// calculates M + nx -- returns vector length between origin and a point
	glm::vec3 at(float x) const	{ return (origin + (direction * x)); }
};

struct Material {
	float	notAbsorbed;		// absorption modifier (ranges from 0.0 to 1.0)
	bool	isSource = false;	//is this a sound source?
	//may also have a scatter modifier (how much randomization to add to reflection) -> not sure if necessary

	Material(float _absorbModifier = 0.4) : notAbsorbed(_absorbModifier) {}

	float soundDampenPercent() { return notAbsorbed; }
};

struct Sphere {
	glm::vec3	center;
	float		radius;
	Material	mtl;

	Sphere() : center(glm::vec3(0, 0, 0)), radius(2) {};
};

//struct Light { //for me this is sound
//	vec3 position;
//	vec3 intensity;
//};

struct HitInfo {
	float		t; //closest hit distance
	glm::vec3	position;
	glm::vec3	normal;
	Material	mtl;
};

struct reflectInfo {
	sineW sound = sineW(440, 1, 22050, false);
	HitInfo hit;
	float totalAbsorbed; // multiply with original sound source to get dampened sound (reduced amplitude)

	reflectInfo(HitInfo _hit) : hit(_hit) { totalAbsorbed = hit.mtl.soundDampenPercent(); }
	reflectInfo(HitInfo _hit, float prevDampen) : hit(_hit) { totalAbsorbed = prevDampen * _hit.mtl.soundDampenPercent(); }
};

//const int NUM_SPHERES = 1; //ideally we want this to be however many surfaces there are in the scene
//int MAX_BOUNCES = 3;
//Sphere spheres[NUM_SPHERES]; //number of objects
//uniform Light  lights[NUM_LIGHTS]; //number of sound sources
//uniform samplerCube envMap; //pretty sure this is just a cube map
//int bounceLimit = 3; //don't understand why this is needed?

//Reverses the calculated order of reflection absorptions
void ReverseAbsorptionOrder(std::vector<reflectInfo>& reflectedSources);

// Intersects the given ray with all spheres in the scene
// and updates the given HitInfo using the information of the sphere
// that first intersects with the ray.
// Returns true if an intersection is found.
bool IntersectRaySphere(HitInfo& hit, Ray ray);


//TODO: modify this. It only computes light rendering at a point. But we can hear places we cannot see. We need to return sound sources at all intersection points
// Given a ray, returns the sound where the ray intersects a sphere.
// If the ray does not hit a sphere, returns nothing.
std::vector<reflectInfo> RayTracer(Ray ray);

float get_random();

Ray GetRandomRay(Listener listener);
#endif