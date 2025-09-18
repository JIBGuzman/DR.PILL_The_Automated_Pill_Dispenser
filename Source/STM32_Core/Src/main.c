/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "string.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* User profile structure */
typedef struct {
    char* name;
    char* id;
    char* type;
} UserProfile;

/* Define user profiles */
const UserProfile users[] = {
    {"Joseph", "026171247", "admin"},
    {"Jonathan", "028482153", "staff"},
    {"Afzal", "028290663", "staff"},
    {"Jehmel", "029118672", "staff"},
    {"Jeremy", "029118685", "staff"},
    {"Lee", "027480815", "patient"}
};

const int NUM_USERS = sizeof(users) / sizeof(users[0]);
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
#if defined ( __ICCARM__ ) /*!< IAR Compiler */
#pragma location=0x2007c000
ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
#pragma location=0x2007c0a0
ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */

#elif defined ( __CC_ARM )  /* MDK ARM Compiler */

__attribute__((at(0x2007c000))) ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
__attribute__((at(0x2007c0a0))) ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */

#elif defined ( __GNUC__ ) /* GNU Compiler */

ETH_DMADescTypeDef DMARxDscrTab[ETH_RX_DESC_CNT] __attribute__((section(".RxDecripSection"))); /* Ethernet Rx DMA Descriptors */
ETH_DMADescTypeDef DMATxDscrTab[ETH_TX_DESC_CNT] __attribute__((section(".TxDecripSection")));   /* Ethernet Tx DMA Descriptors */
#endif

ETH_TxPacketConfig TxConfig;

