#include <msp430g2553.h>
#include <stdint.h>

// SPI Konfig¨¹rasyonu
#define BIT_CLK  BIT5   // SCLK (P1.5)
#define BIT_SOMI BIT6   // MISO (P1.6)
#define BIT_SIMO BIT7   // MOSI (P1.7)
#define BIT_CS   BIT0   // CS (P2.0)

char packet[]={0xF0,0xF0,0xF0,0x40};
int i;

// SPI haberle?mesi i?in i?levler
void spiInit() {
    P1SEL |= BIT_CLK | BIT_SIMO | BIT_SOMI;
    P1SEL2 |= BIT_CLK | BIT_SIMO | BIT_SOMI;
    
    P2DIR |= BIT_CS;
    P2OUT |= BIT_CS;
    
    UCA0CTL1 |= UCSWRST;
    UCB0CTL0 |= UCCKPH + UCMSB + UCMST + UCSYNC + UCCKPL; // 3-pin, 8-bit SPI master
    UCB0CTL1 |= UCSSEL_2; // SMCLK
    UCB0BR0 = 0x01; // /2
    UCB0BR1 = 0;
    UCB0CTL1 &= ~UCSWRST; // A??k ?evir
}

void spiWrite(uint8_t data) {
    UCB0TXBUF = data;
    while (!(IFG2 & UCB0TXIFG)); // Bekleme
}

uint8_t spiRead() {
    while (!(IFG2 & UCB0RXIFG)); // Bekleme
    return UCB0RXBUF;
}

uint8_t Buffer_u8a[128] = {0};
uint16_t BufferSayac_u16 = 0;

void main(void)
{
    WDTCTL = WDTPW + WDTHOLD;                 // Watchdog timeri durdur

    spiInit();
    //P1OUT |= BIT0;                            // LED için P1.0'i high sur
    //P1DIR |= BIT0;                            // LED için P1.0'i high sur
    //
    //UCB0CTL1 |= UCSWRST;                      // **Durum makinesini sifira ayarla**
    //UCB0CTL0 |= UCMST + UCSYNC + UCCKPL + UCMSB; // 3-pin, 8-bit SPI master
    //                                            // Saat polaritesi yüksek, MSB
    //UCB0CTL1 |= UCSSEL_2;                     // SMCLK
    //UCB0BR0 = 0x0A;                           // 10
    //UCB0BR1 = 0;
    //
    //                           // Modülasyon yok
    //UCB0CTL1 &= ~UCSWRST;  // **USCI durum makinesini baslat**

    //P1OUT |= BIT5;
    //P1REN &= ~BIT5;
  
    //P1DIR &= ~BIT3; // P1.3'¨¹ giri? yap
    //P1REN |= BIT3; // P1.3'¨¹ y¨¹kseltici diren? ile etkinle?tir
    //P1OUT |= BIT3; // P1.3'¨¹ y¨¹ksek seviyeye ?ek
    //P1IE |= BIT3; // P1.3 i?in kesmeyi etkinle?tir
    //P1IES |= BIT3; // Y¨¹kselen kenara g?re kesmeyi yap?land?r
    //P1IFG &= ~BIT3; // Kesme bayra??n? temizle
    
    //IE2 |= UCB0TXIE;  //enable A0 TX Irq
    //IFG2 &= ~UCB0TXIFG; //clear flag
 
    //__bis_SR_register(LPM0_bits + GIE);   
                                          // CPU off, enable interrupts
    while(1){
        
        P2OUT &= ~BIT0;
        spiWrite(0xF);
        Buffer_u8a[BufferSayac_u16++] = spiRead();
         P2OUT |= BIT0;
         __delay_cycles(500000);  
         P1OUT &= ~BIT0;
         __delay_cycles(500000);  
         P1OUT |= BIT0;
        if(BufferSayac_u16 > 127)
        {
            BufferSayac_u16 = 0;
        }
    }
}
/*
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void) 
{
  i=0;
  UCB0TXBUF = packet[i];
  P1IFG &= ~ BIT3;  
    
}
#pragma vector= USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void)
{
    i++;
    if(i<sizeof(packet))
    {
      //94 99
      UCB0TXBUF = packet[i];
    }
    else
    {
       IFG2 &= ~UCB0TXIFG; 
    }
    
   
}*/
