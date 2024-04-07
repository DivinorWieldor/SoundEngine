#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <Windows.h>

#include <AL/al.h>
#include <AL/alc.h>

#define SDL_MAIN_HANDLED
#include <SDL/SDL.h>

using namespace std;

struct soundFile {
	string name;
	int channel, sampleRate, bps, size;
	char* wavFile;

	soundFile(string _name) : name(_name){}
};

//wav file handling
bool isBigEndian() {
	int a = 1;

	//if 1, then little endian
	bool firstByte = ((char*)&a)[0];
	return !firstByte;
}

int charToInt(char* buffer, int len) {
	int a = 0;

	if(!isBigEndian()){
		for (int i = 0; i < len; i++) {
			((char*)&a)[i] = buffer[i];
		}
	}
	else {
		for (int i = 0; i < len; i++) {
			((char*)&a)[3-i] = buffer[i];
		}
	}

	return a;
}

char* loadWAV(string filename, int& channel, int& sampleRate, int& bps, int& size) {
	ifstream in(filename, ios::binary);
	char buffer[4];

	in.read(buffer, 4);
	if (strncmp(buffer, "RIFF", 4) != 0) {
		cout << "WAV file does not use RIFF!\n\tExpected 'RIFF', got " << buffer[0] << buffer[1] << buffer[2] << buffer[3] << endl;
	}

	in.read(buffer, 4);
	in.read(buffer, 4);//should read "WAVE"
	in.read(buffer, 4);//should read "fmt"
	in.read(buffer, 4);//should read "16"
	in.read(buffer, 2);//should read "1"
	
	in.read(buffer, 2);//channel info
	channel = charToInt(buffer, 2);

	in.read(buffer, 4);//sample rate info
	sampleRate = charToInt(buffer, 4);

	in.read(buffer, 4);
	in.read(buffer, 2);

	in.read(buffer, 2);//bits per sample info
	bps = charToInt(buffer, 2);

	in.read(buffer, 4);//data

	in.read(buffer, 4);//data size
	size = charToInt(buffer, 4);

	char* data = new char[size];//sound data
	in.read(data, size);

	cout << filename << " successfully loaded" << endl;
	//cout << channel << " " << sampleRate << " " << bps << " " << size << endl;
	return data;
}

//prototypes
void deviceExists(ALCdevice* device);
void contextExists(ALCcontext* context);
void freeContext(ALCdevice* device, ALCcontext* context);
void getAudioFormat(int channel, int bps, unsigned int& format);

int main() {
	soundFile chirping("chirp.wav");

	chirping.wavFile = loadWAV(chirping.name, chirping.channel, chirping.sampleRate, chirping.bps, chirping.size);

	//set up openAL context
	ALCdevice* device = alcOpenDevice(NULL);
	deviceExists(device);

	ALCcontext* context = alcCreateContext(device, NULL);
	contextExists(context);
	alcMakeContextCurrent(context);

	//create sound buffer
	unsigned int bufferid, format;
	alGenBuffers(1, &bufferid);

	getAudioFormat(chirping.channel, chirping.bps, format);
	alBufferData(bufferid, format, chirping.wavFile, chirping.size, chirping.sampleRate);

	//create a sound source
	unsigned int sourceid;
	alGenSources(1, &sourceid); //create 1 source into sourceid
	alSourcei(sourceid, AL_BUFFER, bufferid); // attach ONE(i) buffer to source

	//play sound
	alSourcePlay(sourceid);
	SDL_Delay(5000); //give source time to play (wait 5 seconds before program close)



	//program termination
	alDeleteSources(1, &sourceid);
	alDeleteBuffers(1, &bufferid);

	delete[] chirping.wavFile;
	freeContext(device, context);
	return 0;
}

// prototype implementations

//setup and ending
void deviceExists(ALCdevice* device) {
	if (device == NULL) {
		cout << "sound card cannot be opened" << endl;
		exit(100);
	}
}
void contextExists(ALCcontext* context) {
	if (context == NULL) {
		cout << "context cannot be created" << endl;
		exit(101);
	}
}
void freeContext(ALCdevice* device, ALCcontext* context) {
	alcDestroyContext(context);
	alcCloseDevice(device);
}

// functionals
void getAudioFormat(int channel, int bps, unsigned int& format) {
	if (channel == 1) {
		//mono
		if (bps == 8) {
			format = AL_FORMAT_MONO8;
		}
		else {//bps 16
			format = AL_FORMAT_MONO16;
		}
	}
	else {
		//stereo
		if (bps == 8) {
			format = AL_FORMAT_STEREO8;
		}
		else {//bps 16
			format = AL_FORMAT_STEREO16;
		}
	}
}