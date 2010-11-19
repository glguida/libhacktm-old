/*
 * Microphone to bit array via FDR. Code partially derived from PortAudio examples.
 */

#include <stdio.h>
#include <stdlib.h>
#include <portaudio.h>
#include <fftw3.h>
#include <math.h>

#include <ncurses.h>

#include "htmnode.h"

#define SAMPLE_PERIOD_FRACTION 20
#define SAMPLE_RATE  (44100)
#define FRAMES_PER_BUFFER (4096)
#define NUM_CHANNELS    (1)

#define PA_SAMPLE_TYPE  paFloat32
#define SAMPLE_SILENCE  (0.0f)
#define PRINTF_S_FORMAT "%.8f"

WINDOW *wout1, *wout2, *wout3, *wout4;
fftwf_plan dct;
float *dct_result;


void
display_init(void)
{
    initscr();
    noecho();
    wout1 = newwin(10, COLS-4, 2, 2);
    box(wout1, 0, 0);
    wrefresh(wout1);

    wout2 = newwin(10, COLS-4, 14, 2);
    box(wout2, 0, 0);
    wrefresh(wout2);

    wout3 = newwin(10, COLS-4, 26, 2);
    box(wout3, 0, 0);
    wrefresh(wout3);

    wout4 = newwin(10, COLS-4, 38, 2);
    box(wout4, 0, 0);
    wrefresh(wout4);
}

void
portaudio_init(PaStream **stream)
{
  PaStreamParameters input_param;
  PaError err;

  err = Pa_Initialize();
  if( err != paNoError ) goto error;

  input_param.device = Pa_GetDefaultInputDevice(); /* default input device */
  input_param.channelCount = NUM_CHANNELS;
  input_param.sampleFormat = PA_SAMPLE_TYPE;
  input_param.suggestedLatency = Pa_GetDeviceInfo( input_param.device )->defaultLowInputLatency;
  input_param.hostApiSpecificStreamInfo = NULL;

  err = Pa_OpenStream(
	      stream,
              &input_param,
              NULL,                  /* &outputParameters, */
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              NULL, /* no callback, use blocking API */
              NULL ); /* no callback, so no callback userData */
  if( err != paNoError ) goto error;

  return;
error:
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    exit(-1);
}


void
portaudio_start(PaStream *stream)
{
  PaError err;
  err = Pa_StartStream( stream );
  if( err != paNoError ) {
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    exit(-1);
  }
}


void 
portaudio_read(PaStream *stream, float *out)
{
  Pa_ReadStream( stream, out, SAMPLE_RATE/SAMPLE_PERIOD_FRACTION);
}


void
portaudio_close(PaStream *stream)
{
  PaError err;
  err = Pa_CloseStream( stream );
  if( err != paNoError ) goto error;

  Pa_Terminate();
  return;


error:
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    exit(-1);
}


void
bitar_dumpw(WINDOW *win, bitar_t *ba)
{
  static int k;
  int i, j;
  size_t ms = ba->map_size;
  wmove(win, 1, 1);
  k = 0;
  for ( i = ms -1; i >= 0; i-- )
    {
      uint64_t map = ba->map[i];
      wprintw(win, " %016llx ", map);
      if ( (++k % ((COLS - 6)/18) == 0) )
	wmove(win, getcury(win) + 1, 1);
    }
  wrefresh(win);
}

void nodeconn_mergeval(struct node_conn *inputs, size_t no, struct node_conn *output)
{
  size_t i, sz = bitar_bitsize(VAL(inputs));
  assert(bitar_size(VAL(inputs)) * no == bitar_size(VAL(output)));
  for ( i = 0; i < no; i++ ) {
    bitar_putsub(VAL(output), i, VAL(inputs) + i);
  }
}

void nodeconn_splitexp(struct node_conn *input, struct node_conn *outputs, size_t no)
{
  size_t i, sz = bitar_bitsize(EXP(outputs));
  assert(bitar_size(EXP(outputs)) * no == bitar_size(EXP(input)));
  for ( i = 0; i < no; i++ ) {
    bitar_getsub(EXP(outputs) + i, EXP(input), i);
  }
}

#define FDR_SEGMENTS 20
#define L0_INPUTS FDR_SEGMENTS
#define L1_INPUTS (L0_INPUTS/2)
#define L2_INPUTS (L1_INPUTS/2)

