#include <io.h>                   
#include <stdio.h>
#include <stdlib.h>
#include <delay.h>
#include <string.h>

//#define F_CPU 				8000000UL
#define F_CPU 					3686400UL

#define HAVE_EEPROM
//#define HAVE_EEPROM_TEST
//#define HAVE_MMA7361      // 가속도 센서
#define HAVE_FXLN8361       // 가속도 센서

#define G_DISABLE             	PORTC.3 = 0
#define G_ENABLE             	PORTC.3 = 1
#define DBG_LED_ON            	PORTC.4 = 0
#define DBG_LED_OFF				PORTC.4 = 1
#define FLAG_LED_ON            	PORTC.5 = 0
#define FLAG_LED_OFF			PORTC.5 = 1

#define PWR_STA_PORT		 	( PIND.2 & 1 )
#define MOVE_RANGE_SET			( PIND.3 & 1 )
#define PROGRAM_SET			    ( PIND.4 & 1 )
#define MOVE_FLAG_CLEAR			( PIND.5 & 1 )
#define RESERVE_IN  			( PIND.6 & 1 )
#define FLAG_CPU_ON            	PORTD.7 = 0
#define FLAG_CPU_OFF			PORTD.7 = 1

#define DEFAULT_ACC     		0				//가속센서 초기값
#define SAMPLE_DELAY			delay_ms(0)		//샘플링 DELAY

#ifdef HAVE_MMA7361
#define IGNORE_DIFF     		10				//가속센서 둔감성
#define MOVE_DET_VAL    		1000			//기본 움직이 감지 범위
#elif defined HAVE_FXLN8361
#define IGNORE_DIFF     		10				//가속센서 둔감성
#define MOVE_DET_VAL    		50   			//기본 움직이 감지 범위
#else
#define IGNORE_DIFF     		10				//가속센서 둔감성
#define MOVE_DET_VAL    		1000			//기본 움직이 감지 범위
#endif

#ifdef HAVE_EEPROM
#define EEPROM_FLAG_ADDR   		0x05			// EEPROM에 저정될 주소.
#endif

#define BAUD 2400 
#define BAUDRATE ((F_CPU)/(BAUD*16UL)-1)		// set baud rate value for UBRR

void port_init()
{
	DDRC=(1<<DDB3)|(1<<DDB4)|(1<<DDB5); 
    DDRD=(1<<DDB7); 
}

//init ADC
void adc_init()
{
	// AREF = Internal 1.1V
	ADMUX = (1<<REFS0) | (1<<REFS1) | (1<<ADLAR);
	// AREF = AVcc
	ADMUX = (1<<REFS0) | (1<<ADLAR);

	// ADC Enable and prescaler of 128
	// 8000000/128 = 62500
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
}

//read adc value
unsigned char adc_read(unsigned char ch)
{
    // select the corresponding channel 0~7
    // ANDing with '7' will always keep the value
    // of 'ch' between 0 and 7
    ch &= 0b00000111;                  // AND operation with 7
    ADMUX = (ADMUX & 0xF8)|ch;         // clears the bottom 3 bits before ORing

    // start single conversion
    // write '1' to ADSC
    ADCSRA |= (1<<ADSC);

    // wait for conversion to complete
    // ADSC becomes '0' again
    // till then, run loop continuously
    while(ADCSRA & (1<<ADSC));

    return (ADCH);
}

//init USART
void serial_init(void)
{
    UBRR0H = (BAUDRATE>>8);                     // shift the register right by 8 bits
    UBRR0L = BAUDRATE;                          // set baud rate
    UCSR0B|= (1<<TXEN0)|(1<<RXEN0);             // enable receiver and transmitter
    UCSR0C|= (1<<UCSZ00)|(1<<UCSZ01);           // 8bit data format    
}


#if 0
// function to receive data
unsigned char uart_recieve (void)
{
    if(UCSR0A & (1<<RXC0))                           // wait while data is being received
        return UDR0;                                    // return 8-bit data
    else
        return 255;
}

// function to send data
void uart_transmit (unsigned char data)
{
    while (!( UCSR0A & (1<<UDRE0)));                // wait while register is free
    UDR = data;                                   // load data in the register
}
#endif

#ifdef HAVE_EEPROM
void EEPROM_write(unsigned int uiAddress, unsigned char ucData)
{
    while(EECR & (1<<EEPE));    /* Wait for completion of previous write */

        EEARH = (uiAddress>>8); /* Set up address and Data Registers */
        EEARL = uiAddress;

        EEDR = ucData;
        EECR |= (1<<EEMPE);     /* Write logical one to EEMPE */
        EECR |= (1<<EEPE);      /* Start eeprom write by setting EEPE */
}

unsigned char EEPROM_read(unsigned int uiAddress)
{
        while(EECR & (1<<EEPE)); /* Wait for completion of previous write */

        EEARH = (uiAddress>>8);  /* Set up address register */
        EEARL = uiAddress;

        EECR |= (1<<EERE);       /* Start eeprom read by writing EERE */

        return EEDR;             /* Return data from Data Register */
}
#endif

