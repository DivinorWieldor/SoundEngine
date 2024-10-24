# SoundEngine
 Basic attempt at creating a custom sound engine utilizing OpenAL-Soft

 This project is now archived and will not be developed.
 
 This project utilizes SDL2/OpenAL-Soft and glm.

useful resources for learning:
- [basic OpenAL tutorial](youtube.com/watch?v=tmVRpNFP9ys)

TODO:
- Mouse control
	- w.r.t. xz-plane
	- movement affected
- Raytraced audio
	- Implement material transitivity (allow some rays to move past materials. Will suffer greater dampening if this happens!)
	- Add double face support for triangles
- integrate velocity (for doppler shift)
- Merge SoundFile and SineW
- Improve performance
	- reduce amount of if/else calls
	- add a global buffer pool to pull from. Eliminates need to constantly generate and delete buffers
 - Critical features:
 	- Reverberations
  	- Ray-traced audio with minimal stuttering
   		- this requires the audio to not be generated at hit positions. Ray-tracing should provide information on the audio to play, and a few HRTF aligned speakers around user should use this information to generate the audio. This avoids 256 sound source limitation.

Limitations:
- ALSOFT has a 256 sound source limit! [solutions?](https://stackoverflow.com/questions/28141817)
