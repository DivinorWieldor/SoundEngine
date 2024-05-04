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
	Ray();
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
	float absorb;	// absorption modifier (ranges from 0.0 to 1.0)
	//may also have a scatter modifier (how much randomization to add to reflection) -> not sure if necessary

	Material() : absorb(0.3) {}

	int soundDampenPercent() { return absorb; }
};

struct Sphere {
	glm::vec3	center;
	float		radius;
	Material	mtl;

	Sphere() : center(glm::vec3(0, 0, 0)), radius(5) {};
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
	soundFile sound;
	HitInfo hit;
	int totalAbsorbed; // multiply with original sound source to get dampened sound (reduced amplitude)

	reflectInfo(HitInfo _hit) : hit(_hit) { totalAbsorbed = hit.mtl.soundDampenPercent(); }
	reflectInfo(HitInfo _hit, int prevDampen) : hit(_hit) { totalAbsorbed = prevDampen * _hit.mtl.soundDampenPercent(); }
};

const int NUM_SPHERES = 1; //ideally we want this to be however many surfaces there are in the scene
int MAX_BOUNCES = 5;
Sphere spheres[NUM_SPHERES]; //number of objects
//uniform Light  lights[NUM_LIGHTS]; //number of sound sources
//uniform samplerCube envMap; //pretty sure this is just a cube map
int bounceLimit = 5; //don't understand why this is needed?

// Intersects the given ray with all spheres in the scene
// and updates the given HitInfo using the information of the sphere
// that first intersects with the ray.
// Returns true if an intersection is found.
bool IntersectRaySphere(HitInfo &hit, Ray ray){
	hit.t = 1e30;
	bool foundHit = false;

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
std::vector<reflectInfo> RayTracer(Ray ray){
	HitInfo hit;
	std::vector<reflectInfo> reflectedSources; // stores all sound source instances, the returned ones will play one bit of sound

	if (IntersectRaySphere(hit, ray)) {
		glm::vec3 view = normalize(-ray.getDir());
		//vec3 clr = Shade( hit.mtl, hit.position, hit.normal, view );
		reflectInfo newReflectedSound(hit);
		reflectedSources.push_back(newReflectedSound);

		// Compute reflections
		for(int bounce = 0; bounce < MAX_BOUNCES; ++bounce) {
			if (bounce >= bounceLimit) break;

			Ray r;	// this is the reflection ray
			HitInfo h;	// reflection hit info

			// Initialize the reflection ray
			r.setDir( normalize(ray.getDir()) - 2 * dot(normalize(ray.getDir()), hit.normal) * hit.normal );
			r.setOrig( hit.position + r.getDir() * 0.0001f );


			if (IntersectRaySphere(h, r)) {
				// TODO: Hit found, so make a sound at the hit point (not implemented)
				// clr += vec3(h.normal); // Test reflection intersections
				// clr += vec3(1.0, 0.0, 0.0);
				//clr += Shade(h.mtl, h.position, h.normal, view);
				reflectInfo newReflectedSound(h, reflectedSources.back().totalAbsorbed); // TODO: this needs to be filled with sound source information as soon as you know it!
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
#endif