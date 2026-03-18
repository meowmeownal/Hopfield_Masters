#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "tanh_lut.h"
#include "sin_lut.h"
#include "bg_lut.h"

#ifdef MCU
    #include "main.h"
    #include "stm32f4xx.h"
#endif

//common functions and variaables

#define N 5
#define T 3000

#ifdef MCU

    #define DAC1_ADDR (0x60 << 1) //1st dac adress GND,  0x60 is default adress for this specific DAC
    #define DAC2_ADDR (0x61 << 1) //second dac adress V_ref

    I2C_HandleTypeDef hi2c1;
    UART_HandleTypeDef huart2;
    DMA_HandleTypeDef hdma_usart2_tx;
    void SystemClock_Config(void);
    static void MX_GPIO_Init(void);
    static void MX_DMA_Init(void);
    static void MX_USART2_UART_Init(void);
    static void MX_I2C1_Init(void);


    void log_result(float a2, float y0, float y1, float y2, float y3, float y4);
    void flush_batch(void);
    void uart_dma(void);
#endif

static float Y0[T+1]; // = {0.8f};
static float Y1[T+1]; // = {0.3f};
static float Y2[T+1]; // = {0.4f};
static float Y3[T+1]; //= {0.6f};
static float Y4[T+1]; // = {0.7f};


//------------------------------------------------

//----------- G1 and G2 functions ----------------
float G1(float y, float a1) {
    return 1.0 - a1 * tanhf(y);
}

float G2(float y, float a2, float a3) {
    return a2 - a3 * sinf(y);


}
//-----------------------------------------------

static inline float fast_sin(float x)
{
    // reduction t0 [0, 2pi]
    while (x < 0.0f)       x += 2.0f * M_PI;
    while (x >= 2.0f*M_PI) x -= 2.0f * M_PI;

    float t = (x - SIN_MIN) * (SIN_N - 1) / (SIN_MAX - SIN_MIN);
    uint16_t i = (int)t;
    float f = t - i;

    return SIN_LUT[i] * (1.0f - f) + SIN_LUT[i + 1] * f;
}

static inline float fast_tanh(float x)
{
    if (x <= TANH_MIN) return -1.0f;
    if (x >= TANH_MAX) return  1.0f;

    float t = (x - TANH_MIN) * (TANH_N - 1) / (TANH_MAX - TANH_MIN);
    uint16_t i = (int)t;
    float f = t - i;

    return TANH_LUT[i] * (1.0f - f) + TANH_LUT[i + 1] * f;
}


//----------------------------------------------


//------------------saving data on MCU-----------------------
#ifdef MCU

    typedef struct
    {
        float y[6];
    } DATA;

    #define DATA_BATCH_SIZE 32
    DATA data_buffer[DATA_BATCH_SIZE];
    uint32_t data_buffer_count = 0;
    volatile uint8_t uart_tx_ready = 1;


    void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
        if (huart->Instance == USART2) {
            uart_tx_ready = 1;
        }
    }


    //sending sample to buffor
    void send_sample(float y0, float y1, float y2, float y3, float y4, uint32_t dt)
    {

        data_buffer[data_buffer_count].y[0] = y0;
        data_buffer[data_buffer_count].y[1] = y1;
        data_buffer[data_buffer_count].y[2] = y2;
        data_buffer[data_buffer_count].y[3] = y3;
        data_buffer[data_buffer_count].y[4] = y4;
        data_buffer[data_buffer_count].y[5] = dt; //time of single step calculation

        data_buffer_count++;

        if (data_buffer_count >= DATA_BATCH_SIZE && uart_tx_ready)
        {
            uart_tx_ready = 0; //DMA working
            HAL_UART_Transmit_DMA(&huart2, (uint8_t*)data_buffer,
                                DATA_BATCH_SIZE * sizeof(DATA));
            data_buffer_count = 0;
        }
    }

    //sending remaining samples that didnt fit into multiple of batch
    void flush_samples(void) {
        if (data_buffer_count > 0 && uart_tx_ready) {
            uart_tx_ready = 0;
            HAL_UART_Transmit_DMA(&huart2, (uint8_t*)data_buffer,
                                data_buffer_count * sizeof(DATA));
            data_buffer_count = 0;
        }
    }
#endif
//----------------------------------------------------------


