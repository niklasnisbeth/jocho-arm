#include "stm32f4_discovery.h"
#include "counter.h"

void counter_init(struct counter_t *c, uint32_t max)
{
  c->max = max;
  c->count = 0;
}

uint32_t counter_tick(struct counter_t *c)
{
  uint32_t val = c->count;
  c->count = (c->count+1)%c->max;
  return val;
}

uint32_t counter_val(struct counter_t *c)
{
  return c->count;
}

void counter_reset(struct counter_t *c)
{
  c->count = 0;
}

