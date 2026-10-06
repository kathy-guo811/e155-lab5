// main.c
// Kathy Guo
// kaguo@g.hmc.edu
// 10/4/2026
// Implements interrupts to determin the speed of a motor by reading from a quadrature encoder.

// Necessary includes for printf to work
#include <stdio.h>
#include <stdint.h>
#include "stm32l432xx.h"
#include "main.h"
#include "encoder.h"


// Function used by printf to send characters to the laptop
int _write(int file, char *ptr, int len) {
  int i = 0;
  for (i = 0; i < len; i++) {
    ITM_SendChar((*ptr++));
  }
  return len;
}

int main(void) {

    configureClock(); // Set clock to 80 MHz
    initEncoder(); // Initialize encoder inputs and interrupts

    int previous_count = getEncoderCount(); // Initialize previous count
    uint32_t previous_time = getTime(TIM2); // Initialize previous time

    while(1) {
        uint32_t current_time = getTime(TIM2); // Read current time from TIM2
        uint32_t time_change = current_time - previous_time; // Calculate change in time
    
        if (time_change >= 1000) { // 1hz readouts
            int current_count = getEncoderCount(); // Read current encoder count
            int count_change = current_count - previous_count; // Calculate change in count
            
            uint32_t last_edge_time = getLastEdgeTime(); // Initialize last edge time
            uint32_t time_since_edge = current_time - last_edge_time; // Initialize time since last edge

            float velocity = 0.0f; // Initialize velocity
            

            // case 1: see if motor is spinning at all
            if (time_since_edge >= STOP_TIMEOUT_MS) {
                velocity = 0.0f;
                printf("Direction: Stopped, Velocity: %f rev/sec\n", velocity);
            }
            
            // case 2: see if motor is spinning slowly
            else if (count_change <= LOW_SPEED_COUNT_THRESHOLD && count_change >= -LOW_SPEED_COUNT_THRESHOLD) {
                
                uint32_t encoder_period = getEncoderPeriod(); // get encoder period
                int encoder_direction = getEncoderDirection(); // get encoder direction

                if (encoder_period > 0) {
                    velocity = 1.0f / ((float)encoder_period / 1000.0f * COUNTS_PER_REV); // Calculate velocity in rev/sec

                    if (encoder_direction > 0) {
                        printf("Direction: CCW, Velocity: %f rev/sec\n", velocity);
                    }
                    else if (encoder_direction < 0) {
                        printf("Direction: CW, Velocity: %f rev/sec\n", velocity);
                    }
                } 
                else {
                    printf("Direction: Stopped, Velocity: 0.000000 rev/sec\n");
                }
            }

            // case 3: motor is spinning fast enough to use count change
            else {
                velocity = (float)count_change / ((float)time_change / 1000.0f * COUNTS_PER_REV); // Calculate velocity in rev/sec

                if (velocity > 0) {
                    printf("Direction: CCW, Velocity: %f rev/sec\n", velocity);
                }

                else {
                    printf("Direction: CW, Velocity: %f rev/sec\n", -velocity);
                }
            }

            previous_count = current_count; // Update previous count for next iteration
            previous_time = current_time; // Update previous time for next iteration

        }
    }

    return 0;
}

// For polling 
int main(void) {

    configureClock();

    gpioEnable(GPIO_PORT_A);
    pinMode(PA6, GPIO_INPUT);
    pinMode(PA9, GPIO_INPUT);

    RCC->APB1ENR1 |= (1 << TIM2EN);
    initTIM(TIM2);

    int previous_state = (digitalRead(PA6) << 1) | digitalRead(PA9);
    int encoder_count = 0;
    uint32_t previous_time = getTime(TIM2);

    while (1) {

        int A = digitalRead(PA6);
        int B = digitalRead(PA9);

        int current_state = (A << 1) | B;

        if (current_state != previous_state) {

            if (previous_state == 0b00) {
                if (current_state == 0b01)
                    encoder_count++;
                else if (current_state == 0b10)
                    encoder_count--;
            }

            else if (previous_state == 0b01) {
                if (current_state == 0b11)
                    encoder_count++;
                else if (current_state == 0b00)
                    encoder_count--;
            }

            else if (previous_state == 0b11) {
                if (current_state == 0b10)
                    encoder_count++;
                else if (current_state == 0b01)
                    encoder_count--;
            }

            else if (previous_state == 0b10) {
                if (current_state == 0b00)
                    encoder_count++;
                else if (current_state == 0b11)
                    encoder_count--;
            }

            previous_state = current_state;
        }
        uint32_t current_time = getTime(TIM2);

        if (current_time - previous_time >= 10000) {

            printf("Count: %d\n", encoder_count);

            encoder_count = 0;
            previous_time = current_time;
        }
    }
}