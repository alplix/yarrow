/* Minimal register map for the WCH CH32F203 (STM32F1-style peripherals). */
#ifndef YARROW_REGS_H
#define YARROW_REGS_H

#include <stdint.h>

#define REG(addr) (*(volatile uint32_t *)(addr))
#define REG16(addr) (*(volatile uint16_t *)(addr))

/* clocks */
#define RCC_BASE    0x40021000u
#define RCC_CR      REG(RCC_BASE + 0x00)
#define RCC_CFGR    REG(RCC_BASE + 0x04)
#define RCC_APB2ENR REG(RCC_BASE + 0x18)
#define RCC_APB1ENR REG(RCC_BASE + 0x1C)
#define RCC_BDCR    REG(RCC_BASE + 0x20)
#define RCC_CSR     REG(RCC_BASE + 0x24)

#define IWDG_KR     REG(0x40003000u)
#define IWDG_PR     REG(0x40003004u)
#define IWDG_RLR    REG(0x40003008u)
#define IWDG_SR     REG(0x4000300Cu)

#define FLASH_ACR   REG(0x40022000u)
#define FLASH_KEYR  REG(0x40022004u)
#define FLASH_SR    REG(0x4002200Cu)
#define FLASH_CR    REG(0x40022010u)
#define FLASH_AR    REG(0x40022014u)

#define AFIO_MAPR    REG(0x40010004u)
#define AFIO_EXTICR1 REG(0x40010008u)

#define GPIOA 0x40010800u
#define GPIOB 0x40010C00u
#define GPIOC 0x40011000u
#define GPIO_CRL(p)  REG((p) + 0x00)
#define GPIO_CRH(p)  REG((p) + 0x04)
#define GPIO_IDR(p)  REG((p) + 0x08)
#define GPIO_BSRR(p) REG((p) + 0x10)

#define EXTI_IMR  REG(0x40010400u)
#define EXTI_RTSR REG(0x40010408u)
#define EXTI_FTSR REG(0x4001040Cu)
#define EXTI_PR   REG(0x40010414u)

#define SPI1_BASE 0x40013000u
#define SPI1_CR1  REG(SPI1_BASE + 0x00)
#define SPI1_SR   REG(SPI1_BASE + 0x08)
#define SPI1_DR   REG(SPI1_BASE + 0x0C)

/* TIM5 (tube high voltage PWM on PA2 = channel 3) */
#define TIM5_BASE 0x40000C00u
#define TIM5_CR1   REG(TIM5_BASE + 0x00)
#define TIM5_EGR   REG(TIM5_BASE + 0x14)
#define TIM5_CCMR2 REG(TIM5_BASE + 0x1C)
#define TIM5_CCER  REG(TIM5_BASE + 0x20)
#define TIM5_PSC   REG(TIM5_BASE + 0x28)
#define TIM5_ARR   REG(TIM5_BASE + 0x2C)
#define TIM5_CCR3  REG(TIM5_BASE + 0x3C)

/* ADC1 */
#define ADC1_BASE 0x40012400u
#define ADC1_SR    REG(ADC1_BASE + 0x00)
#define ADC1_CR2   REG(ADC1_BASE + 0x08)
#define ADC1_SMPR1 REG(ADC1_BASE + 0x0C)
#define ADC1_SMPR2 REG(ADC1_BASE + 0x10)
#define ADC1_SQR1  REG(ADC1_BASE + 0x2C)
#define ADC1_SQR3  REG(ADC1_BASE + 0x34)
#define ADC1_DR    REG(ADC1_BASE + 0x4C)

/* power, backup domain, RTC */
#define PWR_CR     REG(0x40007000u)
#define BKP_DR1    REG(0x40006C04u)
#define RTC_CRL    REG(0x40002804u)
#define RTC_PRLH   REG(0x40002808u)
#define RTC_PRLL   REG(0x4000280Cu)
#define RTC_CNTH   REG(0x40002818u)
#define RTC_CNTL   REG(0x4000281Cu)

/* Cortex-M */
#define SYST_CSR  REG(0xE000E010u)
#define SYST_RVR  REG(0xE000E014u)
#define SYST_CVR  REG(0xE000E018u)
#define NVIC_ISER0 REG(0xE000E100u)
#define NVIC_ICER0 REG(0xE000E180u)
#define NVIC_IPR(n) (*(volatile uint8_t *)(0xE000E400u + (n)))
#define SCB_SHPR3_SYSTICK (*(volatile uint8_t *)0xE000ED23u)

#endif
