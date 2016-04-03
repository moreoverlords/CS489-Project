#include <stdio.h>
#include <math.h>
#include "include/portaudio.h"

#define SAMPLE_RATE   (44100)

#ifndef M_PI
#define M_PI (3.14159265)
#endif

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
#define TAPS					(512)
#define MU						(0.006)

static float buffer[TAPS];
static int sampleCount = 0;
static bool readyToStart;

static float w[TAPS];


/*static float* orderBuffer() {
	float newBuffer[TAPS];
	int newBufferPos = 0;

	for(int i = buffer_head+1; i < TAPS; i++, newBufferPos++) {
		newBuffer[newBufferPos] = buffer[i];
	}

	for(int i = 0; i <= buffer_head && i < TAPS; i++, newBufferPos++) {
		newBuffer[newBufferPos] = buffer[i];
	}
	
	return newBuffer;
}*/

static void insertToBuffer(float newVal) {

	for(int i = 0; i < TAPS-1; i++) {
		buffer[i] = buffer[i+1];
	}
	buffer[TAPS-1] = newVal;

}

static float dotProd(float* a, float* b, int size) {
	float result = 0;

	for(int i = 0; i < size; i++) {
		result += a[i] * b[i];
	}

	return result;
}

static float magnitudeSquared(float* a, int size) {
	float result;

	for(int i = 0; i < size; i++) {
		result += a[i] * a[i];
	}

	return result;
}

static int pa_NLMS( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData )
{
    float *out = (float*)outputBuffer;
		const float *in = (const float*)inputBuffer;
    int i;
    int framesToCalc;
    int finished = 0;


		if(in == NULL)
		{
			for( i=0; i<framesPerBuffer; i++ ){
				*out++ = 0;  /* left - silent */
				*out++ = 0;  /* right - silent */
			}
		} else {

			for( i=0; i<framesPerBuffer; i++ ){

				insertToBuffer(*in);
				if(readyToStart) {
					float antinoise = dotProd(buffer, w, TAPS);
					float error = buffer[TAPS-1] - antinoise;

					float weightTerm = MU * error / magnitudeSquared(buffer,TAPS);

					for(int j = 0; j < TAPS; j++) {
						w[j] = w[j] + buffer[j] * weightTerm;
					}
					
					//printf("antinoise: %f\n", antinoise);
					if(antinoise > 1.0) {
						// we've gone off the rails!
						// time to bring it back in
						antinoise = 0.0;
						for(int j = 0; j < TAPS; j++) {
							w[j] = 0.0;
						}
					}

					*out++ = antinoise;
					*out++ = antinoise;

				} else {
					*out++ = 0;  /* left  */
					*out++ = 0;  /* right */
				}

				if(!readyToStart && sampleCount >= TAPS-1) {
						readyToStart = true;
				} else {
					sampleCount = sampleCount+1;		
				}			
			}
		}
    return paContinue; // keep playing indefinitely
}

/*******************************************************************/
int main(void);
int main(void)
{
    PaStreamParameters  outputParameters;
    PaStreamParameters  inputParameters;
    PaStream*           stream;
    PaError             err;
    PaTime              streamOpened;
    int                 i, totalSamps;
    
		// initialize arrays
		for(i = 0; i < TAPS; i++) {
			buffer[i] = 0.0000001;
			w[i] = 0.0;
		}

    err = Pa_Initialize();
    if( err != paNoError )
        goto error;

    outputParameters.device = Pa_GetDefaultOutputDevice(); /* Default output device. */
    if (outputParameters.device == paNoDevice) {
      fprintf(stderr,"Error: No default output device.\n");
      goto error;
    }
    outputParameters.channelCount = 2;                     /* Stereo output. */
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = 2*Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    
		inputParameters.device = Pa_GetDefaultInputDevice(); /* Default input device. */
    if (inputParameters.device == paNoDevice) {
      fprintf(stderr,"Error: No default in device.\n");
      goto error;
    }
		inputParameters.channelCount = 1;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = 2*Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;
    

		err = Pa_OpenStream( &stream,
                         &inputParameters,
                         &outputParameters,
                         SAMPLE_RATE,
                         8,       /* Frames per buffer. */
                         paClipOff,
                         pa_NLMS,
												 NULL);
    if( err != paNoError )
        goto error;

    streamOpened = Pa_GetStreamTime( stream ); /* Time in seconds when stream was opened (approx). */

		printf("Starting Stream.\n");
    err = Pa_StartStream( stream );
    if( err != paNoError )
        goto error;
		
		//printf("Pausing Stream.\n");
    /* Watch until sound is halfway finished. */
    /* (Was ( Pa_StreamTime( stream ) < (totalSamps/2) ) in V18. */
    //while( (Pa_GetStreamTime( stream ) - streamOpened) < (PaTime)NUM_SECONDS / 2.0 )
        //Pa_Sleep(10);


    printf("Waiting for sound to finish.\n");

    while( ( err = Pa_IsStreamActive( stream ) ) == 1 )
        Pa_Sleep(100);
    if( err < 0 )
        goto error;
		
    err = Pa_CloseStream( stream );
    if( err != paNoError )
        goto error;

    Pa_Terminate();
    printf("Test finished.\n");
    return err;
error:
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;
}
