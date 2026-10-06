#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

#define EXTI6 8
#define EXTI9 4
#define TIM2EN 0
#define ENCODER_PULSES_PER_REV 408
#define QUADRATURE_MULTIPLIER  4
#define COUNTS_PER_REV (ENCODER_PULSES_PER_REV * QUADRATURE_MULTIPLIER)

void initEncoder(void);
int getEncoderCount(void);
uint32_t getEncoderPeriod(void);
uint32_t getLastEdgeTime(void);
int getEncoderDirection(void);


#endif // ENCODER_H