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

//destroys allocated OpenAL instances
void freeContext(ALCdevice* device, ALCcontext* context) {
	alcDestroyContext(context);
	alcCloseDevice(device);
}

//deletes the loaded elements of the given sound file, as well as its buffers
void deleteSoundFile(soundFile &sf){
	alDeleteSources(1, &(sf.sourceid));
	alDeleteBuffers(1, &(sf.bufferid));

	delete[] sf.wavFile;
}

//deletes all sound file instances
void deleteSoundFiles(std::vector<soundFile*> &soundFiles){
	for (int i = 0; i < soundFiles.size(); i++) {
		deleteSoundFile(*soundFiles[i]);
		delete soundFiles[i];
	}
	soundFiles.clear();
}

//creates an SDL instance and sets up an interactable window element
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

std::vector<soundFile*> createSounds(std::vector<std::string>& files)
{
	std::vector<soundFile*> allSoundFiles;

	for (int i = 0; i < files.size(); i++) {
		soundFile* newSound = new soundFile(files[i]);
		*newSound = createSoundFile(files[i]);
		initAudioSource(*newSound);

		allSoundFiles.push_back(newSound);
	}

	return allSoundFiles;
}

/**
 * moves sound sources on the x-z plane according to the keyboard input
 */
void keyInput(bool& running, float speed, float sensitivity, Listener& player, std::vector<soundFile*> &sounds) {
	SDL_Event event;
	glm::vec3 position;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			running = false;
			break;
			//
		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
			//stop all sounds
			case SDLK_QUOTEDBL:
				for (int i = 0; i < sounds.size(); i++)
					alSourceStop(sounds[i]->sourceid);
				break;
			//play a sound associated with this keybind
			case SDLK_1:
				alSourcePlay(sounds[0]->sourceid);
				//can also use Stop Pause Rewind instead of Play
				break;
			case SDLK_2:
				alSourcePlay(sounds[1]->sourceid);
				break;
			case SDLK_3:
				alSourcePlay(sounds[2]->sourceid);
				break;
			case SDLK_4:
				alSourcePlay(sounds[3]->sourceid);
				break;
			//////////////////////////////////////
			//misc. keybinds
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
		position = glm::vec3(player.pos.x - speed, player.pos.y, player.pos.z); // absolute movement (w.r.t. world space)
		//position.set(player.pos.x - (player.f.x * speed), player.pos.y, player.pos.z); // relative movement (w.r.t. listener)
		moveListener(position, player);
	}
	if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_RIGHT] || SDL_GetKeyboardState(NULL)[SDL_SCANCODE_D]) {
		position = glm::vec3(player.pos.x + speed, player.pos.y, player.pos.z);// absolute movement (w.r.t. world space)
		//position.set(player.pos.x + (player.f.x * speed), player.pos.y, player.pos.z); // relative movement (w.r.t. listener)
		moveListener(position, player);
	}
	if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_DOWN] || SDL_GetKeyboardState(NULL)[SDL_SCANCODE_S]) {
		position = glm::vec3(player.pos.x, player.pos.y, player.pos.z - speed);// absolute movement (w.r.t. world space)
		//position.set(player.pos.x, player.pos.y, player.pos.z - (player.f.z * speed)); // relative movement (w.r.t. listener)
		moveListener(position, player);
	}
	if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_UP] || SDL_GetKeyboardState(NULL)[SDL_SCANCODE_W]) {
		position = glm::vec3(player.pos.x, player.pos.y, player.pos.z + speed);// absolute movement (w.r.t. world space)
		//position.set(player.pos.x, player.pos.y, player.pos.z + (player.f.z * speed)); // relative movement (w.r.t. listener)
		moveListener(position, player);
	}
	printf("\nposition of player: %f, %f, %f\nforward vector: %f, %f, %f\n", player.pos.x, player.pos.y, player.pos.z,
		(player.f.x - player.pos.x), (player.f.y - player.pos.y), (player.f.z - player.pos.z));
}

void setListenerAngle(float angle, Listener& player) {
	//move player to origin
	glm::vec3 position(0, 0, 0);
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
	position = glm::vec3(xOffset, yOffset, zOffset);
	moveListener(position, player);
}

