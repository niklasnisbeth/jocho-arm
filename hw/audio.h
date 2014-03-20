#ifndef __AUDIO_H
#define __AUDIO_H

#define DAC_DHR12L1_ADDRESS 0x4000740C
#define DAC_DHR12L2_ADDRESS 0x40007418 

void audio_init(volatile uint16_t *buffer1, volatile uint16_t *buffer2, uint16_t bufsize);
extern struct counter_t sh_counter;

/*
void DMA_Config(DMA_Stream_TypeDef *stream, 
    volatile uint16_t *buffer_top, 
    volatile uint16_t *buffer_bottom, 
    uint32_t buffer_size, uint32_t dac_channel);

void DAC_TIM_Config(void);
void Strobe_TIM_Config(void);
void DAC_Config(uint32_t dac_channel); 
*/

#endif