//--------------DAC DATA CONVERTION----------------------------
#ifdef MCU

    uint16_t dac_value(float y,  float y_min,  float y_max)
    {



    	y_max = 10*y_max;
    	y_min = 10*y_min;


		if(fabsf(y_max - y_min) < 0.4f)
		{
			y_max += 0.2f;
			y_min -= 0.2f;
		}

		float range = (y_max - y_min);

    	y_min -= 0.25f * range;
    	y_max += 0.25f * range;



//-------------------------------------

 //-----------------------------------------

    	float normalized = (y - y_min) / (y_max - y_min);

        if(normalized < 0.0f) normalized = 0.0f;
        if(normalized > 1.0f) normalized = 1.0f;

        return (uint16_t)(normalized * 4095.0f);
    }

//    float y0_min,y0_max;
//    float y1_min,y1_max;
//    float y2_min,y2_max;
//    float y3_min,y3_max;
//    float y4_min,y4_max;
//
//    float yx_min,yx_max;
//
//	uint16_t dac_value(float y, uint16_t om, float *y_min, float *y_max)
//	{
//
//		if(om ==1)
//		{
//			*y_min = y - 0.1*y;
//			*y_max = y;
//		}
//
//	    if(om <1 && om < 200)
//	    {
//
//	    	float range = fabsf(*y_max - *y_min);
//	        if(y < *y_min) *y_min = y - 0.25f * range;
//	        if(y > *y_max) *y_max = y + 0.25f * range;
//
////			*y_min -= 0.25f * range;
////			*y_max += 0.25f * range;
//
//	        if(fabsf(*y_max - *y_min) < 0.3f)
//	        {
//	            *y_max += 0.1f;
//	            *y_min -= 0.1f;
//	        }
//	    }
//
//	    float normalized = (y - *y_min) / (*y_max - *y_min);
//
//	    if(normalized < 0.0f) normalized = 0.0f;
//	    if(normalized > 1.0f) normalized = 1.0f;
//
//	    return (uint16_t)(normalized * 4095.0f);
//	}



    void two_bytes_data(uint8_t data[], uint16_t y_data)
    {

        data[0] = (y_data >> 8) & 0x0F; // 4 older bits
        data[1] = y_data & 0xFF;

    }

#endif
//--------------------------------------------------------------



#ifdef MCU
void simulate_realtime(float a2, float a3, float a1,
                      float const weights[N][N], float f1, float f3,
                      float ni,
                      float *restrict y0, float *restrict y1,
                      float *restrict y2, float *restrict y3,
                      float *restrict y4)
#else
void simulate_realtime(float a2, float a3, float a1,
                      float const weights[N][N], float f1, float f3,
                      float ni,
                      float *restrict y0, float *restrict y1,
                      float *restrict y2, float *restrict y3,
                      float *restrict y4,
                      FILE *fp)
#endif