ETH_HandleTypeDef heth;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void SystemClock_Config(void);
void pill_dispense(void);
void test_motors(void);
void dispense_pill(uint8_t target_position);
void process_uart_command(uint8_t* data);  // Declare process_uart_command
static void MX_GPIO_Init(void);
static void MX_ETH_Init(void);
//static void MX_USART3_UART_Init(void);
//static void MX_USB_OTG_FS_PCD_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
extern uint8_t UserRxBufferFS[];
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
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_ETH_Init();
    //MX_USB_OTG_FS_PCD_Init();
    MX_TIM1_Init();
    MX_TIM2_Init();
    MX_TIM3_Init();
    MX_USB_DEVICE_Init();

    /* Clear all GPIO outputs */
    HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Enable_GPIO_Port, Enable_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DIR_2_GPIO_Port, DIR_2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Enable_2_GPIO_Port, Enable_2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, LD1_Pin | LD3_Pin | LD2_Pin, GPIO_PIN_RESET);

    char user_type_prompt[] = "Please enter your ID #: ";
    char invalid_id_msg[] = "Invalid user profile. Please re-enter your ID.\r\n";
    char welcome_msg[] = "Welcome, ";
    char admin_menu[] = "\r\n=== Admin Menu ===\r\n1. Add User\r\n2. Remove User\r\n3. Dispense\r\n4. Check Storage\r\n5. Test Motors\r\n6. Log out\r\nSelect option: ";
    char staff_menu[] = "\r\n=== Staff Menu ===\r\n1. Pill Refill/Dispense\r\n2. Patient Medication\r\n3. Patient Log\r\n4. Log out\r\nSelect option: ";
    char patient_menu[] = "\r\n=== Patient Menu ===\r\n1. Retrieve Medication\r\n2. Medication Log\r\n3. Log out\r\nSelect option: ";
    char newline[] = "\r\n";

    while (1) {
        char user_id[10] = {0};
        uint8_t id_len = 0;
        int valid_user = 0;
        int user_index = -1;

        /* Send initial ID prompt via USB CDC */
        CDC_Transmit_FS((uint8_t*)user_type_prompt, strlen(user_type_prompt));

        /* Get and validate user ID */
        while (!valid_user) {
            id_len = 0;
            while (id_len < 9) {
                /* USB CDC reception handled in CDC_Receive_FS callback */
                if (UserRxBufferFS[0] != 0) {
                    char rxData = UserRxBufferFS[0];

                    if (rxData == '\r' || rxData == '\n') {
                        break;
                    } else if (rxData == '^') {
                        CDC_Transmit_FS((uint8_t*)newline, 2);
                        CDC_Transmit_FS((uint8_t*)user_type_prompt, strlen(user_type_prompt));
                        goto main_menu;
                    } else if (rxData == 127 || rxData == '\b') {
                        if (id_len > 0) {
                            id_len--;
                            CDC_Transmit_FS((uint8_t*)"\b \b", 3);
                        }
                    } else {
                        CDC_Transmit_FS((uint8_t*)&rxData, 1);
                        user_id[id_len++] = rxData;
                    }
                    UserRxBufferFS[0] = 0; // Clear processed byte
                }
            }

            CDC_Transmit_FS((uint8_t*)newline, 2);
            user_id[id_len] = '\0';

            /* Validate user ID */
            for (int i = 0; i < NUM_USERS; i++) {
                if (strcmp(user_id, users[i].id) == 0) {
                    valid_user = 1;
                    user_index = i;
                    /* Send welcome message */
                    CDC_Transmit_FS((uint8_t*)welcome_msg, strlen(welcome_msg));
                    CDC_Transmit_FS((uint8_t*)users[i].name, strlen(users[i].name));
                    break;
                }
            }

            if (!valid_user) {
                CDC_Transmit_FS((uint8_t*)invalid_id_msg, strlen(invalid_id_msg));
            }
        }

        /* User menu loop */
        while (valid_user) {
            /* Display menu */
            if (strcmp(users[user_index].type, "admin") == 0) {
                CDC_Transmit_FS((uint8_t*)admin_menu, strlen(admin_menu));
            } else if (strcmp(users[user_index].type, "staff") == 0) {
                CDC_Transmit_FS((uint8_t*)staff_menu, strlen(staff_menu));
            } else {
                CDC_Transmit_FS((uint8_t*)patient_menu, strlen(patient_menu));
            }

            /* Get menu selection */
            while (UserRxBufferFS[0] == 0) {} // Wait for input
            char rxData = UserRxBufferFS[0];
            UserRxBufferFS[0] = 0; // Clear buffer

            CDC_Transmit_FS((uint8_t*)&rxData, 1);
            CDC_Transmit_FS((uint8_t*)newline, 2);

            // Process menu selection
            if (strcmp(users[user_index].type, "admin") == 0) {
                switch (rxData) {
                    case '1': // Add User
                        // Add functionality
                        break;
                    case '2': // Remove User
                        // Add functionality
                        break;
                    case '3': // Dispense
                        pill_dispense();
                        break;
                    case '4': // Check Storage
                        // Add functionality
                        break;
                    case '5': // Test Motors
                        test_motors();
                        break;
                    case '6': // Log out
                        valid_user = 0;
                        break;
                }
            }
            else if (strcmp(users[user_index].type, "staff") == 0) {
                switch (rxData) {
                    case '1': // Pill Refill/Dispense
                        pill_dispense();
                        break;
                    case '2': // Patient Medication
                        // Add functionality
                        break;
                    case '3': // Patient Log
                        // Add functionality
                        break;
                    case '4': // Log out
                        valid_user = 0;
                        break;
                }
            }
            else { // Patient
                switch (rxData) {
                    case '1': // Retrieve Medication
                        pill_dispense();
                        break;
                    case '2': // Medication Log
                        // Add functionality
                        break;
                    case '3': // Log out
                        valid_user = 0;
                        break;
                }
            }
        }
main_menu:
        continue;
    }
}



void test_motors(void)
{
    char testing_msg[] = "Testing motors...\r\n";
    CDC_Transmit_FS((uint8_t*)testing_msg, strlen(testing_msg));

    for (uint8_t motor_id = 1; motor_id <= 12; motor_id++) {
        char motor_msg[50];
        snprintf(motor_msg, sizeof(motor_msg), "Testing motor %d\r\n", motor_id);
        CDC_Transmit_FS((uint8_t*)motor_msg, strlen(motor_msg));
        dispense_pill(motor_id);
        HAL_Delay(500);
    }

    char completed_msg[] = "Motor testing completed.\r\n";
    CDC_Transmit_FS((uint8_t*)completed_msg, strlen(completed_msg));
}