/**
* Moves the listener to the given location
* Updates lookAt vector of the listener
*/
void moveListener(glm::vec3 position, Listener& player) {

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



// Intersects the given ray with all spheres in the scene
// and updates the given HitInfo using the information of the sphere
// that first intersects with the ray.
// Returns true if an intersection is found.
bool IntersectRaySphere(HitInfo& hit, Ray ray) {
	hit.t = 1e30;
	bool foundHit = false;

	//fucky wucky solution
	const int NUM_SPHERES = 1;
	Sphere spheres[NUM_SPHERES]; //number of objects
	Sphere newSphere;
	spheres[0] = newSphere;

	for (int i = 0; i < NUM_SPHERES; ++i) {
		Sphere sphere = spheres[i];

		// Test for ray-sphere intersection; b^2 - 4ac
		float discriminant = pow(glm::dot(ray.getDir(), (ray.getOrig() - sphere.center)), 2.0) -
			(glm::dot(ray.getDir(), ray.getDir()) * (glm::dot((ray.getOrig() - sphere.center), (ray.getOrig() - sphere.center)) - pow(sphere.radius, 2.0)));

		if (discriminant >= 0.0) { // hit found
			// find the t value of closet ray-sphere intersection
			float t0;
			if (glm::distance(ray.getOrig(), sphere.center) > sphere.radius) //finds hit facing you (use if outside sphere)
				t0 = (-(glm::dot(ray.getDir(), (ray.getOrig() - sphere.center))) - sqrt(discriminant)) / (glm::dot(ray.getDir(), ray.getDir()));
			else //finds hit facing from you (use if inside sphere)
				t0 = (-(glm::dot(ray.getDir(), (ray.getOrig() - sphere.center))) + sqrt(discriminant)) / (glm::dot(ray.getDir(), ray.getDir()));

			// If intersection is found, update the given HitInfo
			if (t0 > 0.0 && t0 <= hit.t) {
				foundHit = true;

				hit.t = t0;
				hit.position = ray.getOrig() + (ray.getDir() * t0);
				hit.normal = normalize((hit.position - sphere.center) / sphere.radius);

				hit.mtl = sphere.mtl;
			}
		}
	}
	return foundHit;
}


//TODO: modify this. It only computes light rendering at a point. But we can hear places we cannot see. We need to return sound sources at all intersection points
// Given a ray, returns the sound where the ray intersects a sphere.
// If the ray does not hit a sphere, returns nothing.
std::vector<reflectInfo> RayTracer(Ray ray) {
	HitInfo hit;
	std::vector<reflectInfo> reflectedSources; // stores all sound source instances, the returned ones will play one bit of sound
	int MAX_BOUNCES = 3;
	int bounceLimit = 3;

	if (IntersectRaySphere(hit, ray)) {
		glm::vec3 view = normalize(-ray.getDir());
		//vec3 clr = Shade( hit.mtl, hit.position, hit.normal, view );
		reflectInfo newReflectedSound(hit);
		reflectedSources.push_back(newReflectedSound);

		// Compute reflections
		for (int bounce = 0; bounce < MAX_BOUNCES; ++bounce) {
			if (bounce >= bounceLimit) break;

			Ray r;	// this is the reflection ray
			HitInfo h;	// reflection hit info

			// Initialize the reflection ray
			r.setDir(normalize(ray.getDir()) - 2 * dot(normalize(ray.getDir()), hit.normal) * hit.normal);
			r.setOrig(hit.position + r.getDir() * 0.0001f);


			if (IntersectRaySphere(h, r)) {
				// TODO: Hit found, so make a sound at the hit point (not implemented)
				// clr += vec3(h.normal); // Test reflection intersections
				// clr += vec3(1.0, 0.0, 0.0);
				//clr += Shade(h.mtl, h.position, h.normal, view);
				reflectInfo newReflectedSound(h, reflectedSources.back().totalAbsorbed); // TODO: this needs to be filled with sound source information as soon as you know it! (will be done when we start detecting for real sound sources)
				reflectedSources.push_back(newReflectedSound);

				// Update the loop variables for tracing the next reflection ray
				hit = h;
				ray = r;
			}
			else {
				// TODO: The refleciton ray did not intersect with anything,
				// so we are using the environment sound (not implemented)
				//clr += k_s * textureCube(envMap, r.dir.xzy).rgb;
				break;	// no more reflections
			}
		}

		//TODO: How does this check if the thing hit a sound/light source? I don't get this part
		//			Is it assuming the camera is the source?
		//TODO: This needs to be reversed if the rays are thrown from the camera! (camera is not the source)
		return reflectedSources;	// TODO: return the accumulated sound, including the reflections. Create sound sources at these locations
	}
	else
		//returns empty source buffer
		return reflectedSources;	// TODO: return the environment sound
}

float get_random() {
	static std::default_random_engine e;
	static std::uniform_real_distribution<> dis(-1, 1); // range [0, 1)
	return dis(e);
}

Ray GetRandomRay(Listener listener) {
	//Ray newRay(listener.pos, glm::vec3(get_random(), get_random(), get_random()));
	return Ray(listener.pos, glm::vec3(get_random(), get_random(), get_random()));
}