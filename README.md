# SoundEngine
 Basic attempt at creating a custom sound engine utilizing OpenAL-Soft
 
 This project utilizes SDL2/OpenAL-Soft and glm.

useful resources for learning:
- [basic OpenAL tutorial](youtube.com/watch?v=tmVRpNFP9ys)

TODO:
- Mouse control
	- w.r.t. xz-plane
	- movement affected
- Raytraced audio
	- Play sound at reflection positions ----> Seperate sound source and object. Currently, the sound source is any sphere object. Make this so that a valid sound source is when a specific object is hit
	- Implement material transitivity (allow some rays to move past materials. Will suffer greater dampening if this happens!)
	- Implement plane equation
		- Implement polygons for complex objects
- integrate velocity (for doppler shift)
- Merge SoundFile and SineW
- Improve performance
	- reduce amount of if/else calls
	- add a global buffer pool to pull from. Eliminates need to constantly generate and delete buffers

Limitations:
- ALSOFT has a 256 sound source limit! [solutions?](https://stackoverflow.com/questions/28141817)