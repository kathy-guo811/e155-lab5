#include "encoder.h"
#include "STM32L432KC_GPIO.h"
#include "STM32L432KC_TIM.h"
#include "stm32l432xx.h"

// variables to compute motor velocity
volatile int encoder_count = 0;
volatile int previous_state;

// variables to compute encoder velocity at low speeds
volatile uint32_t encoder_period = 0;
volatile uint32_t last_edge_time = 0;
volatile int encoder_direction = 0;

int getEncoderCount(void) {
    return encoder_count;
}

uint32_t getEncoderPeriod(void) {
    return encoder_period;
}

uint32_t getLastEdgeTime(void) {
    return last_edge_time;
}

int getEncoderDirection(void) {
    return encoder_direction;
}

void initEncoder(void) {
    gpioEnable(GPIO_PORT_A); // Enable GPIO Port A
    pinMode(PA6, GPIO_INPUT); // Set PA6 as input for encoder channel A
    pinMode(PA9, GPIO_INPUT); // Set PA9 as input for encoder channel B

    // Initialize TIM2 for measuring encoder velocity
    RCC->APB1ENR1 |= (1 << TIM2EN);
    initTIM(TIM2);

    // Enable SYSCFG clock domain in RCC
    RCC->APB2ENR |= (1 << 0); // SYSCFGEN

    // EXTI6 is bits 10:8 of EXTICR2 (EXTICR[1] in C). Port A is 0b000, so clearing the field selects PA6.
    SYSCFG->EXTICR[1] &= ~(0b111 << EXTI6);

    // EXTI9 is bits 6:4 of EXTICR3 (EXTICR[2] in C). Port A is 0b000, so clearing the field selects PA9.
    SYSCFG->EXTICR[2] &= ~(0b111 << EXTI9);

    // Configure interrupts for both edges of PA6
    EXTI->IMR1 |= (1 << 6);       // 1. Configure mask bit
    EXTI->RTSR1 |= (1 << 6);      // 2. Enable rising edge trigger
    EXTI->FTSR1 |= (1 << 6);      // 3. Enable falling edge trigger
    
    // Configure interrupts for both edges of PA9
    EXTI->IMR1 |= (1 << 9);       // 1. Configure mask bit
    EXTI->RTSR1 |= (1 << 9);      // 2. Enable rising edge trigger
    EXTI->FTSR1 |= (1 << 9);      // 3. Enable falling edge trigger

    // Turn on EXTI interrupt in NVIC_ISER (EXTI9_5 is IRQ 23)
    NVIC->ISER[0] |= (1 << 23); 

    // Read initial state of encoder channels
    previous_state = (digitalRead(PA6) << 1) | digitalRead(PA9); 

    last_edge_time = getTime(TIM2); // Initialize last edge time

    // Enable interrupts globally
    __enable_irq();
    
}

void EXTI9_5_IRQHandler(void) {

    int A = digitalRead(PA6);
    int B = digitalRead(PA9);
    int current_state = (A << 1) | B;

    if (EXTI->PR1 & (1 << 6)) {
        EXTI->PR1 = (1 << 6);
    }

    if (EXTI->PR1 & (1 << 9)) {
        EXTI->PR1 = (1 << 9);
    }

    int valid_transition = 0; // default to 0, set to 1 if a valid quadrature transition occurs

    if (previous_state == 0b00) {
        if (current_state == 0b01) {
            encoder_count++;
            valid_transition = 1;
            encoder_direction = 1;
        } 
        else if (current_state == 0b10) {
            encoder_count--;
            valid_transition = 1;
            encoder_direction = -1;
        }
    } 
    else if (previous_state == 0b01) {
        if (current_state == 0b11) {
            encoder_count++;
            valid_transition = 1;
            encoder_direction = 1;
        } 
        else if (current_state == 0b00) {
            encoder_count--;
            valid_transition = 1;
            encoder_direction = -1;
        }
    } 
    else if (previous_state == 0b11) {
        if (current_state == 0b10) {
            encoder_count++;
            valid_transition = 1;
            encoder_direction = 1;
        } 
        else if (current_state == 0b01) {
            encoder_count--;
            valid_transition = 1;
            encoder_direction = -1;
        }
    } 
    else if (previous_state == 0b10) {
        if (current_state == 0b00) {
            encoder_count++;
            valid_transition = 1;
            encoder_direction = 1;
        } 
        else if (current_state == 0b11) {
            encoder_count--;
            valid_transition = 1;
            encoder_direction = -1;
        }
    }

    previous_state = current_state;

    // for low-velocity detection and calculation
    if (valid_transition) {
        uint32_t current_edge_time = getTime(TIM2);
        encoder_period = current_edge_time - last_edge_time;

        last_edge_time = current_edge_time;
        valid_transition = 0; // Reset valid transition flag
    }
}


