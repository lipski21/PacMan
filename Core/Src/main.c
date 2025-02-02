/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "crc.h"
#include "dma2d.h"
#include "i2c.h"
#include "ltdc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "fmc.h"
#include "stm32f4xx_hal.h"
#include "stm32f429i_discovery.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_ts.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */


#include "icons.h"
#include "../../Drivers/BSP/Components/l3gd20/l3gd20.h"
#include <math.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define X 12
#define Y 16
#define adxl_address 0x53<<1
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
LTDC_HandleTypeDef LtdcHandle;
uint16_t InitZyro = L3GD20_MODE_ACTIVE | L3GD20_OUTPUT_DATARATE_1 | L3GD20_X_ENABLE | L3GD20_Y_ENABLE;


/* Pictures position */
int16_t acc_x, acc_y, first_acc_x, first_acc_y;
uint8_t data_rec[6];
float DaneZyroskopu[3];
uint32_t Xpos1 = 0;
uint32_t Xpos2 = 0;
uint32_t Ypos1 = 220;
uint32_t Ypos2 = 160;
uint32_t ruchx = 0;
uint32_t ruchy = 0;
uint32_t szerokosc = 20;
uint32_t wysokosc = 20;
uint32_t next_move = 0;
uint32_t current_time = 0;
uint32_t highest_speed = 0;

int board[X][Y];
enum movement {UP, DOWN, LEFT, RIGHT, NONE};
enum gamestate {RUN, LOSE, WIN, STOP};

char c;
int headx=4, heady=4; //pozycja postaci gracza
enum movement MOVE= NONE;
enum movement MOVEGHOST= NONE;
enum movement last_move= NONE;
enum gamestate STATE=RUN;
int ghost1x=1, ghost1y=1; //pozycja ducha 1
int ghost2x=1, ghost2y=8; //pozycja ducha 2
int ghost3x=8, ghost3y=1; //pozycja ducha 3
int ghost4x=8, ghost4y=8; //pozycja ducha 4

//akcelerometr

void adxl_write(uint8_t reg, uint8_t value){
	uint8_t data[2];
	data[0] = reg;
	data[1] = value;
	HAL_I2C_Master_Transmit (&hi2c3, adxl_address, data, 2, 10);
}

void adxl_read(uint8_t reg, uint8_t){
	HAL_I2C_Mem_Read ( &hi2c3, adxl_address, reg, 1, &data_rec, 6, 100);

	acc_x = (data_rec[1]<<8 | data_rec[0]);
	acc_y = (data_rec[3]<<8 | data_rec[2]);
}


void adxl_init (void){
	adxl_read(0x00, 1);

	adxl_write(0x2d, 0);
	adxl_write(0x2d, 0x08);

	adxl_write(0x31, 0x01);
}

enum gamestate GetGameState(){
    // warunki przegranej (postac znajduje sie na tym samym polu co jakikolwiek duch)
    int coins=0;
    for(int i=0; i<X; i++){
        for(int j=0; j<Y;j++){
            if(board[i][j]==1)
                coins++;
        }
    }
    if(headx==ghost1x && heady==ghost1y)
        return LOSE;
    else if(headx==ghost2x && heady==ghost2y)
        return LOSE;
    else if(headx==ghost3x && heady==ghost3y)
        return LOSE;
    else if(headx==ghost4x && heady==ghost4y)
        return LOSE;
    //warunki wygranej (wszystkie "monetki zebrane")
    else if (coins==0)
        return WIN;

    else
        return RUN;
}

