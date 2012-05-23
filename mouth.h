/*
MOUTH.h
*/
/* Time to beat this into a shape */

int CVIFUNC mouth_InitRF (void);

int CVIFUNC mouth_Error (const char message[], dnaByte abortQ,
						 dnaByte forceErrorQ);

int CVIFUNC mouth_WriteDS345 (const char command[], int numberofBytes);

int CVIFUNC mouth_ZeroDS345 (void);

int CVIFUNC mouth_MakeIdleCommand (char ** command, double amp, double DCOffset,
								   double freq);

int CVIFUNC mouth_AddBufferCommand (dnaBufferRF*buffer, double amp, double freq);

int CVIFUNC mouth_AddBufferZero (dnaBufferRF*buffer);

int CVIFUNC mouth_AddBufferFreq (dnaBufferRF*buffer, double freq);

int CVIFUNC mouth_AddBufferAmp (dnaBufferRF*buffer, double amp);

int CVIFUNC mouth_ResetBuffer (dnaBufferRF*buffer);

int CVIFUNC mouth_OutBufferCommand (dnaBufferRF*buffer);
