#ifndef __COUNTER_H
#define __COUNTER_H

struct counter_t {
  uint32_t count;
  uint32_t max;
};

void counter_init(struct counter_t *c, uint32_t max);
uint32_t counter_tick(struct counter_t *c);
uint32_t counter_val(struct counter_t *c);
void counter_reset(struct counter_t *c);

#endif
