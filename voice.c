#include "stm32f4_discovery.h"
#include "voice.h"

void voice_init(struct voice_t *voice, const uint16_t *table, uint32_t size, uint16_t freqMul)
{
  voice->table = table;
  voice->size = size;
  voice->freqMul = freqMul;
  voice->phaseAccumulator = freqMul;
  voice->ptr = 0; 
}

uint16_t voice_nextSample(struct voice_t *voice)
{
  uint16_t ov = voice->table[voice->ptr];
  voice->ptr+=(++voice->phaseAccumulator)%voice->freqMul;

  if(voice->ptr>=voice->size) {
    voice->ptr = 0;
  }

  return ov;
}