//Definition of process_uart_command
void process_uart_command(uint8_t* data) {
    // Example: Echo received data back to USB CDC
    CDC_Transmit_FS(data, strlen((char*)data));
}


// Pill Dispense Function Call
void pill_dispense(void)
{
    char position_prompt[] = "What pill would you like to request? Pill #1-12: ";
    char count_prompt[] = "Enter number of pills (1-9): ";
    char invalid_msg[] = "Invalid input. Please try again.\r\n";
    uint8_t target_position = 0;
    uint8_t pill_count = 0;

    /* Get position */
    CDC_Transmit_FS((uint8_t*)position_prompt, strlen(position_prompt));
    while (UserRxBufferFS[0] == 0) {} // Wait for input
    target_position = atoi((char*)UserRxBufferFS);
    UserRxBufferFS[0] = 0;

    /* Validate position */
    if (target_position < 1 || target_position > 12) {
        CDC_Transmit_FS((uint8_t*)invalid_msg, strlen(invalid_msg));
        return;
    }

    /* Get count */
    CDC_Transmit_FS((uint8_t*)count_prompt, strlen(count_prompt));
    while (UserRxBufferFS[0] == 0) {} // Wait for input
    pill_count = atoi((char*)UserRxBufferFS);
    UserRxBufferFS[0] = 0;

    /* Execute dispensing */
    for (uint8_t i = 0; i < pill_count; i++) {
        dispense_pill(target_position);
        if (i < pill_count - 1) HAL_Delay(500);
    }
}

