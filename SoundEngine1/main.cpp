#include <iostream>
#include <fstream>
#include <cstring>
#include <string>

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
void ALSetup(ALCdevice* &device, ALCcontext* &context);
void SDLSetup(SDL_Window*& window, SDL_Surface*& screenSurface, int height, int width);
void freeContext(ALCdevice* device, ALCcontext* context);
void getAudioFormat(int channel, int bps, unsigned int& format);

int main() {
	soundFile chirping("chirp.wav");

	chirping.wavFile = loadWAV(chirping.name, chirping.channel, chirping.sampleRate, chirping.bps, chirping.size);

	//set up openAL context
	ALCdevice* device;
	ALCcontext* context;
	ALSetup(device, context);

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
	//SDL_SetVideoMode has been depricated: https://stackoverflow.com/q/28400401/16858784
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window* window;
	SDL_Surface* screenSurface;
	SDLSetup(window, screenSurface, 480, 640);

	SDL_Event event;
	Uint64 start;
	bool running = true;
	bool b[4] = { 0,0,0,0 };
	float x = 0, z = 0, speed = 0.2; // sound position

	while (running) {
		start = SDL_GetTicks64();
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				running = false;
				break;
			//
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
				case SDLK_UP:
					b[0] = true;
					break;
				case SDLK_RIGHT:
					b[1] = true;
					break;
				case SDLK_DOWN:
					b[2] = true;
					break;
				case SDLK_LEFT:
					b[3] = true;
					break;
				case SDLK_SPACE:
					alSourcePlay(sourceid);
					//can also use Stop Pause Rewind instead of Play
					break;
				case SDLK_ESCAPE:
					running = false;
					break;
				}
				break;
			//
			case SDL_KEYUP:
				switch (event.key.keysym.sym) {
				case SDLK_UP:
					b[0] = false;
					break;
				case SDLK_RIGHT:
					b[1] = false;
					break;
				case SDLK_DOWN:
					b[2] = false;
					break;
				case SDLK_LEFT:
					b[3] = false;
					break;
				}
				break;
			}
		}

		if (b[0]) z += speed;
		if (b[1]) x += speed;
		if (b[2]) z -= speed;
		if (b[3]) x -= speed;

		alSource3f(sourceid, AL_POSITION, x, 0, z);
		//alSourcei(sourceid, AL_LOOPING, AL_TRUE); // makes the sound continuously loop once initiated
		
		//position of listener
		//          forward, up
		float f[] = {1,0,0, 0,1,0};
		alListenerfv(AL_ORIENTATION, f);
		
		if (1000 / 30 > SDL_GetTicks64() - start) {
			SDL_Delay(1000 / 30 - (SDL_GetTicks64() - start));
		}
	}



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

/**
 * default device is set to context
 * context is made current
 */
void ALSetup(ALCdevice* &device, ALCcontext* &context) {
	device = alcOpenDevice(NULL);
	deviceExists(device);

	context = alcCreateContext(device, NULL);
	contextExists(context);
	alcMakeContextCurrent(context);
}

void SDLSetup(SDL_Window* &window, SDL_Surface* &screenSurface, int height, int width) {
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
		return exit(111);
	}

	window = NULL;
	screenSurface = NULL;

	window = SDL_CreateWindow("Sphere Rendering",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		width, height, SDL_WINDOW_SHOWN);

	if (window == NULL) {
		fprintf(stderr, "Window could not be created: %s\n", SDL_GetError());
		exit(112);
	}

	screenSurface = SDL_GetWindowSurface(window);
	if (!screenSurface) {
		fprintf(stderr, "Screen surface could not be created: %s\n", SDL_GetError());
		SDL_Quit();
		exit(113);
	}
}

/**
 * sets audio format
 * decides between MONO vs STEREO channel and 8 vs 16 bps
 */
void getAudioFormat(int channel, int bps, unsigned int& format) {
	if (channel == 1) {// directional. Better for bimodal sound
		if (bps == 8) {
			format = AL_FORMAT_MONO8;
		}
		else {//bps 16
			format = AL_FORMAT_MONO16;
		}
	}
	else {// no directionality. Better for background sound
		if (bps == 8) {
			format = AL_FORMAT_STEREO8;
		}
		else {//bps 16
			format = AL_FORMAT_STEREO16;
		}
	}
}