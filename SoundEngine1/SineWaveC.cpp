// sudo apt-get install libopenal-dev  #  install OpenAL on linux

// gcc -o openal_play_monday   openal_play_monday.c  -lopenal -lm

#include <stdio.h>
#include <stdlib.h>    // gives malloc
#include <math.h>

#include <AL/al.h>
#include <AL/alc.h>

#define SDL_MAIN_HANDLED
#include <SDL/SDL.h>

ALCdevice* openal_output_device;
ALCcontext* openal_output_context;

ALuint internal_buffer;
ALuint streaming_source[1];

int al_check_error(const char* given_label) {

    ALenum al_error;
    al_error = alGetError();

    if (AL_NO_ERROR != al_error) {

        printf("ERROR - %s  (%s)\n", alGetString(al_error), given_label);
        return al_error;
    }
    return 0;
}

void MM_init_al() {

    const char* defname = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);

    openal_output_device = alcOpenDevice(defname);
    openal_output_context = alcCreateContext(openal_output_device, NULL);
    alcMakeContextCurrent(openal_output_context);

    // setup buffer and source

    alGenBuffers(1, &internal_buffer);
    al_check_error("failed call to alGenBuffers");
}

void MM_exit_al() {

    ALenum errorCode = 0;

    // Stop the sources
    alSourceStopv(1, &streaming_source[0]);        //      streaming_source
    for (int ii = 0; ii < 1; ++ii) {
        alSourcei(streaming_source[ii], AL_BUFFER, 0);
    }
    // Clean-up
    alDeleteSources(1, &streaming_source[0]);
    alDeleteBuffers(16, &streaming_source[0]);
    errorCode = alGetError();
    alcMakeContextCurrent(NULL);
    errorCode = alGetError();
    alcDestroyContext(openal_output_context);
    alcCloseDevice(openal_output_device);
}

void MM_render_one_buffer() {

    /* Fill buffer with Sine-Wave */
    // float freq = 440.f;
    float freq = 100.f;
    float incr_freq = 0.1f;

    int seconds = 4;
    // unsigned sample_rate = 22050;
    unsigned sample_rate = 44100;
    double my_pi = 3.14159;
    size_t buf_size = seconds * sample_rate;

    // allocate PCM audio buffer        
    //short* samples = malloc(sizeof(short) * buf_size);
    short* samples = new short[buf_size];

    printf("\nhere is freq %f\n", freq);
    int i = 0;
    for (; i < buf_size; ++i) {
        samples[i] = 32760 * sin((2.f * my_pi * freq) / sample_rate * i);

        freq += incr_freq;
        // incr_freq += incr_freq;
        // freq *= factor_freq;

        if (100.0 > freq || freq > 5000.0) {

            incr_freq *= -1.0f;
        }
    }

    /* upload buffer to OpenAL */
    alBufferData(internal_buffer, AL_FORMAT_MONO16, samples, buf_size, sample_rate);
    al_check_error("populating alBufferData");

    delete[] samples;

    /* Set-up sound source and play buffer */
    // ALuint src = 0;
    // alGenSources(1, &src);
    // alSourcei(src, AL_BUFFER, internal_buffer);
    alGenSources(1, &streaming_source[0]);
    alSourcei(streaming_source[0], AL_BUFFER, internal_buffer);
    // alSourcePlay(src);
    alSourcePlay(streaming_source[0]);

    // ---------------------

    ALenum current_playing_state;
    alGetSourcei(streaming_source[0], AL_SOURCE_STATE, &current_playing_state);
    al_check_error("alGetSourcei AL_SOURCE_STATE");
    Uint64 start;

    while (AL_PLAYING == current_playing_state) {
        start = SDL_GetTicks64();

        printf("still playing ... so sleep\n");

        //necessary for sound playing when moving
        if (1000 / 30 > SDL_GetTicks64() - start)
            SDL_Delay(1000 / 30 - (SDL_GetTicks64() - start));

        alGetSourcei(streaming_source[0], AL_SOURCE_STATE, &current_playing_state);
        al_check_error("alGetSourcei AL_SOURCE_STATE");
    }

    printf("end of playing\n");

    /* Dealloc OpenAL */
    MM_exit_al();

}   //  MM_render_one_buffer

int main() {

    MM_init_al();

    MM_render_one_buffer();
}