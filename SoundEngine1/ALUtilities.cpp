#include "ALUtilities.h"


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
char* loadWAV(std::string filename, int& channel, int& sampleRate, int& bps, int& size) {
	std::ifstream in(filename, std::ios::binary);
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
//setup and ending
void deviceExists(ALCdevice* device) {
	if (device == NULL) {
		std::cout << "sound card cannot be opened" << std::endl;
		exit(100);
	}
}

/**
 * checks if the context item was intialized correctly
 * terminates program if not
 */
void contextExists(ALCcontext* context) {
	if (context == NULL) {
		std::cout << "context cannot be created" << std::endl;
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

void deleteSoundFile(soundFile &sf){
	alDeleteSources(1, &(sf.sourceid));
	alDeleteBuffers(1, &(sf.bufferid));

	delete[] sf.wavFile;
}

void SDLSetup(SDL_Window*& window, SDL_Surface*& screenSurface, int height, int width) {
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

void initAudioSource(soundFile& sFile) {
	//create sound buffer
	unsigned int format;
	alGenBuffers(1, &(sFile.bufferid));

	getAudioFormat(sFile.channel, sFile.bps, format);
	alBufferData(sFile.bufferid, format, sFile.wavFile, sFile.size, sFile.sampleRate);

	//create a sound source
	alGenSources(1, &(sFile.sourceid)); //create 1 source into sourceid
	alSourcei(sFile.sourceid, AL_BUFFER, sFile.bufferid); // attach ONE(i) buffer to source
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
void keyInput(bool& running, float speed, float sensitivity, Listener& player, soundFile& sound) {
	SDL_Event event;
	vec3 position;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			running = false;
			break;
			//
		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
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
			setListenerAngle(event.motion.xrel * sensitivity, player);
			printf("forward vector: %f, %f, %f\n", (player.f.x - player.pos.x), (player.f.y - player.pos.y), (player.f.z - player.pos.z));
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

	//simultaneous button press handler
	//reference for scan codes https://wiki.libsdl.org/SDL2/SDL_Scancode
	if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_LEFT] || SDL_GetKeyboardState(NULL)[SDL_SCANCODE_A]) {
		position.set(player.pos.x - speed, player.pos.y, player.pos.z); // absolute movement (w.r.t. world space)
		//position.set(player.pos.x - (player.f.x * speed), player.pos.y, player.pos.z); // relative movement (w.r.t. listener)
		moveListener(position, player);
	}
	if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_RIGHT] || SDL_GetKeyboardState(NULL)[SDL_SCANCODE_D]) {
		position.set(player.pos.x + speed, player.pos.y, player.pos.z);// absolute movement (w.r.t. world space)
		//position.set(player.pos.x + (player.f.x * speed), player.pos.y, player.pos.z); // relative movement (w.r.t. listener)
		moveListener(position, player);
	}
	if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_DOWN] || SDL_GetKeyboardState(NULL)[SDL_SCANCODE_S]) {
		position.set(player.pos.x, player.pos.y, player.pos.z - speed);// absolute movement (w.r.t. world space)
		//position.set(player.pos.x, player.pos.y, player.pos.z - (player.f.z * speed)); // relative movement (w.r.t. listener)
		moveListener(position, player);
	}
	if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_UP] || SDL_GetKeyboardState(NULL)[SDL_SCANCODE_W]) {
		position.set(player.pos.x, player.pos.y, player.pos.z + speed);// absolute movement (w.r.t. world space)
		//position.set(player.pos.x, player.pos.y, player.pos.z + (player.f.z * speed)); // relative movement (w.r.t. listener)
		moveListener(position, player);
	}
	printf("\nposition of player: %f, %f, %f\nforward vector: %f, %f, %f\n", player.pos.x, player.pos.y, player.pos.z,
		(player.f.x - player.pos.x), (player.f.y - player.pos.y), (player.f.z - player.pos.z));
}

void setListenerAngle(float angle, Listener& player) {
	//move player to origin
	vec3 position(0, 0, 0);
	float xOffset = player.pos.x;
	float yOffset = player.pos.y;
	float zOffset = player.pos.z;
	moveListener(position, player);

	//turn player
	float sinAngle = sin(angle);
	float cosAngle = cos(angle);
	float newX = (player.f.x * cosAngle) - (player.f.z * sinAngle);
	float newZ = (player.f.x * sinAngle) + (player.f.z * cosAngle);

	player.f.x = newX;
	player.f.z = newZ;

	//move player back to where it was
	position.set(xOffset, yOffset, zOffset);
	moveListener(position, player);
}

/**
* Moves the listener to the given location
* Updates lookAt vector of the listener
*/
void moveListener(vec3 position, Listener& player) {

	float xOffset = position.x - player.pos.x;
	float yOffset = position.y - player.pos.y;
	float zOffset = position.z - player.pos.z;

	player.pos.x = position.x;
	player.pos.y = position.y;
	player.pos.z = position.z;
	float playerPos[] = { player.pos.x, player.pos.y, player.pos.z };
	alListenerfv(AL_POSITION, playerPos);

	// Keep the listener facing the same direction by
	// moving the "look at" point by the offset values:
	player.f.x += xOffset;
	player.f.y += yOffset;
	player.f.z += zOffset;
	float playerLookAt[] = { player.f.x,  player.f.y,  player.f.z,//forward
							 player.up.x, player.up.y, player.up.z }; // up
	alListenerfv(AL_ORIENTATION, playerLookAt);
}
#pragma endregion prototypes