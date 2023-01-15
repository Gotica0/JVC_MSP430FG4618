#include "msp430fg4618.h"
#include "stdbool.h"

//Данные для дисплея
#define d 0x08
#define c 0x04
#define b 0x02
#define a 0x01
#define g 0x20
#define h 0x80
#define e 0x40
#define f 0x10

volatile unsigned char LCDs[16] = { a + b + c + d + e + f, // 0
        b + c, // 1
        a + b + d + e + g, // 2
        a + b + c + d + g, // 3
        b + c + f + g, // 4
        a + c + d + f + g, // 5
        a + c + d + e + f + g, // 6
        a + b + c, // 7
        a + b + c + d + e + f + g, // 8
        a + b + c + d + f + g, // 9
                                   a + b + c + e + f + g,     //A
                                   c + d + e + f + g,       //B
                                   a + d + e + f,         //C
                                   b + c + d + e + g,       //D
                                   a + d + e + f + g,       //E
                                   a + e + f + g          //F
};


volatile int outbyte;
volatile int inbyte;


volatile int cnt;
volatile int length;

volatile bool start;


volatile int CCR;
volatile int time;

volatile int pointer;
volatile int inputPointer;

int main(void) {
    WDTCTL = WDTPW | WDTHOLD; // остановка сторожевого таймера
    SCFQCTL |= SCFQ_4M; //установка частоты 128*32768

    outbyte = 0x35;
    start = false;

    cnt = 0;
    length = 40;
    inputPointer = 0;

    //настройка портов
    P2DIR |= BIT3;
    P2SEL |= BIT0 | BIT3;
    P5SEL |= BIT2 | BIT3 | BIT4;

    P1IES &= 0;  // прерывание по спадающему фронту
    P1IFG &= ~BIT0; //обнуление флагов прерывания
    P1IE |= BIT0;   // Разрешаем прерывания для P1.0 и P1.1

    //Настройка таймера модуляции
    TBCCR0 = 110;
    TBCCR2 = 55;
    TBCCTL2 = 0; //режим ШИМ (set-reset)

    //TACCTL2 |= CM_3 | SCS | CAP | CCIE;
    TACTL |= TACLR | TASSEL_1 | MC_2;


    LCDACTL |= LCDON | LCD4MUX | LCDFREQ_256; //включение модуля | режим 4-mux | делитель частоты - 512
    LCDAPCTL0 |= LCDS8; //включение выходов S4...S7 (2 младших значений индикаторе)


    __bis_SR_register(LPM0_bits + GIE);
    return 0;
}


#pragma vector = PORT1_VECTOR
__interrupt void P1_ISR(void) {
    P1IFG &= ~BIT0; //обнуление флагов прерывания
    P1IE = 0;
    pointer = 0;
    start = true;
    inputPointer = 0;
    cnt = 0;
    length = 40;
    inbyte = 0;
    TACCTL2 |= CM_3 | SCS | CAP | CCIE;
    TBCCTL2 = OUTMOD_7 | CCIE;
    TBCTL |= TBCLR | TBSSEL_2 | MC_1;
    __bic_SR_register_on_exit(LPM0_bits);
}


#pragma vector = TIMERB1_VECTOR
__interrupt void TB1_ISR(void) {
    TBCCTL2 &= ~CCIFG;
    cnt++;
    if (cnt == 20) {
        TBCCTL2 &= ~OUTMOD_7;
        TBCCTL2 &= ~OUT;
        if (pointer < 8) {
            if ((outbyte >> pointer) & 0x01u) {
                length = 80;
            }
            else {
                length = 40;
            }
        }
        else {
            TBCCTL2 = 0;
            TBCTL = 0;
            P1IFG &= ~BIT0;
            P1IE = BIT0;
        }
    }

    if (cnt == length) {
        cnt = 0;
        pointer++;
        TBCCTL2 |= OUTMOD_7;
    }
    __bic_SR_register_on_exit(LPM0_bits);
}


#pragma vector = TIMERA1_VECTOR
__interrupt void TA1_ISR(void) {
    TACCTL2 &= ~CCIFG;
    if (start) {
        if (!(TACCTL2 & CCI)) {
            CCR = TACCR2;
        }
        if (TACCTL2 & CCI) {
            if (TACCTL2 & COV) {
                TACCTL2 &= ~COV;
                time = TACCR2 + 65536 - CCR;
            }
            else {
                time = TACCR2 - CCR;
            }
            if (time > 22) {
                inbyte |= (0x01u << (pointer - 1));
            }
            inputPointer++;
            if (inputPointer >= 8) {
                start = false;
                TACCTL2 = 0;
                inputPointer = 0;

                LCDMEM[4] = LCDs[inbyte & 0x0F];
                LCDMEM[5] = LCDs[(inbyte >> 4) & 0x0F];
                inbyte = 0;
            }
        }
    }
    else {
        /*
        if(!(TACCTL2 & CCI)) {
            start = true;
            inputPointer = 0;
            inbyte = 0;
            CCR = TACCR2;
        }
        */
    }

    __bic_SR_register_on_exit(LPM0_bits);
}
