#include "c4mlib.h"
#include <string.h>

#define UARTBAUD 38400
#define maxTransmitBytes 10

typedef struct Wheel
{
    int left_front;  //左前輪
    int right_front; //右前輪
    int left_rear;   //左後輪
    int right_rear;  //右後輪
} Wheel;

enum
{
    forward,      //前進
    left,         //向左
    backward,     //後退
    right,        //向右
    left_front,   //左前
    right_front,  //右前
    right_behind, //左後
    left_behind,  //右後
    rotate_cw,    //順時鐘轉
    rotate_ccw,   //逆時鐘轉
};

Wheel wheelVal = {-1, 1, -1, 1};
PWMStr_t *global_PWM_str;

int RPM = 10;
const char init_str[] = "Success";
const char key_value[] = "WASDQEZCR";

void UART_init();
// void Timer_init();

void Movement_condition(int dir);
void Movement_update();

ISR(USART1_RX_vect)
{
    printf("in ISR\n");

    char receiveData[10];
    UARTM_rec(3, 9, 0, maxTransmitBytes, (void *)&receiveData, 5);

    char *receiveData_split;
    receiveData_split = strtok(receiveData, "#");

    if (!strcmp(receiveData_split, "Connect"))
        UARTM_trm(3, 9, 0, sizeof(init_str), (void *)&init_str, 5);
    else
    {
        //判斷WASDQEZCR方向並執行
        char *findDir = strstr(key_value, receiveData_split);

        if (findDir != NULL)
            Movement_condition(findDir - key_value);
    }

    sei();
}

int main()
{
    C4M_DEVICE_set();
    UART_init();

    ASASERVO_PWMPREPRO_LAY();
    global_PWM_str = &PWM_str;
    sei();

    printf("Start\n");
    AsaServo_PwmPrePro_cmd(&PWM_str, 1, RPM);
    AsaServo_PwmPrePro_cmd(&PWM_str, 2, RPM);
    AsaServo_PwmPrePro_cmd(&PWM_str, 3, -RPM);
    AsaServo_PwmPrePro_cmd(&PWM_str, 4, -RPM);

    while (1)
    {
        _delay_ms(1);
    }

    return 0;
}

void UART_init()
{
    //設定UART通訊包率
    uint16_t baud = F_CPU / 16 / UARTBAUD - 1;
    UBRR1H = (uint8_t)(baud >> 8);
    UBRR1L = (uint8_t)baud;

    //設定RX/TX interrupt - RX ENABLE / TX DISABLE
    UCSR1B |= (1 << RXCIE1) | (0 << TXCIE1);

    //設定RX/TX 致能
    UCSR1B |= (1 << RXEN1) | (1 << TXEN1);

    //設定傳輸資料 8 bits
    UCSR1C |= (0x011 << UCSZ10);
}

// void Timer_init()
// {
//     //TCCR3A normal port operation
//     REGFPT(&TCCR3A, 0x03, 0, 0);

//     //TCCR3B 4:3 CTC mode
//     REGFPT(&TCCR3B, 0x18, 3, 1);

//     //TCCR3B 2:0 N = 1024
//     REGFPT(&TCCR3B, 0x07, 0, 5);

//     OCR3A = 539;
//     //f_OCR = f_clk_IO / (2 * N * (1 + OCR3))
//     //      = 11059200 / (2 * 1024 * (1 + 539)) = 10
//     //Enable Timer3 Interrupt
//     REGFPT(&ETIMSK, 0x10, 4, 1);
// }

void Movement_condition(int dir)
{
    printf("dir=%d\n", dir);
    switch (dir)
    {
    case forward:
        wheelVal.left_front = -1;
        wheelVal.right_front = 1;
        wheelVal.left_rear = -1;
        wheelVal.right_rear = 1;
        break;

    case backward:
        wheelVal.left_front = 1;
        wheelVal.right_front = -1;
        wheelVal.left_rear = 1;
        wheelVal.right_rear = -1;
        break;

    case left:
        wheelVal.left_front = 1;
        wheelVal.right_front = 1;
        wheelVal.left_rear = -1;
        wheelVal.right_rear = -1;
        break;

    case right:
        wheelVal.left_front = -1;
        wheelVal.right_front = -1;
        wheelVal.left_rear = 1;
        wheelVal.right_rear = 1;
        break;

    case right_front:
        wheelVal.left_front = -1;
        wheelVal.right_front = 0;
        wheelVal.left_rear = 0;
        wheelVal.right_rear = 1;
        break;

    case left_front:
        wheelVal.left_front = 0;
        wheelVal.right_front = 1;
        wheelVal.left_rear = -1;
        wheelVal.right_rear = 0;
        break;

    case right_behind:
        wheelVal.left_front = 1;
        wheelVal.right_front = 0;
        wheelVal.left_rear = 0;
        wheelVal.right_rear = -1;
        break;

    case left_behind:
        wheelVal.left_front = 0;
        wheelVal.right_front = -1;
        wheelVal.left_rear = 1;
        wheelVal.right_rear = 0;
        break;

    case rotate_cw:
        wheelVal.left_front = -1;
        wheelVal.right_front = -1;
        wheelVal.left_rear = -1;
        wheelVal.right_rear = -1;
        break;

    case rotate_ccw:
        wheelVal.left_front = 1;
        wheelVal.right_front = 1;
        wheelVal.left_rear = 1;
        wheelVal.right_rear = 1;
        break;
    }

    Movement_update();
}

void Movement_update()
{
    /*
            ---front---
            ___________
    2 -     |         |   - 0
            |         |
            |         |
            |         |
            |         |   - 1
    3 -     |_________|
            ---back---
    */
    AsaServo_PwmPrePro_SpeedAdj(global_PWM_str, 1, RPM * wheelVal.left_front);
    AsaServo_PwmPrePro_SpeedAdj(global_PWM_str, 2, RPM * wheelVal.right_front);
    AsaServo_PwmPrePro_SpeedAdj(global_PWM_str, 3, RPM * wheelVal.left_rear);
    AsaServo_PwmPrePro_SpeedAdj(global_PWM_str, 4, RPM * wheelVal.right_rear);
}