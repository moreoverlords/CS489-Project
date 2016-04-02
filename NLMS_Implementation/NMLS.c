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

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
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

		if( inputBuffer == NULL )
		{
			for( i=0; i<framesPerBuffer; i++ ){
				*out++ = 0;  /* left - silent */
				*out++ = 0;  /* right - silent */
			}
		}else {
			for( i=0; i<framesPerBuffer; i++ ){
				*out++ = *in++;  /* left - distorted */
				*out++ = *in++;          /* right - clean */
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
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    
		inputParameters.device = Pa_GetDefaultInputDevice(); /* Default input device. */
    if (inputParameters.device == paNoDevice) {
      fprintf(stderr,"Error: No default in device.\n");
      goto error;
    }
		inputParameters.channelCount = 2;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;
    

		err = Pa_OpenStream( &stream,
                         &inputParameters,
                         &outputParameters,
                         SAMPLE_RATE,
                         256,       /* Frames per buffer. */
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