//domyslnie po prostu zapewne będziemy brać nie z rand() tylko z odczytu z zegarów mikroprocesora
enum movement RandomizeMovement(){
    int num =rand()%4;
    if (num==0)
        return UP;
    else if (num==1)
        return DOWN;
    else if (num==2)
        return LEFT;
    else if (num==3)
        return RIGHT;
    else
    return NONE;
}
void MoveGhosts(){
    bool moved =0;
    while(moved==0){
        MOVEGHOST=RandomizeMovement();

        if(MOVEGHOST==DOWN){
            if(ghost1x<X-1 && board[ghost1x+1][ghost1y]!=2){
                ghost1x++;
                moved=1;
            }
        }
        else if(MOVEGHOST==LEFT){
            if(ghost1y>0 && board[ghost1x][ghost1y-1]!=2){
                ghost1y--;
                moved=1;
            }
        }
        else if(MOVEGHOST==UP){
            if(ghost1x>0 && board[ghost1x-1][ghost1y]!=2){
                ghost1x--;
                moved=1;
            }
        }
        else if(MOVEGHOST==RIGHT){
            if(ghost1y<Y-1  && board[ghost1x][ghost1y+1]!=2){
                ghost1y++;
                moved=1;
            }
        }
        else
            moved=1;
    }
    moved = 0;
    while(moved==0){
        MOVEGHOST=RandomizeMovement();

        if(MOVEGHOST==DOWN){
            if(ghost2x<X-1 && board[ghost2x+1][ghost2y]!=2){
                ghost2x++;
                moved=1;
            }
        }
        else if(MOVEGHOST==LEFT){
            if(ghost2y>0 && board[ghost2x][ghost2y-1]!=2){
                ghost2y--;
                moved=1;
            }
        }
        else if(MOVEGHOST==UP){
            if(ghost2x>0 && board[ghost2x-1][ghost2y]!=2){
                ghost2x--;
                moved=1;
            }
        }
        else if(MOVEGHOST==RIGHT){
            if(ghost2y<Y-1  && board[ghost2x][ghost2y+1]!=2){
                ghost2y++;
                moved=1;
            }
        }
        else
            moved=1;
    }
    moved = 0;
    while(moved==0){
        MOVEGHOST=RandomizeMovement();

        if(MOVEGHOST==DOWN){
            if(ghost3x<X-1 && board[ghost3x+1][ghost3y]!=2){
                ghost3x++;
                moved=1;
            }
        }
        else if(MOVEGHOST==LEFT){
            if(ghost3y>0 && board[ghost3x][ghost3y-1]!=2){
                ghost3y--;
                moved=1;
            }
        }
        else if(MOVEGHOST==UP){
            if(ghost3x>0 && board[ghost3x-1][ghost3y]!=2){
                ghost3x--;
                moved=1;
            }
        }
        else if(MOVEGHOST==RIGHT){
            if(ghost3y<Y-1  && board[ghost3x][ghost3y+1]!=2){
                ghost3y++;
                moved=1;
            }
        }
        else
            moved=1;
    }
    moved = 0;
    while(moved==0){
        MOVEGHOST=RandomizeMovement();

        if(MOVEGHOST==DOWN){
            if(ghost4x<X-1 && board[ghost4x+1][ghost4y]!=2){
                ghost4x++;
                moved=1;
            }
        }
        else if(MOVEGHOST==LEFT){
            if(ghost4y>0 && board[ghost4x][ghost4y-1]!=2){
                ghost4y--;
                moved=1;
            }
        }
        else if(MOVEGHOST==UP){
            if(ghost4x>0 && board[ghost4x-1][ghost4y]!=2){
                ghost4x--;
                moved=1;
            }
        }
        else if(MOVEGHOST==RIGHT){
            if(ghost4y<Y-1  && board[ghost4x][ghost4y+1]!=2){
                ghost4y++;
                moved=1;
            }
        }
        else
            moved=1;
    }
}
//domyslnie bedzie w jakis sposob to kooperowac z zyroskopem
enum movement GetPlayerMove(){
//    printf("Wpisz na klawiature odpowiednia litere zeby wykonac ruch\n");
//    printf("1.W - idz do gory\n");
//    printf("2.S - idz w dol\n");
//    printf("3.A - idz w prawo\n");
//    printf("4.D - idz w lewo\n");
//    printf("5.N - stoj w miejscu\n");
//
//    if (scanf(" %c", &c) != 1) {
//        printf("Blad podczas wczytywania znaku.\n");
//        while (getchar() != '\n');
//        return NONE;
//    }
//    while (getchar() != '\n');
//
//    if(c=='W'||c=='w'){
//        return UP;
//    }
//    else if(c=='S'||c=='s'){
//        return DOWN;
//    }
//    else if(c=='A'||c=='a'){
//        return LEFT;
//    }
//    else if(c=='D'||c=='d'){
//        return RIGHT;
//    }
//    else if(c=='N'||c=='n'){
//        return NONE;
//    }
//    else{
//        printf("Bledne dane wejsciowe sprouj jeszcze raz\n");
//        c='n';
//        return NONE;
//    }
	L3GD20_ReadXYZAngRate(DaneZyroskopu);
	adxl_read(0x32, 6);

