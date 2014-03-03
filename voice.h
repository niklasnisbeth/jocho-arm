#ifndef __VOICE_H
#define __VOICE_H

struct voice_t {
  const uint16_t *table;
  uint32_t size;
  uint32_t ptr;
  uint16_t freqMul;
  uint16_t phaseAccumulator;
};

void voice_init(struct voice_t *voice, const uint16_t *table, uint32_t size, uint16_t freqMul);

uint16_t voice_nextSample(struct voice_t *voice);

#endif
