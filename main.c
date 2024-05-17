#include <msp430g2553.h>
#include <stdint.h>
#include <stdio.h>
#include <myRc522.h>
static int dataflag =0;
uint8_t i = 0x00;
unsigned char data1[16] = {0xFF,0xAA,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xAA,0xFF};
unsigned char data2[16] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; 
unsigned char DefaultKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 

unsigned char tanimliIDs[10][5]={0};

void main(void)
{
    WDTCTL = WDTPW + WDTHOLD;                 // Watchdog timeri durdur
    
    spiInit();
    rc522Init();
    
    P1OUT &= ~BIT0;       //Kart eslesiyo mu ?                     
    P1DIR |= BIT0;                          
    P1OUT &= ~BIT1;       //Kart Geldi mi ?                     
    P1DIR |= BIT1;
    P2OUT &= ~BIT5;       //Data Bos                    
    P2DIR |= BIT5;                            
    P2OUT &= ~BIT4;       //Data Dolu                      
    P2DIR |= BIT4;
    unsigned char status = 0;
    unsigned char str[6] = {0};
    unsigned char strx[16] = {0};
    unsigned char gelenID[5]={0}; 
    
    P1DIR &= ~BIT3;   // P1.3'i giris yap
    P1REN |= BIT3;   // P1.3'i yukseltici direnc ile etkinle?tir
    P1OUT |= BIT3;   // P1.3'un yuksek seviyeye ?ek
    P1IE |= BIT3;    // P1.3 icin kesmeyi etkinle?tir
    P1IES |= BIT3;   // Yukselen kenara g?re kesmeyi yap?land?r
    P1IFG &= ~BIT3;  // Kesme bayragini temizle
    
    P2DIR &= ~BIT3;   // P1.3'i giris yap
    P2REN |= BIT3;    // P1.3'i yukseltici direnc ile etkinle?tir
    P2OUT |= BIT3;    // P1.3'un yuksek seviyeye ?ek
    P2IE |= BIT3;     // P1.3 icin kesmeyi etkinle?tir
    P2IES |= BIT3;    // Yukselen kenara g?re kesmeyi yap?land?r
    P2IFG &= ~BIT3;   // Kesme bayragini temizle
    
    P1DIR &= ~BIT4; // P1.4'i giris yap (Yeni buton)
    P1REN |= BIT4; // P1.4'i yukseltici direnc ile etkinle?tir
    P1OUT |= BIT4; // P1.4'un yuksek seviyeye ?ek
    
    __bis_SR_register(GIE);
                             
    while(1)
    { 
      if ((P1IN & BIT4) == 0) // Butona basilmis mi kontrol et
      {
        status = Request(PICC_REQIDL, str);
        status = kartIDOku(str);
        if (status == MI_OK)
        {
            status = SelectTag(str);
            status = Auth(PICC_AUTHENT1A, 1, DefaultKey, str);
            if (status == MI_OK)
            {
                status = WriteBlock(1, data1); // data2'yi yaz
            }
        }
      }
      if(dataflag==1)
      {
        for(int i=0; i<5;i++)
        {
          status = Request(PICC_REQIDL, str); //Kart geldi mi ?
          status = kartIDOku(str);
          if(status == MI_OK)
            break;
        }
          status = SelectTag(str);
          status = Auth(PICC_AUTHENT1A,1,DefaultKey,str);
          
          if(status == MI_OK)
          {
               status = WriteBlock(1,data2);              
          }
        dataflag=0;
      }  

        status = Request(PICC_REQIDL, str);
        if (status == MI_OK)
         {
            status = kartIDOku(str); //Geldiyse, ID oku
            P1OUT |=BIT1; // Ledi yak
         }
        else
        {
           P1OUT &= ~BIT1; 
        }
        
        if (status == MI_OK)
        {
           for (int i=0;i<5;i++)
           {
             gelenID[i]=str[i]; //Gelen ID yi buffera at
           }
           for(int j=0;j<10;j++) 
           {
             if( tanimliIDs[j][0]==gelenID[0]&&
                tanimliIDs[j][1]==gelenID[1]&&
                tanimliIDs[j][2]==gelenID[2]&&
                tanimliIDs[j][3]==gelenID[3]&&
                tanimliIDs[j][4]==gelenID[4]  ) 
               {
                 P1OUT |=BIT0; //ID tanimliysa ledi yak.
                 __delay_cycles(10000);
               }
               else
               {  
                 P1OUT &= ~BIT0;//ID tanimli degilse ledi sondur.
               }
           }
        }
        
            
           status = SelectTag(str);
           status = Auth(PICC_AUTHENT1A,1,DefaultKey,str);
             
           status= ReadBlock(1,strx);   
           if(strx[0]==0x00 && strx[1]==0x00 && strx[14]==0x00 && strx[15]==0x00)
           {  
               P2OUT &= ~BIT5;
               P2OUT |=BIT4;//sari led
           }
           if(strx[0]==data1[0] && strx[1]==data1[1] && strx[14]==data1[14] && strx[15]==data1[15])
           {
              P2OUT &= ~BIT4;
              P2OUT |=BIT5;//yesil led
           } 
    }
}
#pragma vector=PORT2_VECTOR
__interrupt void Port_2(void) 
{
    unsigned char str[8]= {0}; 
    unsigned char status = 5;
    static uint8_t i=0; 
    unsigned char gelenID[5]={0};
    status = Request(PICC_REQIDL, str);
    
    if (status == MI_OK)
    {
      status = kartIDOku(str);
      if (status == MI_OK)
      {
        for (int z=0;z<5;z++)
        {
          gelenID[z]=str[z]; //Gelen ID yi buffera at
        }
        for(int j=0;j<5;j++)
        {
          tanimliIDs[i][j]=gelenID[j]; //Tanimli ID bufferina at
        }
        i++;
        if(i>9)
        {
          i=0; //10 u gecerse 0 la
        }
      }  
    }
     P2IFG &= ~BIT3;
}
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void) 
{
    dataflag=1;
    P1IFG &= ~BIT3;
}
