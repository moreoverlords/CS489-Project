#include <stdio.h>
#include <math.h>
#include "include/portaudio.h"

#define NUM_SECONDS   (8)
#define SAMPLE_RATE   (44100)
#define TABLE_SIZE    (200)
#define TEST_UNSIGNED (0)

#ifndef M_PI
#define M_PI (3.14159265)
#endif

// list of variables that are shared between streams
// get ready for some gross concurrent code!
static int taps = 256;
static float mu = 0.06;
static int arrSize = 32768;
static float noise[32768]; static int noisePos = 0;
static float estimate[32768]; static int estPos = 0;
static float error[32768]; static int errPos = 0;

/* routine for the outer mic
** gathers sound and lets the other routines do the hard work
*/

static int pa_outerMic( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData ) {
		const float *in = (const float*)inputBuffer;
		int i = 0;

		for(i = 0; i < taps; i++) {
			noise[noisePos] = *in++;			
			//printf("noise: %f\n", noise[noisePos]); // avoid printing anything in the callback if possible
			noisePos = (noisePos+1) % arrSize;
		}
		return paContinue; // keep running
}


/* routine for the inner mic
** gathers sound and lets the other routines do the hard work
*/

static int pa_innerMic( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData ) {
		const float *in = (const float*)inputBuffer;
		int i = 0;

		for(i = 0; i < taps; i++) {
			error[errPos] = (*in++) *-1; // any sound we hear now is an error
			//printf("error: %f\n", error[errPos]);
			errPos = (errPos+1) % arrSize;
		}

		return paContinue; // keep running
}

/* routine for the headphone output
** Takes the input from the mics and estimates anti-noise to output
*/
static int pa_headphones( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData ) {
		float *out = (float*)outputBuffer;
		const float *in = (const float*)inputBuffer;
		int i = 0;
		int j = 0;

		for(i = 0; i < taps; i++) {
			//calculate the denominator of the NLMS step
			float denom = 0.000001;
			for(j = 0; j < taps; j++) {
				denom += noise[(noisePos-j)%arrSize] * noise[(noisePos-j)%arrSize];
			}

			// calculate and play the new output
			estimate[(estPos+1)%arrSize] = estimate[estPos%arrSize]
																		+(mu * error[estPos%arrSize]) / denom;
			//printf("estimate: %f\n", estimate[(estPos+1)%taps]);
			estPos = (estPos+1) % arrSize;
			
			*out++ = estimate[estPos%arrSize];	// we have two-channel output
			*out++ = estimate[estPos%arrSize];	// make sure to copy the sound to both ears

			error[errPos] = (*in++) *-1; // any sound we hear now is an error
			//printf("error: %f\n", error[errPos]);
			errPos = (errPos+1) % arrSize;	
		}

		return paContinue; // keep running
}


/*
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

		if( inputBuffer == NULL )
		{
			for( i=0; i<framesPerBuffer; i++ ){
				*out++ = 0;  // left - silent 
				*out++ = 0;  // right - silent
			}
		}else {
			for( i=0; i<framesPerBuffer; i++ ){
				*out++ = *in++;  // left - distorted
				*out++ = *in++;  // right - clean
			}
		}

    return paContinue; // keep playing indefinitely
}*/

/*******************************************************************/
int main(void);
int main(void)
{
    PaStreamParameters  headphones;
    PaStreamParameters  outerMic;
    PaStreamParameters  innerMic;
    PaStream*           noiseStream;
		PaStream*           outputStream;
    PaError             err;
    PaTime              streamOpened;
    int                 i, totalSamps;

		// initialization
    err = Pa_Initialize();
    if( err != paNoError )
        goto error;

		for(i = 0; i < arrSize; i++) {noise[i] = 0.00001;}
		for(i = 0; i < arrSize; i++) {estimate[i] = 0.00001;}
		for(i = 0; i < arrSize; i++) {error[i] = 0.00001;}

		// info for anti-noise output
    headphones.device = Pa_GetDefaultOutputDevice(); /* Default output device. */
    if (headphones.device == paNoDevice) {
      fprintf(stderr,"Error: No default output device.\n");
      goto error;
    }
    headphones.channelCount = 2;                     /* Stereo output. */
    headphones.sampleFormat = paFloat32;
    headphones.suggestedLatency = Pa_GetDeviceInfo( headphones.device )->defaultLowOutputLatency;
    headphones.hostApiSpecificStreamInfo = NULL;
    
		// info for the noise reference mic
		outerMic.device = Pa_GetDefaultInputDevice(); /* Default input device. */
    if (outerMic.device == paNoDevice) {
      fprintf(stderr,"Error: No default in device.\n");
      goto error;
    }
		outerMic.channelCount = 1;
    outerMic.sampleFormat = paFloat32;
    outerMic.suggestedLatency = Pa_GetDeviceInfo( outerMic.device )->defaultLowInputLatency;
    outerMic.hostApiSpecificStreamInfo = NULL;

		// info for the error reference mic
		innerMic.device = 5; /* hopefully the usb mic */
    if (innerMic.device == paNoDevice) {
      fprintf(stderr,"Error: No default in device.\n");
      goto error;
    }
		innerMic.channelCount = 1;
    innerMic.sampleFormat = paFloat32;
    innerMic.suggestedLatency = Pa_GetDeviceInfo( innerMic.device )->defaultLowInputLatency;
    innerMic.hostApiSpecificStreamInfo = NULL;
    

		// create the outer mic stream
		err = Pa_OpenStream( &noiseStream,
                         &outerMic,
                         NULL,
                         SAMPLE_RATE,
                         taps,       /* Frames per buffer. */
                         paClipOff,
                         pa_outerMic,
												 NULL);
    if( err != paNoError )
        goto error;

		// create the headphone stream
		err = Pa_OpenStream( &outputStream,
                         &innerMic,
                         &headphones,
                         SAMPLE_RATE,
                         taps,       /* Frames per buffer. */
                         paClipOff,
                         pa_headphones,
												 NULL);
    if( err != paNoError )
        goto error;

		printf("Starting Stream.\n");
    err = Pa_StartStream( noiseStream );
    if( err != paNoError )
        goto error;
		printf("Starting Stream.\n");
    err = Pa_StartStream( outputStream );
    if( err != paNoError )
        goto error;
		
    printf("Running...\n");

		while( ( err = Pa_IsStreamActive( noiseStream ) ) == 1 &&
					( err = Pa_IsStreamActive( outputStream ) ) == 1) {
        //Pa_Sleep(1);
		}
    if( err < 0 )
        goto error;
	 

		// close the streams
    err = Pa_CloseStream( noiseStream );
    if( err != paNoError )
        goto error;
    err = Pa_CloseStream( outputStream );
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
