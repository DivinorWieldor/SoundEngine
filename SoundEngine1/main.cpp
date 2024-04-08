#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <math.h>

#include <AL/al.h>
#include <AL/alc.h>

#define SDL_MAIN_HANDLED
#include <SDL/SDL.h>

using namespace std;

struct vec3 {
	float x, y, z;
};

struct Listener {
	vec3 f, up;

	Listener() {
		f.x = 0; f.y = 0; f.z = 1;
		up.x = 0; up.y = 1; up.z = 0;
	}
};

struct soundFile {
	string name;
	int channel, sampleRate, bps, size;
	char* wavFile;
	float x = 0, y = 0, z = 0; // sound position
	unsigned int sourceid, bufferid; // post-initialization

	soundFile(string _name) : name(_name) {}
};

#pragma region WAV_loaders
bool isBigEndian() {
	int a = 1;

	//if 1, then little endian
	bool firstByte = ((char*)&a)[0];
	return !firstByte;
}

int charToInt(char* buffer, int len) {
	int a = 0;

	if (!isBigEndian()) {
		for (int i = 0; i < len; i++) {
			((char*)&a)[i] = buffer[i];
		}
	}
	else {
		for (int i = 0; i < len; i++) {
			((char*)&a)[3 - i] = buffer[i];
		}
	}

	return a;
}

/**
 * detects the properties of a WAV file and loads from file name
 */
char* loadWAV(string filename, int& channel, int& sampleRate, int& bps, int& size) {
	ifstream in(filename, ios::binary);
	char buffer[4];

	in.read(buffer, 4);
	if (strncmp(buffer, "RIFF", 4) != 0) {
		std::cout << "WAV file does not use RIFF!\n\tExpected 'RIFF', got " << buffer[0] << buffer[1] << buffer[2] << buffer[3] << std::endl;
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

	printf("%s successfully loaded\nchannel: %i; sample rate: %i; bits per second: %i, audio size: %i\n", filename.c_str(), channel, sampleRate, bps, size);
	return data;
}
#pragma endregion WAV_loaders

#pragma region prototypes
//prototypes
//device set up
void deviceExists(ALCdevice* device);
void contextExists(ALCcontext* context);
void ALSetup(ALCdevice*& device, ALCcontext*& context);
void SDLSetup(SDL_Window*& window, SDL_Surface*& screenSurface, int height, int width);
void freeContext(ALCdevice* device, ALCcontext* context);

//utilities
void getAudioFormat(int channel, int bps, unsigned int& format);
void keyInput(bool& running, float speed, float sensitivity, Listener& player, soundFile &sound);
soundFile createSoundFile(std::string fileName);
void setListenerAngle(float angle, Listener& player);
#pragma endregion prototypes

int main() {
	soundFile chirping = createSoundFile("chirp.wav");

	//set up openAL context
	ALCdevice* device;
	ALCcontext* context;
	ALSetup(device, context);

	//create sound buffer
	unsigned int format;
	alGenBuffers(1, &(chirping.bufferid));

	getAudioFormat(chirping.channel, chirping.bps, format);
	alBufferData(chirping.bufferid, format, chirping.wavFile, chirping.size, chirping.sampleRate);

	//create a sound source
	alGenSources(1, &(chirping.sourceid)); //create 1 source into sourceid
	alSourcei(chirping.sourceid, AL_BUFFER, chirping.bufferid); // attach ONE(i) buffer to source

	//play sound
	//SDL_SetVideoMode has been depricated: https://stackoverflow.com/q/28400401/16858784
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window* window;
	SDL_Surface* screenSurface;
	SDLSetup(window, screenSurface, 480, 640);
	SDL_SetRelativeMouseMode(SDL_TRUE);

	Uint64 start;
	bool running = true;
	float speed = 0.2;
	float sensitivity = 0.005; //in degrees
	Listener me;

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
	alDeleteSources(1, &(chirping.sourceid));
	alDeleteBuffers(1, &(chirping.bufferid));

	delete[] chirping.wavFile;
	freeContext(device, context);
	return 0;
}

//**************************************************************
//************** prototype implementations *********************
//**************************************************************

//setup and ending
void deviceExists(ALCdevice* device) {
	if (device == NULL) {
		cout << "sound card cannot be opened" << std::endl;
		exit(100);
	}
}

/**
 * checks if the context item was intialized correctly
 * terminates program if not
 */
void contextExists(ALCcontext* context) {
	if (context == NULL) {
		cout << "context cannot be created" << std::endl;
		exit(101);
	}
}

/**
 * default device is set to context
 * context is made current
 */
void ALSetup(ALCdevice*& device, ALCcontext*& context) {
	device = alcOpenDevice(NULL);
	deviceExists(device);

	context = alcCreateContext(device, NULL);
	contextExists(context);
	alcMakeContextCurrent(context);
}

void freeContext(ALCdevice* device, ALCcontext* context) {
	alcDestroyContext(context);
	alcCloseDevice(device);
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
 * detects and sets audio format
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

soundFile createSoundFile(std::string fileName)
{
	soundFile newFile(fileName);

	newFile.wavFile = loadWAV(newFile.name, newFile.channel, newFile.sampleRate, newFile.bps, newFile.size);

	return newFile;
}

/**
 * moves sound sources on the x-z plane according to the keyboard input
 */
void keyInput(bool& running, float speed, float sensitivity, Listener &player, soundFile &sound) {
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			running = false;
			break;
			//
		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
			case SDLK_UP:
				sound.z += speed;
				break;
			case SDLK_RIGHT:
				sound.x += speed;
				break;
			case SDLK_DOWN:
				sound.z -= speed;
				break;
			case SDLK_LEFT:
				sound.x -= speed;
				break;
			case SDLK_SPACE:
				alSourcePlay(sound.sourceid);
				//can also use Stop Pause Rewind instead of Play
				break;
			case SDLK_ESCAPE:
				running = false;
				break;
			case SDLK_c:
				SDL_SetRelativeMouseMode(SDL_FALSE);
				break;
			}
			break;
		case SDL_MOUSEMOTION:
			/**
			* left is negative, so it will turn right without modifications
			* due to this, the degree is multiplied by -1 here
			*/
			setListenerAngle(event.motion.xrel * sensitivity * -1, player);
			printf("forward vector: %f, %f, %f\n", player.f.x, player.f.y, player.f.z);
			//cout << "mouse Motion output: " << event.motion.xrel << " " << event.motion.yrel << endl;
			break;
			/*case SDL_KEYUP:
				switch (event.key.keysym.sym) {
				case SDLK_UP:
					movingDirection[0] = false;
					break;
				case SDLK_RIGHT:
					movingDirection[1] = false;
					break;
				case SDLK_DOWN:
					movingDirection[2] = false;
					break;
				case SDLK_LEFT:
					movingDirection[3] = false;
					break;
				}
				break;*/
		}
	}
}

void setListenerAngle(float angle, Listener& player){
	float sinAngle = sin(angle);
	float cosAngle = cos(angle);

	player.f.x = (player.f.x * cosAngle) - (player.f.z * sinAngle);
	player.f.z = (player.f.x * sinAngle) + (player.f.z * cosAngle);
}