{
    #ifdef MCU
        send_sample(y0[0], y1[0], y2[0], y3[0], y4[0], 0.0);
    #endif


     y0[0] = 0.8;
     y1[0] = 0.3;
     y2[0] = 0.4;
     y3[0] = 0.6;
     y4[0] = 0.7;


    for (uint16_t om = 1; om < T+1; om++)
    {
		y0[om]=0;
		y1[om]=0;
		y2[om]=0;
		y3[om]=0;
		y4[om]=0;

        #ifdef MCU
            uint32_t start_ms = HAL_GetTick();
        #endif


        for (uint16_t r = 1; r < om + 1; r++)
        {
        	const float bg = BG_LUT[om-r];
            y0[om] += (-y0[r-1] + weights[0][1] * fast_tanh(y1[r-1]) + weights[0][2] * fast_sin(y2[r-1]) + weights[0][3] * fast_sin(y3[r-1])) * bg;
            y1[om] += (-y1[r-1] + weights[1][0] * fast_sin(y0[r-1]) + weights[1][2] * fast_sin(y2[r-1]) + weights[1][4] * fast_sin(y4[r-1]) + f1) * bg;
            y2[om] += (-y2[r-1] + weights[2][0] * fast_tanh(y0[r-1]) + weights[2][1] * fast_tanh(y1[r-1]) + weights[2][2] * fast_sin(y2[r-1])) * bg;
            y3[om] += (-y3[r-1] + weights[3][0] * fast_tanh(y0[r-1]) + G2(y4[r-1],a2,a3) * fast_tanh(y3[r-1]) + f3) * bg;
            y4[om] += (-y4[r-1] + weights[4][1] * fast_tanh(y1[r-1]) + G1(y2[r-1],a1) * fast_tanh(y4[r-1])) * bg;


        }

        y0[om] += y0[0];
        y1[om] += y1[0];
        y2[om] += y2[0];
        y3[om] += y3[0];
        y4[om] += y4[0];

        #ifdef MCU

            uint32_t delay_step = 1; //by how many steps delayed
            uint32_t end_ms = HAL_GetTick();
            uint32_t dt = end_ms - start_ms;

            send_sample(y0[om], y1[om], y2[om], y3[om], y4[om], dt);

            if (om % 100 == 0) {
                HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            }

            //converting data to voltage 0 - 3.3V
            uint16_t y0_data = dac_value(y0[om]*10, 0.55, 0.62); //moze jakies saklowanie danych, ktore maja mała roznice, np
            uint16_t y1_data = dac_value(y1[om]*10,  -4.05, -3.8);
            uint16_t y2_data = dac_value(y2[om]*10, -1.85, -1.55);
            uint16_t y3_data = dac_value(y3[om]*10, 7.306, 7.316);
            uint16_t y4_data = dac_value(y4[om]*10, -1.05, -0.85);

            uint16_t yph1_data = dac_value(y3[om]*y2[om]*10, -13.1, -11.5);


//            uint16_t yph2_data = dac_value(y0[om]*y1[om], 0.583*-3.953, 0.586*-3.950);


            uint8_t data0[2] = {0};
            uint8_t data1[2] = {0};
            uint8_t data2[2] = {0};
            uint8_t data3[2] = {0};
            uint8_t data4[2] = {0};

            uint8_t data_ph1[2] = {0};

            two_bytes_data(data0, y0_data);
            two_bytes_data(data1, y1_data);
            two_bytes_data(data2, y2_data);
            two_bytes_data(data3, y3_data);
            two_bytes_data(data4, y4_data);

            two_bytes_data(data_ph1, yph1_data);
            HAL_I2C_Master_Transmit(&hi2c1, DAC1_ADDR, data_ph1, 2, HAL_MAX_DELAY);

            //------------------delayed ----------------------
            if(om  >= 1 + delay_step)
            {

                uint8_t data_delay[2] = {0};
                uint16_t y_delayed = dac_value(y0[om - delay_step]*y1[om - delay_step]*10, -2.34, -2.2);

                //uint16_t y_delayed = dac_value(y4[om - delay_step]*10, -1.05, -0.85);

                two_bytes_data(data_delay,  y_delayed);
                HAL_I2C_Master_Transmit(&hi2c1, DAC2_ADDR, data_delay, 2, HAL_MAX_DELAY);

            }
            //----------------------------------------------
            //HAL_I2C_Master_Transmit(&hi2c1, DAC2_ADDR, data1, 2, HAL_MAX_DELAY); //przedostatni argument to ilosc wysylanych bajtów, wysyla sie 2 wiec chyba trzeba mzienic

        #endif



    }

    #ifndef MCU
        //for(int i = T + 1 - 1000; i < T + 1; i++)
        for(int i = 0 ; i < T + 1; i++)  //if i = 0, will show initial vals
        {
            //int y0_data = dac_value(y0[i]);
            fprintf(fp,"%0.10lf, %0.10lf, %0.10lf, %0.10lf, %0.10lf\n", y0[i], y1[i], y2[i], y3[i], y4[i]); //, y0_data); %d\n"
        }

    #endif


    #ifdef MCU
        flush_samples();
    #else
        fclose(fp);
    #endif
}



