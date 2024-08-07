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


//Reverses the calculated order of reflection absorptions
void ReverseAbsorptionOrder(std::vector<reflectInfo> &reflectedSources) {
	float totalDampen = 1;

	for (int i = reflectedSources.size() - 1; i >= 0; i--) {
		totalDampen *= reflectedSources[i].hit.mtl.soundDampenPercent();
		reflectedSources[i].totalAbsorbed = totalDampen;
	}
}

// Intersects the given ray with all spheres in the scene
// and updates the given HitInfo using the information of the sphere
// that first intersects with the ray.
// Returns true if an intersection is found.
bool IntersectRaySphere(HitInfo& hit, Ray ray) {
	hit.t = 1e30;
	bool foundHit = false;

	//TODO: remove these from the class. These should not be here, keep them in main
	//fucky wucky solution
	std::vector<Sphere> spheres;

	spheres.push_back( Sphere(glm::vec3(3, 0, 0), 2) );
	spheres.push_back( Sphere(glm::vec3(0, 0, 0), 1, true) ); // sound source


	for (int i = 0; i < spheres.size(); ++i) {
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

// Intersects the given ray with triangles in the scene
// and updates the given HitInfo using the information of the triangle
// that first intersects with the ray.
// Returns true if an intersection is found.
bool IntersectRayTriangle(HitInfo& hit, Ray ray) {
	hit.t = 1e30;
	bool foundHit = false;

	std::vector<Triangle> triangles;

	triangles.push_back( Triangle(glm::vec3(1, -1, 0), glm::vec3(-1, -1, 0), glm::vec3(0, -1, 1)) );
	triangles.push_back( Triangle(glm::vec3(-1, -1, 0), glm::vec3(1, -1, 0), glm::vec3(0, -1, -1)) );

	for (int i = 0; i < triangles.size(); ++i) {
		Triangle& tri = triangles[i];

		//calculate the normal of the triangle
		glm::vec3 edge1 = tri.v1 - tri.v0;
		glm::vec3 edge2 = tri.v2 - tri.v0;
		glm::vec3 h = glm::cross(ray.getDir(), edge2);
		float a = glm::dot(edge1, h);

		#pragma region CheckIntersection
		if (a > -1e-7 && a < 1e-7) // This means the ray is parallel to the triangle. No intersection so we skip this triangle
			continue;

		// Compute the factor to check if the intersection point is inside the triangle
		float f = 1.0 / a;
		glm::vec3 s = ray.getOrig() - tri.v0;
		float u = f * glm::dot(s, h);

		if (u < 0.0 || u > 1.0)
			continue; // The intersection point is outside the triangle

		glm::vec3 q = glm::cross(s, edge1);
		float v = f * glm::dot(ray.getDir(), q);

		if (v < 0.0 || u + v > 1.0)
			continue; // The intersection point is outside the triangle
		#pragma endregion CheckIntersection

		//We know that there is an intersection with the triangle
		// So we can compute t to find out where the intersection point is on the line.
		float t = f * glm::dot(edge2, q);

		if (t > 1e-7) { // Ray intersection
			if (t < hit.t) { // Check if this is the closest intersection so far
				foundHit = true;
				hit.t = t;
				hit.position = ray.getOrig() + ray.getDir() * t;
				hit.normal = normalize(glm::cross(edge1, edge2)); // Compute normal at the intersection point
				hit.mtl = tri.mtl;
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
	bool hitFound = false;

	#pragma region CheckIntersection
	// Check for sphere intersection
	hitFound = IntersectRaySphere(hit, ray);

	// Check for triangle intersection
	HitInfo triangleHit;
	if (IntersectRayTriangle(triangleHit, ray) && (!hitFound || triangleHit.t < hit.t)) {
		hit = triangleHit;
		hitFound = true;
	}
	#pragma endregion CheckIntersection

	if (hitFound) {
		glm::vec3 view = normalize(-ray.getDir());
		reflectInfo newReflectedSound(hit);
		reflectedSources.push_back(newReflectedSound);
		if (hit.mtl.isSource) return reflectedSources; // if the hit object is a sound source, stop tracing reflections

		// Compute reflections
		for (int bounce = 0; bounce < MAX_BOUNCES; ++bounce) {

			Ray r;	// this is the new reflection ray
			HitInfo h;	// reflection new hit info
			bool reflectionHitFound = false;

			// Initialize the reflection ray
			r.setDir(normalize(ray.getDir()) - 2 * dot(normalize(ray.getDir()), hit.normal) * hit.normal);
			r.setOrig(hit.position + r.getDir() * 0.0001f);

			#pragma region CheckIntersection
			// Check for sphere intersection
			reflectionHitFound = IntersectRaySphere(h, r);

			// Check for triangle intersection
			HitInfo triangleReflectionHit;
			if (IntersectRayTriangle(triangleReflectionHit, r) && (!reflectionHitFound || triangleReflectionHit.t < h.t)) {
				h = triangleReflectionHit;
				reflectionHitFound = true;
			}
			#pragma endregion CheckIntersection

			if (reflectionHitFound) {
				// TODO: Hit found, so make a sound at the hit point (not implemented)
				reflectInfo newReflectedSound(h, reflectedSources.back().totalAbsorbed);
				reflectedSources.push_back(newReflectedSound);

				ReverseAbsorptionOrder(reflectedSources);
				if (h.mtl.isSource) return reflectedSources; // if the hit object is a sound source, stop tracing reflections

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

		//returns empty source buffer cuz no sound source was hit
		for (int i = 0; i < reflectedSources.size(); i++) {
			alDeleteSources(1, &(reflectedSources[i].sound.sourceid));
			alDeleteBuffers(1, &(reflectedSources[i].sound.bufferid));
		}
		reflectedSources.clear();
		return reflectedSources;	// TODO: return the environment sound
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