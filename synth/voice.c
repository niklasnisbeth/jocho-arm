#include "voice.h"
#include "algorithms.h"

void
voice_init ( struct voice_t *voice )
{
  voice->algorithm = 1;
  int i = 0;
  for (i = 0; i<NUM_OPS-1; i++)
  {
    op_init(&voice->ops[i], 110, 0.05f, 0.0f);
    env_init(&voice->ops[i].aenv, 1.0, 20, 1500, 0.0, 0.9f);
    env_init(&voice->ops[i].penv, 1.0, 1, 1, 1.0, -0.9f);
  }

  op_init(&voice->ops[1], 220, 0.0f, 0.2f);
  env_init(&voice->ops[1].aenv, 0.0, 300, 800, 1.0, 0.0f);
  env_init(&voice->ops[1].penv, 1.0, 50, 100, 1.0, -0.9f);
  op_init(&voice->ops[2], 220, 0.0f, 0.8f);
  env_init(&voice->ops[2].aenv, 1.0, 300, 800, 1.0, 0.9f);
  env_init(&voice->ops[2].penv, 1.0, 50, 100, 1.0, -0.9f);
  op_init(&voice->ops[3], 800, 0.0f, 1.0f);
  env_init(&voice->ops[3].aenv, 1.0, 10, 750, 0.0, 8.9f);
  env_init(&voice->ops[3].penv, 2.0, 10, 300, 1.0, 1.9f);
}

void
voice_trigger (struct voice_t *voice )
{
  int i;
  for (i = 0; i<NUM_OPS; i++)
  {
    op_trigger(&voice->ops[i]);
  }
}


float
voice_next_sample ( struct voice_t *voice )
{
  voice->output_buffer = 0;

  for (int i=0; i<NUM_OPS; i++)
  {
    op_update_phase(&voice->ops[i]);
  }

  return (*algorithms[voice->algorithm])(voice);
}

void
voice_update_envs ( struct voice_t *voice )
{
  int i;
  for (i=0; i<NUM_OPS; i++)
  {
    env_update(&voice->ops[i].aenv);
    env_update(&voice->ops[i].penv);
  }
}