	// 1 = down 2 = up 3 = right 4 = left
					int xx = acc_x - first_acc_x;
					int yy = acc_y - first_acc_y;

				if(fabs(acc_y) > fabs(acc_x)){
					if(acc_y < 0){
						return DOWN;
					}
					else if(acc_y > 0){
						return UP;
					}
				}
				else if(acc_x < 0){
					return RIGHT;
				}
				else if(acc_x > 0){
					return LEFT;
				}

}
void MovePlayer(){

    MOVE=GetPlayerMove();

    	if(MOVE==DOWN){
    		if(headx<X-1 && board[headx+1][heady]!=2)
    			headx++;
    	}
    	else if(MOVE==LEFT){
    		if(heady>0 && board[headx][heady-1]!=2)
    			heady--;
    	}
    	else if(MOVE==UP){
    		if(headx>0 && board[headx-1][heady]!=2)
    			headx--;
    	}
    	else if(MOVE==RIGHT){
    		if(heady<Y-1  && board[headx][heady+1]!=2)
    			heady++;
    	}
    	printf("X=%d Y=%d\n",headx,heady);
    	highest_speed = 0;

    	if(board[headx][heady]==1)
    		board[headx][heady]=0;
}
// void MoveCharacter(int *varx, int *vary, enum movement MOVE){


//     if(MOVE==DOWN){
//         if(*varx<X && board[*varx+1][*vary]!=2)
//             *varx++;
//     }
//     else if(MOVE==LEFT){
//         if(*vary>0 && board[*varx][*vary-1]!=2)
//             *vary--;
//     }
//     else if(MOVE==UP){
//         if(*varx>0 && board[*varx-1][*vary]!=2)
//             varx--;
//     }
//     else if(MOVE==RIGHT){
//         if(vary<Y  && board[varx][vary+1]!=2)
//             vary++;
//     }

//     MOVE=NONE;