void main(void)
{
    unsigned int acc_x,acc_y, acc_z;
    unsigned int pri_acc_x=DEFAULT_ACC,pri_acc_y=DEFAULT_ACC, pri_acc_z=DEFAULT_ACC;
    unsigned int diff_x, diff_y, diff_z, diff_sum=0;
    unsigned int move_sum=0, move_det_val;
    int i, move_flag=0, toggle=0;
    int clock_rate, mean_sq;      
#ifdef HAVE_EEPROM_TEST          
    unsigned char eeprom_val;
#endif    

    serial_init();
    adc_init();
    port_init();
    FLAG_LED_OFF;
    FLAG_CPU_OFF;
    G_ENABLE;

    printf("\n\r\n\r[uCOM Start!]\n\r");

    if(PROGRAM_SET)
    {
        clock_rate=0;
        printf("Debug running, uCOM Programing is possible 0x%x\n\r", clock_rate);
    }
    else
    {
        clock_rate=7;
        printf("Clock Down, Serial maybe not work from now 0x%x\n\r", clock_rate);
        CLKPR = 0x80;    
        CLKPR = clock_rate;
        //PRR = (1<<PRTWI)| (1<<PRTIM2)| (1<<PRTIM0) | (1<<PRTIM1)| (1<<PRSPI) | (1<<PRUSART0);
    }

    //평균 샘플수 2^mean_sq
    mean_sq = 7 - clock_rate;
    
    if(MOVE_RANGE_SET)
        move_det_val = MOVE_DET_VAL * 4;
    else
        move_det_val = MOVE_DET_VAL;

    if(!clock_rate) printf("Move Rage : %d\n\r", move_det_val);

#ifdef HAVE_EEPROM_TEST
    // EEPROM TEST CODE.
    printf("eeprom write 0x13\n\r");
    EEPROM_write(0x03, 0x13);
    eeprom_val = EEPROM_read(0x03);
    printf("eeprom read 0x%x\n\r", eeprom_val);
    eeprom_val = EEPROM_read(0x00);
    printf("eeprom read 0x%x\n\r", eeprom_val);
    eeprom_val = EEPROM_read(0x01);
    printf("eeprom read 0x%x\n\r", eeprom_val);
#endif    

#ifdef HAVE_EEPROM
    // AVR에 이미지를 쓰게 되면 EEPROM의 값은 0xff 가 된다. 
    // 그러므로 장비가 최초로 부팅되게 되면 움직였다고 생각하는 것이 맞다.
    move_flag = EEPROM_read(EEPROM_FLAG_ADDR);
    if (move_flag != 0)
    {
        if(!clock_rate) printf("#### The device is stolen !!!!! #####\n\r\n\r");
        FLAG_LED_ON;
        FLAG_CPU_ON;
        G_DISABLE;
        move_flag = 1;
    }         
#endif

    while(1)
    {                            
        // Port.D.2 를 수동으로 조작하여 상태값을 바꿨다.
        if( PWR_STA_PORT == 0 )
        {
            if( move_flag == 0 )
            {
                acc_x = 0;
                acc_y = 0;
                acc_z = 0;

                for(i=0 ; i<(1<<mean_sq) ; i++)
                {
                    SAMPLE_DELAY;
                    acc_x += (unsigned int)adc_read(0);
                    acc_y += (unsigned int)adc_read(1);
                    acc_z += (unsigned int)adc_read(2);
                }

                acc_x = acc_x >> mean_sq;
                acc_y = acc_y >> mean_sq;
                acc_z = acc_z >> mean_sq;

                diff_x = (pri_acc_x > acc_x ? pri_acc_x - acc_x : acc_x - pri_acc_x);
                diff_y = (pri_acc_y > acc_y ? pri_acc_y - acc_y : acc_y - pri_acc_y);
                diff_z = (pri_acc_z > acc_z ? pri_acc_z - acc_z : acc_z - pri_acc_z);

                if(pri_acc_x==DEFAULT_ACC && pri_acc_y==DEFAULT_ACC && pri_acc_z==DEFAULT_ACC)
                    diff_sum =0;
                else
                    diff_sum = diff_x + diff_y + diff_z;

                pri_acc_x = acc_x;
                pri_acc_y = acc_y;
                pri_acc_z = acc_z;
                    
                if( diff_sum > IGNORE_DIFF )
                    move_sum += diff_sum;

                if(!clock_rate) printf("x:%4d y:%4d z:%4d, diff=%4d, move=%4d(%d)\n\r", acc_x, acc_y, acc_z, diff_sum, move_sum, move_det_val);                    
                    
                if( move_sum > move_det_val )
                {
                    if(!clock_rate) printf("#### The device is stolen !!!!! #####\n\r\n\r");
                    FLAG_LED_ON;
                    FLAG_CPU_ON;
                    G_DISABLE;
#ifdef HAVE_EEPROM                    
                    // EEPROM 에 쓰는 횟수는 최대 100,000번이다.
                    // 그러므로 값이 바뀔때에만 EEPROM을 사용하도록 한다.
                    if (move_flag != 1)     
                    {
                        if(!clock_rate) printf("WRITE EEPROM---------!!\n\r");
                        EEPROM_write(EEPROM_FLAG_ADDR, 0x01);
                        move_flag=1;
                    }
#else                       
                    move_flag=1;
#endif                    
                    move_sum=0;
                }
            }
        }
        else
        {
            if(!clock_rate) printf("The UTM's power is ON\n\r");
            
            if (MOVE_FLAG_CLEAR)
            {
                FLAG_LED_OFF;
                FLAG_CPU_OFF;
                G_ENABLE;
                if(!clock_rate) printf("CLEAR DATA\n\r");
#ifdef HAVE_EEPROM                
                // EEPROM 에 쓰는 횟수는 최대 100,000번이다.
                // 그러므로 값이 바뀔때에만 EEPROM을 사용하도록 한다.                
                if (move_flag != 0)                        
                {
                   
                    if(!clock_rate) printf("CLEAR EEPROM---------!!\n\r");
                    EEPROM_write(EEPROM_FLAG_ADDR, 0x00);
                    move_flag = 0;
                }                 
#else        
                move_sum=0;
#endif                       
                move_flag = 0;
            }
        }

        if(toggle)
            DBG_LED_ON;
        else
            DBG_LED_OFF;
        toggle = !toggle;
    }
}
