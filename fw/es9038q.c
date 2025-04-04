/*
    STC8G1K08 MCU firmware for ES9038Q2M DAC
    faust93 at <monumentum@gmail.com>
*/
#include "fw_hal.h"

//#define DEBUG

#define ES9038_ADDR  0x90

#define I2C_SDA P32
#define I2C_SCL P33

/* ES9038Q2M register space */
#define ES9038Q2M_SYSTEM_SETTING                        0x00
#define ES9038Q2M_INPUT_CONFIG                          0x01
#define ES9038Q2M_MIXING                                0x02
#define ES9038Q2M_AUTOMUTE_TIME                         0x04
#define ES9038Q2M_AUTOMUTE_LEVEL                        0x05
#define ES9038Q2M_DEEMP_DOP                             0x06
#define ES9038Q2M_FILTER                                0x07
#define ES9038Q2M_GPIO_CONFIG                           0x08
#define ES9038Q2M_MASTER_MODE                           0x0A
#define ES9038Q2M_SPDIF_INPUT                           0x0B
#define ES9038Q2M_DPLL                                  0x0C
#define ES9038Q2M_THD_COMPENSATION                      0x0D
#define ES9038Q2M_SOFT_START                            0x0E
#define ES9038Q2M_VOLUME1                               0x0F
#define ES9038Q2M_VOLUME2                               0x10
#define ES9038Q2M_INPUT_SEL                             0x15
#define ES9038Q2M_2_HARMONIC_COMPENSATION_0             0x16
#define ES9038Q2M_2_HARMONIC_COMPENSATION_1             0x17
#define ES9038Q2M_3_HARMONIC_COMPENSATION_0             0x18
#define ES9038Q2M_3_HARMONIC_COMPENSATION_1             0x19
#define ES9038Q2M_GENERAL_CONFIG_0                      0x1B
#define ES9038Q2M_GENERAL_CONFIG_1                      0x27

#define ES9038Q2M_CHIP_ID                               0x40

#define ES9038Q2M_DPLL_N1                               0x42
#define ES9038Q2M_DPLL_N2                               0x43
#define ES9038Q2M_DPLL_N3                               0x44
#define ES9038Q2M_DPLL_N4                               0x45

#define ES9038Q2M_INPUT                                 0x60

#define RegNum 12
__CODE uint8_t es9038reg[RegNum][2]={
    {ES9038Q2M_SYSTEM_SETTING, 0x01},
    {ES9038Q2M_INPUT_CONFIG, 0xCC}, //0xCC 32-bit, I2S, DSD, SPD, I2S auto-select
    {ES9038Q2M_DEEMP_DOP, 0x42},
    {ES9038Q2M_GPIO_CONFIG, 0xFF},
    {ES9038Q2M_MASTER_MODE, 0x02}, // async DPLL mode default. set to 0x12 128fs for sync mode
    {ES9038Q2M_SPDIF_INPUT, 0x00}, // 0x20 DATA2 pin for SPDIF input (0x00 = DATA_CLK, default)
    {ES9038Q2M_DPLL, 0x5a}, // 0x5a DPLL config. default
    {ES9038Q2M_SOFT_START, 0x8A},
    {ES9038Q2M_VOLUME1, 0x00},
    {ES9038Q2M_VOLUME2, 0x00},
    {ES9038Q2M_FILTER, 0x80},
    {ES9038Q2M_GENERAL_CONFIG_0, 0x8C},
/* filter values
    0xE0 - brick wall
    0xC0 - corrected minimum phase fast roll-off
    0x80 - apodizing fast roll-off (default)
    0x60 - minimum phase slow roll-off
    0x40 - minimum phase fast roll-off
    0x20 - linear phase slow roll-off
    0x00 - linear phase fast roll-off
*/
};

/* I2C */
void i2c_init(void)
{
    I2C_SCL = SET;
    SYS_DelayUs(2);
    I2C_SDA = SET;
    SYS_DelayUs(2);
}

void i2c_start(void)
{
    I2C_SDA = SET;
    I2C_SCL = SET;
    SYS_DelayUs(5);
    I2C_SDA = RESET;
    SYS_DelayUs(5);
    I2C_SCL = RESET;
}

void i2c_stop(void)
{
     I2C_SCL = RESET;
     I2C_SDA = RESET;
     SYS_DelayUs(5);
     I2C_SCL = SET;
     SYS_DelayUs(5);
     I2C_SDA = SET;
     SYS_DelayUs(5);
}

void i2c_ack(void)
{
    I2C_SCL = RESET;
    I2C_SDA = RESET;
    SYS_DelayUs(5);
    I2C_SCL = SET;
    SYS_DelayUs(5);
    I2C_SCL = RESET;
}

void i2c_nack(void)
{
    I2C_SCL = RESET;
    I2C_SDA = SET;
    SYS_DelayUs(2);
    I2C_SCL = SET;
    SYS_DelayUs(4);
    I2C_SCL = RESET;
}

uint8_t i2c_wait_ack(void)
{
    uint8_t ucErrTime=0;

    I2C_SDA = SET;
    SYS_DelayUs(1);
    I2C_SCL = SET;
    SYS_DelayUs(1);

    while((I2C_SDA & 0x01))
    {
        ucErrTime++;
        if(ucErrTime > 250)
        {
            i2c_stop();
            return 1;
        }
    }
    I2C_SCL = RESET;
    return 0;
}

