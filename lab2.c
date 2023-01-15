#include "msp430fg4618.h"
#include "stdbool.h"

volatile int outbyte;
volatile int buf;

volatile bool start;
volatile bool update1;
volatile bool update2;

volatile int pointer;

int main(void) {
    WDTCTL = WDTPW | WDTHOLD; // остановка сторожевого таймера
    SCFQCTL |= SCFQ_4M; //установка частоты 128*32768

    outbyte = 243; //передаваемые данные
    start = false;
    update1 = false;
    update2 = false;

    //настройка портов
    P2DIR |= BIT3;
    P2SEL |= BIT3;

    //Настройка таймера модуляции
    TBCCR0 = 110;
    TBCCR2 = 55;
    TBCTL |= TBSSEL_2; //тактирование от SMCLK
    TBCCTL2 &= ~OUT; //режим ШИМ (set-reset)
    TBCTL |= TBCLR | MC_1; // обнуление счетчика и запуск таймера (16 bit)

    TACCR1 = 0; //17 при передаче
    TACCR0 = 34; //68 при лог 1
    TACTL |= TASSEL_1 | MC_1;

    while(1) {
        if(TAR > TACCR1) {
            TBCCTL2 &= ~OUTMOD_3;
            TBCCTL2 &= ~OUT;
        }
        else {
            TBCCTL2 = OUTMOD_3; //режим установка/сброс
        }

        if(start) {
            if ((TAR == TACCR1) & (!update1)) { //отсчет пауз для передачи 1 или 0
                update1 = true;
                update2 = false;
                bool bit = (buf >> pointer) & 0x01u;
                if (bit) {
                    TACCR0 = 68;
                }
                else {
                    TACCR0 = 34;
                }
            }
            if ((TAR == TACCR0) & (!update2)) { //отсчет длительности импульса и переход к след. биту данныых
                update1 = false;
                update2 = true;
                TACCR1 = 17;
                pointer++;
                if (pointer == 8) { //если передача закончилась, то переключаем start
                    TACCR1 = 0;
                    start = false;
                }
            }
        }

        //запускаем передачу переключением перем-й start, pointer нужен для побитовой передачи
        if(!(P1IN & BIT0) & (!start)) {
            start = true;
            pointer = -1;
            update1 = true;
            update2 = false;
            buf = outbyte;
        }
    }

    return 0;
}
