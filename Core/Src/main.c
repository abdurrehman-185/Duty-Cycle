#include "stm32l4xx_hal.h"
#include <stdio.h>
#include <stdlib.h>

void Error_Handler(void);
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_TIM4_Init(uint32_t pwm_frequency, uint8_t duty_cycle);
static uint32_t Calculate_Period(uint32_t pwm_frequency);
void HAL_TIM_MspPostInit(TIM_HandleTypeDef* htim);

#define TIM_PRESCALAR (7999)

UART_HandleTypeDef hlpuart1;
TIM_HandleTypeDef htim4;

char uart_buf[100];
int uart_buf_len;
char input[10];
uint32_t pwm_frequency = 0;
uint32_t arr_value;
uint32_t pulse_value;
uint8_t duty_cycle = 0;

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_LPUART1_UART_Init();

    while (1)
    {
        // Prompt user for PWM frequency
        uart_buf_len = sprintf(uart_buf, "Enter PWM Frequency (Hz):\r\n");
        HAL_UART_Transmit(&hlpuart1, (uint8_t*)uart_buf, uart_buf_len, HAL_MAX_DELAY);

        int count = 0;
        while(count < 10) {
            char ch;
            HAL_UART_Receive(&hlpuart1, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
            if (ch == '\r')
                break;
            input[count] = ch;
            count++;
        }
        input[count] = '\0';  // Null-terminate the string
        // 10
        // freq=10
        // duty=100
        //string parsing in C

        pwm_frequency = atoi(input);

        // duty cycle
        uart_buf_len = sprintf(uart_buf, "Enter Duty Cycle (%%):\r\n");
        HAL_UART_Transmit(&hlpuart1, (uint8_t*)uart_buf, uart_buf_len, HAL_MAX_DELAY);

        count = 0;
        while(count < 10) {
            char ch;
            HAL_UART_Receive(&hlpuart1, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
            if (ch == '\r')
                break;
            input[count] = ch;
            count++;
        }
        input[count] = '\0';  // Null-terminate the string
        duty_cycle = atoi(input);

        arr_value = Calculate_Period(pwm_frequency);

        pulse_value = (arr_value * duty_cycle) / 100;

        uart_buf_len = sprintf(uart_buf, "Value of ARR/Period is %lu\r\n", arr_value);
        HAL_UART_Transmit(&hlpuart1, (uint8_t*)uart_buf, uart_buf_len, HAL_MAX_DELAY);

        uart_buf_len = sprintf(uart_buf, "Pulse value is %lu for %u%% duty cycle\r\n", pulse_value, duty_cycle);
        HAL_UART_Transmit(&hlpuart1, (uint8_t*)uart_buf, uart_buf_len, HAL_MAX_DELAY);

        HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_2);
        MX_TIM4_Init(pwm_frequency, duty_cycle);

        HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);

        uart_buf_len = sprintf(uart_buf, "PWM Frequency is: %lu Hz, Duty Cycle is: %u%%\r\n", pwm_frequency, duty_cycle);
        HAL_UART_Transmit(&hlpuart1, (uint8_t*)uart_buf, uart_buf_len, HAL_MAX_DELAY);
    }
}

static uint32_t Calculate_Period(uint32_t pwm_frequency)
{
    uint32_t period = (80000000 / (pwm_frequency * (TIM_PRESCALAR + 1))) - 1;
    return period;
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 1;
    RCC_OscInitStruct.PLL.PLLN = 10;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
    {
        Error_Handler();
    }
}

static void MX_GPIO_Init(void)
{
    /* USER CODE BEGIN MX_GPIO_Init 0 */

    /* USER CODE END MX_GPIO_Init 0 */

    // Enable GPIO Ports Clock
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Configure GPIO pins : PG7 PG8
    GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF8_LPUART1;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    /* USER CODE BEGIN MX_GPIO_Init 1 */

    /* USER CODE END MX_GPIO_Init 1 */
}

// LPUART1 Initialization Function
static void MX_LPUART1_UART_Init(void)
{
    /* USER CODE BEGIN MX_LPUART1_UART_Init 0 */

    /* USER CODE END MX_LPUART1_UART_Init 0 */

    hlpuart1.Instance = LPUART1;
    hlpuart1.Init.BaudRate = 115200;
    hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
    hlpuart1.Init.StopBits = UART_STOPBITS_1;
    hlpuart1.Init.Parity = UART_PARITY_NONE;
    hlpuart1.Init.Mode = UART_MODE_TX_RX;
    hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&hlpuart1) != HAL_OK)
    {
        Error_Handler();
    }
}

static void MX_TIM4_Init(uint32_t pwm_frequency, uint8_t duty_cycle)
{
    TIM_OC_InitTypeDef sConfigOC = {0};
    uint32_t prescaler = TIM_PRESCALAR;
    uint32_t arr_value = Calculate_Period(pwm_frequency);
    uint32_t pulse_value = (arr_value * duty_cycle) / 100;

    htim4.Instance = TIM4;
    htim4.Init.Prescaler = prescaler;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = arr_value;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
    {
        Error_Handler();
    }

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.Pulse = pulse_value;

    if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
    {
        Error_Handler();
    }

}
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    while (1)
    {
        // Stay in loop
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