int main()
{


    #ifdef MCU
        HAL_Init();
        SystemClock_Config();
        MX_GPIO_Init();
        MX_DMA_Init();
        MX_USART2_UART_Init();
        MX_I2C1_Init();

        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        HAL_Delay(1000);

    #endif

    #ifndef MCU
        FILE *fp = fopen("testMACRO_float_f_338.csv", "w");
    #endif

    const float f1 = 0.0;
    const float f3 = 0.0;
    const float a1 = -2.2;
    const float a2 = 6.02; //changing that /-3.38
    const float a3 = 1.2;

    const float weights[N][N] = {
        { 0.0, -0.3, -0.8, -0.6,  0.0 },
        {-3.0,  0.0,  2.0,  0.0,  0.4 },
        { 1.7, -0.4,  3.0,  0.0,  0.0 },
        { 0.7,  0.0,  0.0,  0.0,  0.0 },
        { 0.0,  1.7,  0.0,  0.0,  0.0 }
    };


    for(uint16_t i = 0; i <= T; i++)
    {
    	Y4[i] = 0.0f;
    	Y3[i] = 0.0f;
    	Y2[i] = 0.0f;
    	Y1[i] = 0.0f;
    	Y0[i] = 0.0f;
    }
    Y4[0] = 0.7f;
    Y3[0] = 0.6f;
    Y2[0] = 0.4f;
    Y1[0] = 0.3f;
    Y0[0] = 0.8f;

    #ifdef MCU
        simulate_realtime(a2, a3, a1, weights, f1, f3, 0.7, Y0, Y1, Y2, Y3, Y4);
    #else
        simulate_realtime(a2, a3, a1, weights, f1, f3, 0.7, Y0, Y1, Y2, Y3, Y4, fp);
    #endif


    #ifdef MCU
        HAL_Delay(100);

    while (1)
    {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        HAL_Delay(100);
    }
    #endif

}

#ifdef MCU
    void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
        HAL_UART_DeInit(huart);
        HAL_UART_Init(huart);
        char msg[] = "UART reset\r\n";
        HAL_UART_Transmit(huart, (uint8_t*)msg, strlen(msg), 100);
    }
    /* USER CODE END 4 */
    /**
    * @brief System Clock Configuration
    * @retval None
    */
    void SystemClock_Config(void)
    {
        RCC_OscInitTypeDef RCC_OscInitStruct = {0};
        RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

        /** Configure the main internal regulator output voltage
        */
        __HAL_RCC_PWR_CLK_ENABLE();
        __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

        /** Initializes the RCC Oscillators according to the specified parameters
        * in the RCC_OscInitTypeDef structure.
        */
        RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
        RCC_OscInitStruct.HSIState = RCC_HSI_ON;
        RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
        RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
        if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
        {
            Error_Handler();
        }

        /** Initializes the CPU, AHB and APB buses clocks
        */
        RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                    |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
        RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
        RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
        RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
        RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

        if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
        {
            Error_Handler();
        }
    }

    static void MX_I2C1_Init(void)
    {

        /* USER CODE BEGIN I2C1_Init 0 */

        /* USER CODE END I2C1_Init 0 */

        /* USER CODE BEGIN I2C1_Init 1 */

        /* USER CODE END I2C1_Init 1 */
        hi2c1.Instance = I2C1;
        hi2c1.Init.ClockSpeed = 100000;
        hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
        hi2c1.Init.OwnAddress1 = 0;
        hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
        hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
        hi2c1.Init.OwnAddress2 = 0;
        hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
        hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
        if (HAL_I2C_Init(&hi2c1) != HAL_OK)
        {
            Error_Handler();
        }
        /* USER CODE BEGIN I2C1_Init 2 */

        /* USER CODE END I2C1_Init 2 */

    }

    /**
    * @brief USART2 Initialization Function
    * @param None
    * @retval None
    */
    static void MX_USART2_UART_Init(void)
    {

    /* USER CODE BEGIN USART2_Init 0 */

    /* USER CODE END USART2_Init 0 */

    /* USER CODE BEGIN USART2_Init 1 */

    /* USER CODE END USART2_Init 1 */
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 921600;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN USART2_Init 2 */

    /* USER CODE END USART2_Init 2 */

    }

    /**
    * Enable DMA controller clock
    */
    static void MX_DMA_Init(void)
    {

    /* DMA controller clock enable */
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* DMA interrupt init */
    /* DMA1_Stream6_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);

    }

    /**
    * @brief GPIO Initialization Function
    * @param None
    * @retval None
    */
    static void MX_GPIO_Init(void)
    {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* USER CODE BEGIN MX_GPIO_Init_1 */

    /* USER CODE END MX_GPIO_Init_1 */

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

    /*Configure GPIO pin : PA5 */
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USER CODE BEGIN MX_GPIO_Init_2 */

    /* USER CODE END MX_GPIO_Init_2 */
    }

    /* USER CODE BEGIN 4 */

    /* USER CODE END 4 */

    /**
    * @brief  This function is executed in case of error occurrence.
    * @retval None
    */
    void Error_Handler(void)
    {
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
    }
    #ifdef USE_FULL_ASSERT
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
#endif