void dispense_pill(uint8_t target_position) {
  uint8_t direction;

  switch (target_position) {
      case 1:
          direction = 1;
          HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, direction);
          HAL_GPIO_WritePin(Enable_GPIO_Port, Enable_Pin, GPIO_PIN_RESET);
          HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
          HAL_Delay(1000);
          direction = 0;
          HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, direction);
          HAL_Delay(1000);
          HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
          HAL_GPIO_WritePin(Enable_GPIO_Port, Enable_Pin, GPIO_PIN_SET);
          HAL_Delay(300);

          direction = 1;
      	HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, direction);
      	HAL_GPIO_WritePin(Enable_GPIO_Port, Enable_Pin, GPIO_PIN_RESET);
      	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
      	HAL_Delay(100);
      	direction = 0;
      	HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, direction);
      	HAL_Delay(100);
      	HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
      	HAL_GPIO_WritePin(Enable_GPIO_Port, Enable_Pin, GPIO_PIN_SET);
          break;

      case 2:
          direction = 0;
          HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, direction);
          HAL_GPIO_WritePin(Enable_GPIO_Port, Enable_Pin, GPIO_PIN_RESET);
          HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
          HAL_Delay(1000);
          direction = 1;
          HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, direction);
          HAL_Delay(1000);
          HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
          HAL_GPIO_WritePin(Enable_GPIO_Port, Enable_Pin, GPIO_PIN_SET);

          HAL_Delay(300);

          direction = 0;
      	HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, direction);
      	HAL_GPIO_WritePin(Enable_GPIO_Port, Enable_Pin, GPIO_PIN_RESET);
      	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
      	HAL_Delay(100);
      	direction = 1;
      	HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, direction);
      	HAL_Delay(100);
      	HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
      	HAL_GPIO_WritePin(Enable_GPIO_Port, Enable_Pin, GPIO_PIN_SET);
          break;

      case 3:
              direction = 1;
              HAL_GPIO_WritePin(DIR_2_GPIO_Port, DIR_2_Pin, direction);
              HAL_GPIO_WritePin(Enable_2_GPIO_Port, Enable_2_Pin, GPIO_PIN_RESET);
              HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
              HAL_Delay(1000);
              direction = 0;
              HAL_GPIO_WritePin(DIR_2_GPIO_Port, DIR_2_Pin, direction);
              HAL_Delay(1000);
              HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);
              HAL_GPIO_WritePin(Enable_2_GPIO_Port, Enable_2_Pin, GPIO_PIN_SET);

              HAL_Delay(300);

              direction = 1;
              HAL_GPIO_WritePin(DIR_2_GPIO_Port, DIR_2_Pin, direction);
              HAL_GPIO_WritePin(Enable_2_GPIO_Port, Enable_2_Pin, GPIO_PIN_RESET);
              HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
          	HAL_Delay(100);
          	direction = 0;
              HAL_GPIO_WritePin(DIR_2_GPIO_Port, DIR_2_Pin, direction);
              HAL_Delay(100);
              HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);
              HAL_GPIO_WritePin(Enable_2_GPIO_Port, Enable_2_Pin, GPIO_PIN_SET);
              break;
      case 4:
              direction = 0;
              HAL_GPIO_WritePin(DIR_2_GPIO_Port, DIR_2_Pin, direction);
              HAL_GPIO_WritePin(Enable_2_GPIO_Port, Enable_2_Pin, GPIO_PIN_RESET);
              HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
              HAL_Delay(1000);
              direction = 1;
              HAL_GPIO_WritePin(DIR_2_GPIO_Port, DIR_2_Pin, direction);
              HAL_Delay(1000);
              HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);
              HAL_GPIO_WritePin(Enable_2_GPIO_Port, Enable_2_Pin, GPIO_PIN_SET);

              HAL_Delay(300);

              direction = 0;
              HAL_GPIO_WritePin(DIR_2_GPIO_Port, DIR_2_Pin, direction);
              HAL_GPIO_WritePin(Enable_2_GPIO_Port, Enable_2_Pin, GPIO_PIN_RESET);
              HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
          	HAL_Delay(100);
          	direction = 1;
              HAL_GPIO_WritePin(DIR_2_GPIO_Port, DIR_2_Pin, direction);
              HAL_Delay(100);
              HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);
              HAL_GPIO_WritePin(Enable_2_GPIO_Port, Enable_2_Pin, GPIO_PIN_SET);
              break;

      case 5:
              direction = 1;
              HAL_GPIO_WritePin(DIR_3_GPIO_Port, DIR_3_Pin, direction);
              HAL_GPIO_WritePin(Enable_3_GPIO_Port, Enable_3_Pin, GPIO_PIN_RESET);
              HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
              HAL_Delay(1000);
              direction = 0;
              HAL_GPIO_WritePin(DIR_3_GPIO_Port, DIR_3_Pin, direction);
              HAL_Delay(1000);
              HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
              HAL_GPIO_WritePin(Enable_3_GPIO_Port, Enable_3_Pin, GPIO_PIN_SET);

              HAL_Delay(300);

              direction = 1;
              HAL_GPIO_WritePin(DIR_3_GPIO_Port, DIR_3_Pin, direction);
              HAL_GPIO_WritePin(Enable_3_GPIO_Port, Enable_3_Pin, GPIO_PIN_RESET);
              HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
              HAL_Delay(100);
              direction = 0;
              HAL_GPIO_WritePin(DIR_3_GPIO_Port, DIR_3_Pin, direction);
              HAL_Delay(100);
              HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
              HAL_GPIO_WritePin(Enable_3_GPIO_Port, Enable_3_Pin, GPIO_PIN_SET);
              break;
      case 6:
              direction = 0;
              HAL_GPIO_WritePin(DIR_3_GPIO_Port, DIR_3_Pin, direction);
              HAL_GPIO_WritePin(Enable_3_GPIO_Port, Enable_3_Pin, GPIO_PIN_RESET);
              HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
              HAL_Delay(1000);
              direction = 1;
              HAL_GPIO_WritePin(DIR_3_GPIO_Port, DIR_3_Pin, direction);
              HAL_Delay(1000);
              HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
              HAL_GPIO_WritePin(Enable_3_GPIO_Port, Enable_3_Pin, GPIO_PIN_SET);

              HAL_Delay(300);

              direction = 0;
              HAL_GPIO_WritePin(DIR_3_GPIO_Port, DIR_3_Pin, direction);
              HAL_GPIO_WritePin(Enable_3_GPIO_Port, Enable_3_Pin, GPIO_PIN_RESET);
              HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
              HAL_Delay(1000);
              direction = 1;
              HAL_GPIO_WritePin(DIR_3_GPIO_Port, DIR_3_Pin, direction);
              HAL_Delay(1000);
              HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
              HAL_GPIO_WritePin(Enable_3_GPIO_Port, Enable_3_Pin, GPIO_PIN_SET);
              break;
      case 7:
              direction = 1;
              HAL_GPIO_WritePin(DIR_4_GPIO_Port, DIR_4_Pin, direction);
              HAL_GPIO_WritePin(Enable_4_GPIO_Port, Enable_4_Pin, GPIO_PIN_RESET);
              HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
              HAL_Delay(1000);
              direction = 0;
              HAL_GPIO_WritePin(DIR_4_GPIO_Port, DIR_4_Pin, direction);
              HAL_Delay(1000);
              HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2);
              HAL_GPIO_WritePin(Enable_4_GPIO_Port, Enable_4_Pin, GPIO_PIN_SET);

              HAL_Delay(300);

              direction = 1;
              HAL_GPIO_WritePin(DIR_4_GPIO_Port, DIR_4_Pin, direction);
              HAL_GPIO_WritePin(Enable_4_GPIO_Port, Enable_4_Pin, GPIO_PIN_RESET);
              HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
              HAL_Delay(100);
              direction = 0;
              HAL_GPIO_WritePin(DIR_3_GPIO_Port, DIR_3_Pin, direction);
              HAL_Delay(100);
              HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2);
              HAL_GPIO_WritePin(Enable_3_GPIO_Port, Enable_3_Pin, GPIO_PIN_SET);
              break;
      case 8:
              direction = 0;
              HAL_GPIO_WritePin(DIR_4_GPIO_Port, DIR_4_Pin, direction);
              HAL_GPIO_WritePin(Enable_4_GPIO_Port, Enable_4_Pin, GPIO_PIN_RESET);
              HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
              HAL_Delay(1000);
              direction = 1;
              HAL_GPIO_WritePin(DIR_4_GPIO_Port, DIR_4_Pin, direction);
              HAL_Delay(1000);
              HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2);
              HAL_GPIO_WritePin(Enable_4_GPIO_Port, Enable_4_Pin, GPIO_PIN_SET);

              HAL_Delay(300);

              direction = 0;
              HAL_GPIO_WritePin(DIR_4_GPIO_Port, DIR_4_Pin, direction);
              HAL_GPIO_WritePin(Enable_4_GPIO_Port, Enable_4_Pin, GPIO_PIN_RESET);
              HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
          	HAL_Delay(100);
          	direction = 1;
              HAL_GPIO_WritePin(DIR_4_GPIO_Port, DIR_4_Pin, direction);
              HAL_Delay(100);
              HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2);
              HAL_GPIO_WritePin(Enable_4_GPIO_Port, Enable_4_Pin, GPIO_PIN_SET);
              break;

      case 9:
              direction = 1;
              HAL_GPIO_WritePin(DIR_5_GPIO_Port, DIR_5_Pin, direction);
              HAL_GPIO_WritePin(Enable_5_GPIO_Port, Enable_5_Pin, GPIO_PIN_RESET);
              HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
              HAL_Delay(1000);
              direction = 0;
              HAL_GPIO_WritePin(DIR_5_GPIO_Port, DIR_5_Pin, direction);
              HAL_Delay(1000);
              HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
              HAL_GPIO_WritePin(Enable_5_GPIO_Port, Enable_5_Pin, GPIO_PIN_SET);

              HAL_Delay(300);

              direction = 1;
              HAL_GPIO_WritePin(DIR_5_GPIO_Port, DIR_5_Pin, direction);
              HAL_GPIO_WritePin(Enable_5_GPIO_Port, Enable_5_Pin, GPIO_PIN_RESET);
              HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
              HAL_Delay(100);
              direction = 0;
              HAL_GPIO_WritePin(DIR_5_GPIO_Port, DIR_5_Pin, direction);
              HAL_Delay(100);
              HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
              HAL_GPIO_WritePin(Enable_5_GPIO_Port, Enable_5_Pin, GPIO_PIN_SET);
              break;
      case 10:
              direction = 0;
              HAL_GPIO_WritePin(DIR_5_GPIO_Port, DIR_5_Pin, direction);
              HAL_GPIO_WritePin(Enable_5_GPIO_Port, Enable_5_Pin, GPIO_PIN_RESET);
              HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
              HAL_Delay(1000);
              direction = 1;
              HAL_GPIO_WritePin(DIR_5_GPIO_Port, DIR_5_Pin, direction);
              HAL_Delay(1000);
              HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
              HAL_GPIO_WritePin(Enable_5_GPIO_Port, Enable_5_Pin, GPIO_PIN_SET);

              HAL_Delay(300);

              direction = 0;
              HAL_GPIO_WritePin(DIR_5_GPIO_Port, DIR_5_Pin, direction);
              HAL_GPIO_WritePin(Enable_5_GPIO_Port, Enable_5_Pin, GPIO_PIN_RESET);
              HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
              HAL_Delay(100);
              direction = 1;
              HAL_GPIO_WritePin(DIR_5_GPIO_Port, DIR_5_Pin, direction);
              HAL_Delay(100);
              HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
              HAL_GPIO_WritePin(Enable_5_GPIO_Port, Enable_5_Pin, GPIO_PIN_SET);
              break;
      case 11:
              direction = 1;
              HAL_GPIO_WritePin(DIR_6_GPIO_Port, DIR_6_Pin, direction);
              HAL_GPIO_WritePin(Enable_6_GPIO_Port, Enable_6_Pin, GPIO_PIN_RESET);
              HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
              HAL_Delay(1000);
              direction = 0;
              HAL_GPIO_WritePin(DIR_6_GPIO_Port, DIR_6_Pin, direction);
              HAL_Delay(1000);
              HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
              HAL_GPIO_WritePin(Enable_6_GPIO_Port, Enable_6_Pin, GPIO_PIN_SET);

              HAL_Delay(300);

              direction = 1;
              HAL_GPIO_WritePin(DIR_6_GPIO_Port, DIR_6_Pin, direction);
              HAL_GPIO_WritePin(Enable_6_GPIO_Port, Enable_6_Pin, GPIO_PIN_RESET);
              HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
              HAL_Delay(1000);
              direction = 0;
              HAL_GPIO_WritePin(DIR_6_GPIO_Port, DIR_6_Pin, direction);
              HAL_Delay(1000);
              HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
              HAL_GPIO_WritePin(Enable_6_GPIO_Port, Enable_6_Pin, GPIO_PIN_SET);
              break;
      case 12:
              direction = 0;
              HAL_GPIO_WritePin(DIR_6_GPIO_Port, DIR_6_Pin, direction);
              HAL_GPIO_WritePin(Enable_6_GPIO_Port, Enable_6_Pin, GPIO_PIN_RESET);
              HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
              HAL_Delay(1000);
              direction = 1;
              HAL_GPIO_WritePin(DIR_6_GPIO_Port, DIR_6_Pin, direction);
              HAL_Delay(1000);
              HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
              HAL_GPIO_WritePin(Enable_6_GPIO_Port, Enable_6_Pin, GPIO_PIN_SET);

              HAL_Delay(300);

              direction = 0;
              HAL_GPIO_WritePin(DIR_6_GPIO_Port, DIR_6_Pin, direction);
              HAL_GPIO_WritePin(Enable_6_GPIO_Port, Enable_6_Pin, GPIO_PIN_RESET);
              HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
              HAL_Delay(1000);
              direction = 1;
              HAL_GPIO_WritePin(DIR_6_GPIO_Port, DIR_6_Pin, direction);
              HAL_Delay(1000);
              HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
              HAL_GPIO_WritePin(Enable_6_GPIO_Port, Enable_6_Pin, GPIO_PIN_SET);
              break;

      default:
      	break;

  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 96;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ETH Initialization Function
  * @param None
  * @retval None
  */