void i2c_write_byte(uint8_t byte)
{
    uint8_t i=0;

    for(i=0;i<8;i++)
    {
        I2C_SDA = byte & (0x80>>i);
        I2C_SCL = SET;
        SYS_DelayUs(1);
        I2C_SCL = RESET;
        SYS_DelayUs(1);
    }
}

uint8_t i2c_read_byte(void)
{
    uint8_t i,byte=0;

    I2C_SDA = SET;
    for(i=0;i<8;i++)
    {
        I2C_SCL = SET;
        if((I2C_SDA & 0x01)) {
            byte|=(0x80>>i);
        }
        SYS_DelayUs(1);
        I2C_SCL = RESET;
        SYS_DelayUs(1);
    }
    return byte;
}


void es_write_byte(uint8_t reg_address, uint8_t byte)
{
    uint8_t ack = 0;

    i2c_start();
    i2c_write_byte(ES9038_ADDR);
    ack = i2c_wait_ack();
    i2c_write_byte(reg_address);
    ack = i2c_wait_ack();
    i2c_write_byte(byte);
    ack = i2c_wait_ack();
    i2c_stop();
}


uint8_t es_read_byte(uint8_t reg_address)
{
    uint8_t byte = 0;
    uint8_t ack = 0;

    i2c_start();
    i2c_write_byte(ES9038_ADDR);
    ack = i2c_wait_ack();
    i2c_write_byte(reg_address);
    ack = i2c_wait_ack();

    i2c_start();
    i2c_write_byte(ES9038_ADDR|0x01);
    ack = i2c_wait_ack();
    byte = i2c_read_byte();
    i2c_nack();
    i2c_stop();
    return byte;
}

void es_init(void){
    // ES9038 RESETB PIN, power ON
    P55 = SET;
    SYS_Delay(50);
    // init es9038 registers
    for(uint8_t i=0; i<RegNum; i++) {
        es_write_byte(es9038reg[i][0], es9038reg[i][1]);
    }
}

void thd_сompensation(int16_t c2, int16_t c3, int8_t onoff) {
    int8_t c2lower = c2;
    int8_t c2upper = c2 >> 8;
    int8_t c3lower = c3;
    int8_t c3upper = c3 >> 8;
    es_write_byte(ES9038Q2M_THD_COMPENSATION, onoff);
    es_write_byte(ES9038Q2M_2_HARMONIC_COMPENSATION_0, c2lower);
    es_write_byte(ES9038Q2M_2_HARMONIC_COMPENSATION_1, c2upper);
    es_write_byte(ES9038Q2M_3_HARMONIC_COMPENSATION_0, c3lower);
    es_write_byte(ES9038Q2M_3_HARMONIC_COMPENSATION_1, c3upper);
}

void main(void)
{
    SYS_SetClock();
#ifdef DEBUG
    UART1_Config8bitUart(UART1_BaudSource_Timer1, HAL_State_ON, 115200);
#endif
    // Configure I2C pins
    GPIO_P3_SetMode(GPIO_Pin_2, GPIO_Mode_InOut_QBD); // sda
    GPIO_P3_SetMode(GPIO_Pin_3, GPIO_Mode_Output_PP); // scl
    i2c_init();
    SYS_Delay(50);
    es_init();
    //thd_сompensation(0x0000, 0x0000, 0x00);

#ifdef DEBUG
    uint8_t rv = 0;

    UART1_TxString("CHIP ID: ");
    rv = es_read_byte(ES9038Q2M_CHIP_ID);
    UART1_TxHex(rv);
    UART1_TxString("\r\n");

    UART1_TxString("REG DUMP:\r\n");
    for (uint8_t i=0; i<RegNum; i++)
    {
        rv = es_read_byte(es9038reg[i][0]);
        UART1_TxHex(es9038reg[i][0]);
        UART1_TxString(":");
        UART1_TxHex(rv);
        UART1_TxChar(',');
    }

    uint32_t dpllNumber;
    uint16_t r0,r1,r2,r3;
#endif

    while(1)
    {
#ifdef DEBUG
        r0 = es_read_byte(ES9038Q2M_DPLL_N1);
        r1 = es_read_byte(ES9038Q2M_DPLL_N2);
        r2 = es_read_byte(ES9038Q2M_DPLL_N3);
        r3 = es_read_byte(ES9038Q2M_DPLL_N4);
        r3 <<= 8;
        dpllNumber = r3 | r2;
/* DPLL value to SRATE conversion
  20 = 32Hz
  28 = 44Hz
  31 = 48Hz
  57 = 88Hz
  62 = 96Hz
 115 = 176Hz
 125 = 192Hz
 230 = 352Hz
 231 = 352Hz
 251 = 384Hz
*/
        UART1_TxString("DPLL:");
        UART1_TxHex(dpllNumber);
        UART1_TxString("\r\n");
/* lock state
 0 = not locked
 1 = locked
*/
        UART1_TxString("LOCK & INPUT: ");
        rv = es_read_byte(ES9038Q2M_CHIP_ID);
        rv &= 0x01;
        UART1_TxHex(rv);
        UART1_TxString(",");
        rv = es_read_byte(ES9038Q2M_INPUT);
/* input selection values mapping
 1 = DSD
 2 = I2S
 4 = SPDIF
 8 = DoP
*/
        UART1_TxHex(rv);
        UART1_TxString("\r\n");
#endif
        SYS_Delay(1000);
    }
}