//     if(board[headx][heady]==1)
//         board[headx][heady]=0;
// }
void Board2WallsInit(){
	int wallcoardx[68] = {1,2,3,4,5,5,5,5,5,4,3,3,3,6,7,8,9,7,7,8,9,10,11,11,11,11,11,1,2,3,4,5,1,2,3,4,5,5,5,5,5,5,4,3,2,1,2,2,3,3,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,10,10};
	int wallcoardy[68] = {1,1,1,1,1,2,3,4,5,5,5,4,3,1,1,1,1,5,4,4,4,4,4,3,2,1,0,7,7,7,7,7,9,9,9,9,9,10,11,12,13,14,14,14,14,14,11,12,12,11,7,8,9,10,11,12,13,14,7,8,9,10,11,12,13,14,10,11};

	for(int i=0; i<68;i++){
		board[wallcoardx[i]][wallcoardy[i]] = 2;
	}
}
void BoardInit1(){
	board[0][0]=2;
}
void BoardInit2(){
	board[0][5]=2;
}
void BoardInit3(){
	board[5][0]=2;
}
void BoardInit4(){
	board[5][5]=2;
}
void BoardWallsInit(){
	int wallcoardx[88] = {4,3,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,3,4,7,8,9,10,11,11,11,11,11,11,11,11,11,11,11,11,11,11,10,9,8,7,2,2,2,2,2,3,3,4,4,4,4,7,7,7,7,8,8,9,9,9,9,9,7,7,7,7,8,8,9,9,9,9,9,2,2,2,2,2,3,3,4,4,4,4};
	int wallcoardy[88] = {0,0,0,0,0,1,2,3,4,5,6,9,10,11,12,13,14,15,15,15,15,15,15,15,15,15,15,14,13,12,11,10,9,6,5,4,3,2,1,0,0,0,0,0,2,3,4,5,6,5,6,5,6,2,3,2,3,5,6,5,6,5,6,4,3,2,9,10,12,13,9,10,9,10,11,12,13,9,10,11,12,13,9,10,9,10,12,13};

	for(int i=0; i<88;i++){
		board[wallcoardx[i]][wallcoardy[i]] = 2;
	}

}



//uzupelnianie mapy "monetkami" tam gdzie nie ma murkow oraz glowy ghracza
void BoardCoinInit(){
    for(int i=0; i<X; i++){
        for(int j=0; j<Y;j++){
            if(board[i][j]==0)
                board[i][j]=1;
        }
    }
}
void BoardDraw(){
	//BSP_LCD_Clear(0x0000);
    for(int i=0; i<X; i++){
        for(int j=0; j<Y;j++){
            if(i==headx && j==heady)
            	BSP_LCD_DrawBitmap_RGB565(headx*20, heady*20, (uint8_t *)PacManicon, szerokosc, wysokosc);
            else if(i==ghost1x && j==ghost1y)
            	BSP_LCD_DrawBitmap_RGB565(ghost1x*20, ghost1y*20, (uint8_t *)duszek1, szerokosc, wysokosc);
            else if(i==ghost2x && j==ghost2y)
            	BSP_LCD_DrawBitmap_RGB565(ghost2x*20, ghost2y*20, (uint8_t *)duszek2, szerokosc, wysokosc);
            else if(i==ghost3x && j==ghost3y)
            	BSP_LCD_DrawBitmap_RGB565(ghost3x*20, ghost3y*20, (uint8_t *)duszek3, szerokosc, wysokosc);
            else if(i==ghost4x && j==ghost4y)
            	BSP_LCD_DrawBitmap_RGB565(ghost4x*20, ghost4y*20, (uint8_t *)duszek4, szerokosc, wysokosc);
            else if(board[i][j]==0)
            	BSP_LCD_DrawBitmap_RGB565(i*20, j*20, (uint8_t *)puste_pole, szerokosc, wysokosc);
            else if(board[i][j]==1)
            	BSP_LCD_DrawBitmap_RGB565(i*20, j*20, (uint8_t *)moneta, szerokosc, wysokosc);
            else if(board[i][j]==2)
            	BSP_LCD_DrawBitmap_RGB565(i*20, j*20, (uint8_t *)sciana, szerokosc, wysokosc);
        }
        printf("\n");
    }
}
void play(){
    //BoardWallsInit();
    BoardCoinInit();

    while(GetGameState()==RUN){
    	GetPlayerMove();
        BoardDraw();
        MovePlayer();
        MoveGhosts();
        MX_USB_HOST_Process();
        HAL_Delay(600);
    }
    BoardDraw();
    if(GetGameState()==WIN){
        printf("Brawo wygrales\n");
        printf("Wpisz cokolwiek, aby zagrac ponownie\n");
        printf("Wpisz x, zeby zakonczyc\n");
        char c=' ';
        scanf("%c", &c);
        if(c=='x')
            play();
    }
    if(GetGameState()==LOSE){
        printf("Niestety przegrales\n");
        printf("Wpisz cokolwiek, aby zagrac ponownie\n");
        printf("Wpisz x, zeby zakonczyc\n");
        char c=' ';
        scanf("%c", &c);
        if(c=='x')
            play();
    }
}
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	srand(time(0));
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  BSP_LED_Init(LED3);
  L3GD20_Init(InitZyro);