static void MX_ETH_Init(void)
{

  /* USER CODE BEGIN ETH_Init 0 */

  /* USER CODE END ETH_Init 0 */

   static uint8_t MACAddr[6];

  /* USER CODE BEGIN ETH_Init 1 */

  /* USER CODE END ETH_Init 1 */
  heth.Instance = ETH;
  MACAddr[0] = 0x00;
  MACAddr[1] = 0x80;
  MACAddr[2] = 0xE1;
  MACAddr[3] = 0x00;
  MACAddr[4] = 0x00;
  MACAddr[5] = 0x00;
  heth.Init.MACAddr = &MACAddr[0];
  heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
  heth.Init.TxDesc = DMATxDscrTab;
  heth.Init.RxDesc = DMARxDscrTab;
  heth.Init.RxBuffLen = 1524;

  /* USER CODE BEGIN MACADDRESS */

  /* USER CODE END MACADDRESS */

  if (HAL_ETH_Init(&heth) != HAL_OK)
  {
    Error_Handler();
  }

  memset(&TxConfig, 0 , sizeof(ETH_TxPacketConfig));
  TxConfig.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
  TxConfig.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
  TxConfig.CRCPadCtrl = ETH_CRC_PAD_INSERT;
  /* USER CODE BEGIN ETH_Init 2 */

  /* USER CODE END ETH_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 4000;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 500;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 250;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 4000;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 500;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 250;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 4000;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 500;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 250;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
//static void MX_USART3_UART_Init(void)
//{
//
//  /* USER CODE BEGIN USART3_Init 0 */
//
//  /* USER CODE END USART3_Init 0 */
//
//  /* USER CODE BEGIN USART3_Init 1 */
//
//  /* USER CODE END USART3_Init 1 */
//  huart3.Instance = USART3;
//  huart3.Init.BaudRate = 115200;
//  huart3.Init.WordLength = UART_WORDLENGTH_8B;
//  huart3.Init.StopBits = UART_STOPBITS_1;
//  huart3.Init.Parity = UART_PARITY_NONE;
//  huart3.Init.Mode = UART_MODE_TX_RX;
//  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
//  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
//  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
//  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
//  if (HAL_UART_Init(&huart3) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  /* USER CODE BEGIN USART3_Init 2 */
//
//  /* USER CODE END USART3_Init 2 */
//
//}

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, DIR_Pin|Enable_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LD1_Pin|LD3_Pin|LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, DIR_3_Pin|Enable_3_Pin|DIR_4_Pin|Enable_4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, Enable_2_Pin|DIR_2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, DIR_5_Pin|Enable_5_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, DIR_6_Pin|Enable_6_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : USER_Btn_Pin */
  GPIO_InitStruct.Pin = USER_Btn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USER_Btn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : DIR_Pin Enable_Pin */
  GPIO_InitStruct.Pin = DIR_Pin|Enable_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LD1_Pin LD3_Pin LD2_Pin */
  GPIO_InitStruct.Pin = LD1_Pin|LD3_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : DIR_3_Pin Enable_3_Pin DIR_4_Pin Enable_4_Pin */
  GPIO_InitStruct.Pin = DIR_3_Pin|Enable_3_Pin|DIR_4_Pin|Enable_4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : Enable_2_Pin DIR_2_Pin */
  GPIO_InitStruct.Pin = Enable_2_Pin|DIR_2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_OverCurrent_Pin */
  GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : DIR_5_Pin Enable_5_Pin */
  GPIO_InitStruct.Pin = DIR_5_Pin|Enable_5_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : DIR_6_Pin Enable_6_Pin */
  GPIO_InitStruct.Pin = DIR_6_Pin|Enable_6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

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