int
main()
{
  int i;
  PaStream *stream;
  int totframes = SAMPLE_RATE/SAMPLE_PERIOD_FRACTION;
  int numsamples = totframes * NUM_CHANNELS;
  int numbytes = numsamples * sizeof(float);
  float *audio_in = malloc(numbytes);
  float *dct_out = malloc(numbytes);
  fdr_t dct2bitar;
  node_t n1[L1_INPUTS], n2[L2_INPUTS], ntop[1];
  struct node_conn l0[L0_INPUTS], 
    l1in[L1_INPUTS], l1out[L1_INPUTS],
    l2in[L2_INPUTS], l2out[L2_INPUTS], l3in[1], l3out[1];
  uint64_t l0s = 0, l1s = 0, l2s = 0, l3s = 0;
  
  assert(audio_in != NULL);
  assert(dct_out != NULL);

  /* Initialize ncurses screen. */
  display_init();

  /* Initialize FDR and bitar */
  fdr_init(&dct2bitar, numsamples/FDR_SEGMENTS, 20, 20, 10);

  /* Initialize example node hierarchy n1->n2->n3 */
  for ( i = 0; i < L0_INPUTS; i++ )
    node_conn_init(l0 + i, 20);

  for ( i = 0; i < L1_INPUTS; i++ ) {
    node_init(n1 + i, 20 * 2, 4, 5, 10, 5, 5);
    node_conn_init(l1in + i, 20 * 2);
    node_conn_init(l1out + i, 20);
  }

  for ( i = 0; i < L2_INPUTS; i++ ) {
    node_init(n2 + i, 20 * 2, 4, 5/*in_conn_fct*/, 10/*%top*/, 5, 5);
    node_conn_init(l2in + i, 20 * 2);
    node_conn_init(l2out + i, 20);
  }

  node_init(ntop, 20 * 5, 12, 5, 10/*%top*/, 5, 5);
  node_conn_init(l3in, 20 * 5);
  node_conn_init(l3out, 20 * 3);
  
  /* Initialize DCT with fftw (single precision) */
  dct = fftwf_plan_r2r_1d(numsamples, audio_in, dct_out, FFTW_REDFT10, FFTW_MEASURE);

  /* Initialize PortAudio */
  portaudio_init(&stream);

  /* Start recording! */
  portaudio_start(stream);

  while ( 1 ) {
    int i;
    int cont = 0;

    wmove(wout4, 1,1);
    wprintw(wout4, "l1s/l0s: %08f\nl2s/l0s: %08f\nl3s/l0s: %08f\n", (float)l1s/l0s, (float)l2s/l0s, (float)l3s/l0s);
    wrefresh(wout4);

    bitar_dumpw(wout3, VAL(l1out));
    bitar_dumpw(wout2, VAL(l2out));
    bitar_dumpw(wout1, VAL(l3out));


    for( i=0; i<numsamples; i++ ) audio_in[i] = 0;
    portaudio_read(stream, audio_in);
    fftwf_execute(dct);

    /* Create a (non-logarithmic) tonotopical map! */
    for ( i = 0; i < FDR_SEGMENTS; i++ ) {
      bitar_reset(VAL(l0 + i));
      /* Check for signal energy */
      if ( floor(abs(dct_out[0])) == 0 ) {
	bzero(dct_out, sizeof(*dct_out * numbytes));
      }
      vfdr_generate(&dct2bitar, dct_out + i * 20, NULL, VAL(l0 + i));
      //      bitar_putsub(VAL(&l0[i/2]), (i % 2) * 20, &ba);
    }
    l0s++;

    cont = 0;

    for ( i = 0; i < L1_INPUTS; i++ ) {
      /* Merge from EXP(L1 + i / x) to this EXP(l1tmp). */
      nodeconn_mergeval(l0 + 2 * i, 2, l1in + i);
      if ( !node_process(n1 + i, l1in + i, l1out + i) ) {
	continue;
      }
      cont |= 1;
      //      bitar_putsub(VAL(l1), (i & 1) * 20, VAL(&l1tmp));
      /* Merge from VAL(l1tmp) into VAL(L1 + i/x) */
    }
    if ( !cont )
      continue;
    l1s++;
    cont = 0;


    for ( i = 0; i < L2_INPUTS; i++ ) {
      nodeconn_mergeval(l1out + 2 * i, 2, l2in + i);
      if ( !node_process(n2 + i, l2in + i, l2out + i) ) {
	nodeconn_splitexp(l2in + i, l1out + 2 * i, 2);
	continue;
      }
      cont |= 1;
      nodeconn_splitexp(l2in + i, l1out + 2 * i, 2);
    }
    
    l2s++;
    if ( !cont )
      continue;
    cont = 0;

    nodeconn_mergeval(l2out, 5, l3in);
    if ( !node_process(ntop, l3in, l3out) ) {
      nodeconn_splitexp(l3in, l2out, 5);
      continue;
    }
    nodeconn_splitexp(l3in, l2out, 5);
    l3s++;
  }
  portaudio_close(stream);
}
