#pragma once
#ifndef ALUTIL
#define ALUTIL
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <math.h>
#include <vector>

#include <AL/al.h>
#include <AL/alc.h>

#define SDL_MAIN_HANDLED
#include <SDL/SDL.h>

// WAV_loaders
bool isBigEndian();
int charToInt(char* buffer, int len);
char* loadWAV(std::string filename, int& channel, int& sampleRate, int& bps, int& size);

#pragma region structs
struct vec3 {
	float x, y, z;

	vec3() {} // default constructor, does not initialize vectors
	vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {} // vector initializer
	void set(float _x, float _y, float _z) { x = _x; y = _y; z = _z; }
};

struct Listener {
	vec3 f, up, pos, vel;

	Listener() {
		f.x = 0; f.y = 0; f.z = 1;
		up.x = 0; up.y = 1; up.z = 0;
		pos.x = 0; pos.y = 0; pos.z = 0;
		vel.x = 0; vel.y = 0; vel.z = 0;
	}
};

struct soundFile {
	std::string name;
	int channel, sampleRate, bps, size;
	char* wavFile;
	float x = 0, y = 0, z = 0; // sound position
	unsigned int sourceid, bufferid; // post-initialization

	soundFile(std::string _name) : name(_name) {}
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
void moveListener(vec3 position, Listener& player);
#pragma endregion prototypes

class ALUtilities
{

};

#endif