//  BSP_LCD_Init();
//  BSP_LCD_LayerDefaultInit(0, LCD_FRAME_BUFFER);
//  BSP_LCD_SelectLayer(0);
//  BSP_LCD_DisplayOn();
//  BSP_LCD_Clear(LCD_COLOR_WHITE);
//  BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
//  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
//  TS_StateTypeDef TS_State;
//  BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CRC_Init();
  MX_DMA2D_Init();
  MX_FMC_Init();
  MX_I2C3_Init();
  MX_LTDC_Init();
  MX_SPI5_Init();
  MX_TIM1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  	BSP_LCD_Init();
  	adxl_init();
  	// ili9341_Init();

    BSP_LCD_LayerDefaultInit(LCD_BACKGROUND_LAYER, LCD_FRAME_BUFFER);
    BSP_LCD_LayerDefaultInit(LCD_FOREGROUND_LAYER, LCD_FRAME_BUFFER);
    BSP_LCD_SelectLayer(1);
    BSP_LCD_DisplayOn();
    TS_StateTypeDef TS_State;
    BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
 //   BSP_LCD_SetFont(&LCD_DEFAULT_FONT); // Ustawienie domyślnej czcionki
 //   BSP_LCD_SetTextColor(LCD_COLOR_YELLOW); // Ustawienie koloru tekstu
 //   BSP_LCD_DisplayStringAt(0, 100, (uint8_t *)"Pac Man!", CENTER_MODE);
 //   BSP_LCD_SelectLayer(0);
 //   BSP_LCD_Clear(LCD_COLOR_BLUE);
 //     BSP_LCD_SetFont(&LCD_DEFAULT_FONT); // Ustawienie domyślnej czcionki
 //     BSP_LCD_SetTextColor(LCD_COLOR_BLUE); // Ustawienie koloru tekstu
 //     BSP_LCD_DisplayStringAt(0, 200, (uint8_t *)"Pac Man!", CENTER_MODE);
 //   BSP_LCD_DrawBitmap_RGB565(ruchx, ruchy, (uint8_t *)PacManicon, szerokosc, wysokosc);

    uint16_t screenWidth = BSP_LCD_GetXSize();
    uint16_t screenHeight = BSP_LCD_GetYSize();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    adxl_read(0x32, 6);
    first_acc_x = (data_rec[1]<<8 | data_rec[0]);
    first_acc_y = (data_rec[3]<<8 | data_rec[2]);
    //play();
    BSP_LCD_Clear(LCD_COLOR_BLUE);
  while (1)
  {
      // Odczytanie stanu dotyku
      BSP_TS_GetState(&TS_State);
      if (TS_State.TouchDetected) {
          // Współrzędne dotyku
          uint16_t x = TS_State.touchX;
          uint16_t y = TS_State.touchY;

          // Sprawdzenie, która część ekranu została dotknięta i wywołanie odpowiedniej funkcji
          if (y < screenHeight / 4) {
              // Górna część
        	  BoardInit1();
              play();
          } else if (y >= screenHeight / 4 && y < screenHeight / 2) {
              // Druga część
        	  BoardInit2();
              play();
          } else if (y >= screenHeight / 2 && y < 3 * screenHeight / 4) {
              // Trzecia część
        	  BoardInit3();
              play();
          } else if (y >= 3 * screenHeight / 4) {
              // Dolna część
        	  BoardInit4();
              play();
          }
      }
      HAL_Delay(100);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */





/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
