#include "stm32f4_discovery.h"
#include "misc.h"
#include "hw/audio.h"
#include "counter.h"

struct counter_t sh_counter;

static void 
DMA_Config(DMA_Stream_TypeDef *stream, volatile uint16_t *buffer_top, volatile uint16_t *buffer_bottom, uint32_t buffer_size, uint32_t dac_channel)
{
  DMA_DeInit(stream);

  DMA_InitTypeDef DMA_InitStructure;
  DMA_StructInit(&DMA_InitStructure);

  DMA_InitStructure.DMA_Channel = DMA_Channel_7; 
  DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable; 
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single; 

  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)buffer_top;
  DMA_InitStructure.DMA_BufferSize = buffer_size;
  DMA_InitStructure.DMA_PeripheralBaseAddr = ((dac_channel == DAC_Channel_1) ? DAC_DHR12L1_ADDRESS : DAC_DHR12L2_ADDRESS);

  DMA_ITConfig(stream, DMA_IT_TC, ENABLE);

  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = ((stream == DMA1_Stream6) ? DMA1_Stream6_IRQn : DMA1_Stream5_IRQn);
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  DMA_Init(stream, &DMA_InitStructure);

  DMA_DoubleBufferModeConfig(stream, (uint32_t)buffer_bottom, 0);
  DMA_DoubleBufferModeCmd(stream, ENABLE);
        
  DMA_Cmd(stream, ENABLE);
}

static void 
DAC_TIM_Config(void)
{
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
  
  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
  TIM_TimeBaseStructure.TIM_Period = 0x400-1;
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; 

  TIM_UpdateRequestConfig(TIM4, TIM_UpdateSource_Regular);
  TIM_SelectOutputTrigger(TIM4, TIM_TRGOSource_Update); 

  TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
  
  TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
  TIM_ITConfig(TIM4, TIM_IT_CC1, DISABLE);
  TIM_ITConfig(TIM4, TIM_IT_CC2, DISABLE);
  TIM_ITConfig(TIM4, TIM_IT_CC3, DISABLE);
  TIM_ITConfig(TIM4, TIM_IT_CC4, DISABLE);

  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure); 

  TIM_Cmd(TIM4, ENABLE);
}

static void 
Strobe_TIM_Config(void)
{
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  TIM_OCInitTypeDef TIM_OCInitStructure;
  TIM_ICInitTypeDef TIM_ICInitStructure;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE); 
  
  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
  TIM_TimeBaseStructure.TIM_Period = 0x3A0-1;
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; 
  TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);

  TIM_OCStructInit(&TIM_OCInitStructure);
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = 0xF0;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
  TIM_OC3Init(TIM5, &TIM_OCInitStructure); 
  
  TIM_ICStructInit(&TIM_ICInitStructure);
  TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
  TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
  TIM_ICInitStructure.TIM_ICFilter = 0;
  TIM_ICInit(TIM5, &TIM_ICInitStructure);
  
  TIM_SelectOnePulseMode(TIM5, TIM_OPMode_Single);
  TIM_ITRxExternalClockConfig(TIM5, TIM_TS_ITR2);
  TIM_SelectSlaveMode(TIM5, TIM_SlaveMode_Trigger);
  TIM_ARRPreloadConfig(TIM5, ENABLE); 
  TIM_OC3PreloadConfig(TIM5, TIM_OCPreload_Enable);

  TIM_Cmd(TIM5, ENABLE);
}

static void 
DAC_Config(uint32_t dac_channel)
{
  DAC_InitTypeDef DAC_InitStructure; 

  DAC_StructInit(&DAC_InitStructure);
  DAC_InitStructure.DAC_Trigger = DAC_Trigger_T4_TRGO;
  DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
  DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
  DAC_Init(dac_channel, &DAC_InitStructure);

  DAC_Cmd(dac_channel, ENABLE);

  DAC_DMACmd(dac_channel, ENABLE);
}

void
audio_init (volatile uint16_t *buffer1, volatile uint16_t *buffer2, uint16_t bufsize, uint16_t num_channels)
{ 
  counter_init(&sh_counter, num_channels);

  GPIO_InitTypeDef GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1 | RCC_AHB1Periph_GPIOA, ENABLE);

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1; // S&H address
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; // S&H strobe
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; 
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_TIM5); 

  /* DAC & DMA initial init */
  DMA_Config(DMA1_Stream5, &buffer1[0], &buffer1[bufsize], bufsize, DAC_Channel_1);
  DMA_Config(DMA1_Stream6, &buffer2[0], &buffer2[bufsize], bufsize, DAC_Channel_2);

  DAC_Config(DAC_Channel_1);
  DAC_Config(DAC_Channel_2);

  DAC_TIM_Config(); 
  Strobe_TIM_Config();
}
