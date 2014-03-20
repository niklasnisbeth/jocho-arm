#include "stm32f4_discovery.h"
#include "misc.h"

#include "hw/audio.h"

#include "synth/voice.h"
#include "synth/wt.h"
#include "counter.h"

#define SINE_SIZE 1024
#define BUFFER_SIZE 16*3

volatile uint16_t buffer1[BUFFER_SIZE*2];
volatile uint16_t buffer2[BUFFER_SIZE*2];
volatile uint32_t ptr1 = 0;
volatile uint32_t ptr2 = 0;

struct voice_t voice1;
struct voice_t voice2;
struct voice_t voice3;
struct voice_t voice4;
struct voice_t voice5;
struct voice_t voice6;

float sinevals[SINE_SIZE];
struct wavetable_t sinetable;

void 
DMA1_Stream5_IRQHandler(void)
{
  if (DMA_GetFlagStatus(DMA1_Stream5, DMA_FLAG_TCIF5) != RESET)
  {
    ptr2 = 0;

    DMA_ClearFlag(DMA1_Stream5, DMA_FLAG_TCIF5);
  }
}

void 
DMA1_Stream6_IRQHandler(void)
{
  if (DMA_GetFlagStatus(DMA1_Stream6, DMA_FLAG_TCIF6) != RESET)
  {
    ptr1 = 0;

    DMA_ClearFlag(DMA1_Stream6, DMA_FLAG_TCIF6); 
  }
} 

void
TIM4_IRQHandler(void)
{ 
  if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET) { 
    uint32_t val = counter_tick(&sh_counter);
    GPIO_Write(GPIOA, val&0b11);

    TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
  }
}

int 
main(void)
{ 
  sinetable.vals = sinevals;
  sinetable.size = SINE_SIZE; 

  voice_init(&voice1, &sinetable);
  voice_init(&voice2, &sinetable);
  voice_init(&voice3, &sinetable);
  voice_init(&voice4, &sinetable);
  voice_init(&voice5, &sinetable);
  voice_init(&voice6, &sinetable);

  voice_trigger(&voice1);
  voice_trigger(&voice2);
  voice_trigger(&voice3);
  voice_trigger(&voice4);
  voice_trigger(&voice5);
  voice_trigger(&voice6);

  audio_init(buffer1, buffer2, BUFFER_SIZE);

  while(1) {
    if(ptr1<BUFFER_SIZE) {
      voice_update_envs(&voice1);
      buffer1[(!DMA_GetCurrentMemoryTarget(DMA1_Stream5))*BUFFER_SIZE+ptr1] =
        (uint16_t)((1.f+voice_next_sample(&voice1))*32578.f);
      ptr1++;
      voice_update_envs(&voice2);
      buffer1[(!DMA_GetCurrentMemoryTarget(DMA1_Stream5))*BUFFER_SIZE+ptr1] = 0;
      ptr1++;
      voice_update_envs(&voice3);
      buffer1[(!DMA_GetCurrentMemoryTarget(DMA1_Stream5))*BUFFER_SIZE+ptr1] = 0;
      ptr1++;
    }
    if(ptr2<BUFFER_SIZE) {
      voice_update_envs(&voice4);
      buffer2[(!DMA_GetCurrentMemoryTarget(DMA1_Stream6))*BUFFER_SIZE+ptr2] = 0;
      ptr2++;
      voice_update_envs(&voice5);
      buffer2[(!DMA_GetCurrentMemoryTarget(DMA1_Stream6))*BUFFER_SIZE+ptr2] = 0;
      ptr2++;
      voice_update_envs(&voice6);
      buffer2[(!DMA_GetCurrentMemoryTarget(DMA1_Stream6))*BUFFER_SIZE+ptr2] = 0;
      ptr2++;
    }
  }
}


#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void 
assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}

#endif

