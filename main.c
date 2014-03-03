/* Includes ------------------------------------------------------------------*/
#include "stm32f4_discovery.h"
#include "misc.h"

#include "voice.h"
#include "counter.h"

/* Private define ------------------------------------------------------------*/
#define DAC_DHR12R1_ADDRESS 0x40007408
#define DAC_DHR12R2_ADDRESS 0x40007414

#define SINE_SIZE 32
#define RAMP_SIZE 6

#define BUFFER_SIZE 16*3

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
volatile uint16_t buffer1[BUFFER_SIZE*2];
volatile uint16_t buffer2[BUFFER_SIZE*2];
volatile uint32_t ptr1 = 0;
volatile uint32_t ptr2 = 0;

struct voice_t voice1;
struct voice_t voice2;
struct voice_t voice3;
struct counter_t sh_counter;

const uint16_t sine[SINE_SIZE] = {
                      2047, 2447, 2831, 3185, 3498, 3750, 3939, 4056, 4095, 4056,
                      3939, 3750, 3495, 3185, 2831, 2447, 2047, 1647, 1263, 909, 
                      599, 344, 155, 38, 0, 38, 155, 344, 599, 909, 1263, 1647};

const uint16_t ramp[RAMP_SIZE] = {0x0<<4, 0x33<<4, 0x66<<4, 0x99<<4, 0xCC<<4, 0xFF<<4};

/* Private typedef -----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void DAC_TIM_Config(void);
static void Strobe_TIM_Config(void);

static void DAC_Config(uint32_t dac_channel);
static void DMA_Config(DMA_Stream_TypeDef *stream, volatile uint16_t *buffer_top, volatile uint16_t *buffer_bottom, uint32_t buffer_size, uint32_t dac_channel);

uint32_t strobecnt(void);
TIM_TypeDef *DAC_TIM = TIM4;

/* Private functions ---------------------------------------------------------*/

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
  voice_init(&voice1, sine, SINE_SIZE, 2);
  voice_init(&voice2, sine, SINE_SIZE, 3);
  voice_init(&voice3, sine, SINE_SIZE, 3);

  counter_init(&sh_counter, 4);

  /* Preconfiguration before using DAC----------------------------------------*/
  GPIO_InitTypeDef GPIO_InitStructure;

  /* DMA1 clock and GPIOA clock enable (to be used with DAC) */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1 | RCC_AHB1Periph_GPIOA, ENABLE);

  /* DAC Periph clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

  /* DAC channel 1 & 2 (DAC_OUT1 = PA.4)(DAC_OUT2 = PA.5) configuration */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
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
  DMA_Config(DMA1_Stream6, &buffer1[0], &buffer1[BUFFER_SIZE], BUFFER_SIZE, DAC_Channel_1);
  DMA_Config(DMA1_Stream5, &buffer2[0], &buffer2[BUFFER_SIZE], BUFFER_SIZE, DAC_Channel_2);

  DAC_Config(DAC_Channel_1);
  DAC_Config(DAC_Channel_2);

  DAC_TIM_Config(); 
  Strobe_TIM_Config();

  while(1) {
    if(ptr1<BUFFER_SIZE) {
      //channel1
      buffer1[(!DMA_GetCurrentMemoryTarget(DMA1_Stream6))*BUFFER_SIZE+ptr1] = voice_nextSample(&voice1);
      ptr1++;
      //channel2
      buffer1[(!DMA_GetCurrentMemoryTarget(DMA1_Stream6))*BUFFER_SIZE+ptr1] = voice_nextSample(&voice2);
      ptr1++;
      //channel3
      buffer1[(!DMA_GetCurrentMemoryTarget(DMA1_Stream6))*BUFFER_SIZE+ptr1] = 0;
      ptr1++;
    }
    if(ptr2<BUFFER_SIZE) {
      buffer2[(!DMA_GetCurrentMemoryTarget(DMA1_Stream5))*BUFFER_SIZE+ptr2] = voice_nextSample(&voice3);
      ptr2++;
      buffer2[(!DMA_GetCurrentMemoryTarget(DMA1_Stream5))*BUFFER_SIZE+ptr2] = 0;
      ptr2++;
      buffer2[(!DMA_GetCurrentMemoryTarget(DMA1_Stream5))*BUFFER_SIZE+ptr2] = 0;
      ptr2++;
    }
  }
}

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
  DMA_InitStructure.DMA_PeripheralBaseAddr = ((dac_channel == DAC_Channel_1) ? DAC_DHR12R1_ADDRESS : DAC_DHR12R2_ADDRESS);

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
  TIM_TimeBaseStructure.TIM_Period = 0x800-1;
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
  TIM_TimeBaseStructure.TIM_Period = 0x800-1;
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; 
  TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);

  TIM_OCStructInit(&TIM_OCInitStructure);
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = 0x100;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; 
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

