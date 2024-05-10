#include <msp430g2553.h>
#include <stdint.h>
#include <stdio.h>
#include <myRc522.h>

#define BIT_CLK  BIT5   // SCLK (P1.5)
#define BIT_SOMI BIT6   // MISO (P1.6)
#define BIT_SIMO BIT7   // MOSI (P1.7)
#define BIT_CS   BIT0   // CS (P2.0)

void spiInit()
{
    P1SEL |= BIT_CLK | BIT_SIMO | BIT_SOMI;
    P1SEL2 |= BIT_CLK | BIT_SIMO | BIT_SOMI;
    
    P2DIR |= BIT_CS;
    P2OUT |= BIT_CS;
    
    UCA0CTL1 |= UCSWRST;
    UCB0CTL0 |= UCCKPH + UCMSB + UCMST + UCSYNC; // 3-pin, 8-bit SPI master
    UCB0CTL1 |= UCSSEL_2; // SMCLK
    UCB0BR0 = 0x02; // /2
    UCB0BR1 = 0;
    UCB0CTL1 &= ~UCSWRST; // A??k ?evir
}

void spiWrite(uint8_t data) 
{
    UCB0TXBUF = data; 
    while (!(IFG2 & UCB0TXIFG)); // Bekleme
}

uint8_t spiRead()
{
    while (!(IFG2 & UCB0RXIFG)); // Bekleme
    return UCB0RXBUF;
}
void rc522WriteRegister(uint8_t reg, uint8_t data)
{
    P2OUT &= ~BIT_CS;  
    spiWrite((reg << 1) & 0x7E); 
    spiWrite(data);
    P2OUT |= BIT_CS;
}

uint8_t rc522ReadRegister(uint8_t reg) 
{
    P2OUT &= ~BIT_CS;
    spiWrite(((reg << 1) & 0x7E) | 0x80);
    spiWrite(0xFF);
    uint8_t data = spiRead();
    P2OUT |= BIT_CS;
    return data;
}
void SetBitMask(unsigned char reg, unsigned char mask)  
{       
    unsigned char tmp;
    tmp = rc522ReadRegister(reg);
    rc522WriteRegister(reg, tmp | mask);  // set bit mask
}

void ClearBitMask(unsigned char reg, unsigned char mask)  
{
    unsigned char tmp;
    tmp = rc522ReadRegister(reg);
    rc522WriteRegister(reg, tmp & (~mask));  // clear bit mask
} 

void rc522AntennaOn() 
{
  unsigned char temp;
      temp = rc522ReadRegister(TxControlReg);
      if (!(temp & 0x03))
      {
       SetBitMask(TxControlReg, 0x03); //Tx1RFEn , Tx2RFEn 1 
      }
}
void rc522AntennaOff() 
{
     ClearBitMask(TxControlReg, 0x03); //Tx1RFEn , Tx2RFEn 0
}

void rc522Reset()
{
    rc522WriteRegister(CommandReg, PCD_RESETPHASE);
}
void rc522Init() 
{
    rc522Reset();                                                                                          
    // internal timer ayarlari 	
    //Timer: TPrescaler*TreloadVal/6.78MHz = 24ms
    rc522WriteRegister(TModeReg, 0x8D);		//Tauto=1; f(Timer) = 6.78MHz/TPreScaler 
    rc522WriteRegister(TPrescalerReg, 0x3E); 	//TModeReg[3..0] + TPrescalerReg
    rc522WriteRegister(TReloadRegL, 30);   
    rc522WriteRegister(TReloadRegH, 0); 
    rc522WriteRegister(TxAutoReg, 0x40);		//100%ASK 
    rc522WriteRegister(ModeReg, 0x3D);	//6363h check sum icin kullanilir
    
    //ClearBitMask(Status2Reg, 0x08);		//MFCrypto1On=0
    //rc522WriteRegister(RxSelReg, 0x86);		//RxWait = RxSelReg[5..0]
    //rc522WriteRegister(RFCfgReg, 0x7F);   		//RxGain = 48dB
    rc522AntennaOn();
}

unsigned char Auth(unsigned char authMode, unsigned char BlockAddr, unsigned char *Sectorkey, unsigned char *serNum)
{
	unsigned char status;
	unsigned int recvBits;
	unsigned char i;
	unsigned char buff[12]; 

	buff[0] = authMode;
	buff[1] = BlockAddr;
	for (i=0; i<6; i++)
	{    
		buff[i+2] = *(Sectorkey+i);   
	}
	for (i=0; i<4; i++)
	{    
		buff[i+8] = *(serNum+i);   
	}
	status = ToCard(PCD_AUTHENT, buff, 12, buff, &recvBits);

	if ((status != MI_OK) || (!(rc522ReadRegister(Status2Reg) & 0x08)))
	{   
		status = MI_ERR;   
	}
    
	return status;
}

void CalulateCRC(unsigned char *pIndata, unsigned char len, unsigned char *pOutData)
{
	unsigned char i, n;

	ClearBitMask(DivIrqReg, 0x04);			//CRCIrq = 0
	SetBitMask(FIFOLevelReg, 0x80);			
	//WriteReg(CommandReg, PCD_IDLE);

	for (i=0; i<len; i++)
	{   
		rc522WriteRegister(FIFODataReg, *(pIndata+i));   
	}
	rc522WriteRegister(CommandReg, PCD_CALCCRC);

	i = 0xFF;
	do 
	{
		n = rc522ReadRegister(DivIrqReg);
		i--;
	}
	while ((i!=0) && !(n&0x04));			//CRCIrq = 1

	pOutData[0] = rc522ReadRegister(CRCResultRegL);
	pOutData[1] = rc522ReadRegister(CRCResultRegM);
}

unsigned SelectTag(unsigned char *serNum)
{
	unsigned char i;
	unsigned char status;
	unsigned char size;
	unsigned int recvBits;
	unsigned char buffer[9]; 

	//ClearBitMask(Status2Reg, 0x08);			//MFCrypto1On=0

	buffer[0] = PICC_SElECTTAG;
	buffer[1] = 0x70;
	for (i=0; i<5; i++)
	{
		buffer[i+2] = *(serNum+i);
	}
	CalulateCRC(buffer, 7, &buffer[7]);		//??
	status = ToCard(PCD_TRANSCEIVE, buffer, 9, buffer, &recvBits);
	
	if ((status == MI_OK) && (recvBits == 0x18))
	{   
		size = buffer[0]; 
	}
	else
	{   
		size = 0;    
	}

	return size;
}

unsigned char ReadBlock(unsigned char blockAddr, unsigned char *recvData)
{
	unsigned char status;
	unsigned int unLen;

	recvData[0] = PICC_READ;
	recvData[1] = blockAddr;
	CalulateCRC(recvData,2, &recvData[2]);
	status = ToCard(PCD_TRANSCEIVE, recvData, 4, recvData, &unLen);

	if ((status != MI_OK) || (unLen != 0x90))
	{
		status = MI_ERR;
	}
	
	return status;
}

unsigned char WriteBlock(unsigned char blockAddr, unsigned char *writeData)
{
	unsigned char status;
	unsigned int recvBits;
	unsigned char i;
	unsigned char buff[18]; 
    
	buff[0] = PICC_WRITE;
	buff[1] = blockAddr;
	CalulateCRC(buff, 2, &buff[2]);
	status = ToCard(PCD_TRANSCEIVE, buff, 4, buff, &recvBits);

	if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A))
	{   
		status = MI_ERR;   
	}

	if (status == MI_OK)
	{
		for (i=0; i<16; i++)
		{    
			buff[i] = *(writeData+i);   
		}
		CalulateCRC(buff, 16, &buff[16]);
		status = ToCard(PCD_TRANSCEIVE, buff, 18, buff, &recvBits);

		if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A))
		{   
			status = MI_ERR;   
		}
	}
	
	return status;
}

unsigned char ToCard(unsigned char command, 
                     unsigned char *sendData,
		     unsigned char sendLen, 
	             unsigned char *backData, 
                     unsigned int *backLen)
{
	unsigned char status = MI_ERR;
	unsigned char irqEn = 0x00;
	unsigned char waitIRq = 0x00;
	unsigned char lastBits;
	unsigned char n;
	unsigned int i;

	switch (command)
	{
		case PCD_AUTHENT:
		{
			irqEn = 0x12;
			waitIRq = 0x10;
			break;
		}
		case PCD_TRANSCEIVE:
		{
			irqEn = 0x77;
			waitIRq = 0x30;
			break;
		}
		default:
			break;
	}
   
	rc522WriteRegister(CommIEnReg, irqEn|0x80);
	ClearBitMask(CommIrqReg, 0x80);
	SetBitMask(FIFOLevelReg, 0x80);
	
	rc522WriteRegister(CommandReg, PCD_IDLE);

	for (i=0; i<sendLen; i++)
	{   
            rc522WriteRegister(FIFODataReg, sendData[i]);    
	}

	rc522WriteRegister(CommandReg, command);
	if (command == PCD_TRANSCEIVE)
	{    
            SetBitMask(BitFramingReg, 0x80);		//StartSend=1,transmission of data starts  
	}   

	i = 2000;
	do 
	{
            n = rc522ReadRegister(CommIrqReg);
            i--;
	}
	while ((i!=0) && !(n&0x01) && !(n&waitIRq));

	ClearBitMask(BitFramingReg, 0x80);			//StartSend=0
	
	if (i != 0)
	{    
		if(!(rc522ReadRegister(ErrorReg) & 0x1B))	//BufferOvfl Collerr CRCErr ProtecolErr
		{
			status = MI_OK;
			if (n & irqEn & 0x01)
			{   
				status = MI_NOTAGERR;			//??   
			}

			if (command == PCD_TRANSCEIVE)
			{
				n = rc522ReadRegister(FIFOLevelReg);
				lastBits = rc522ReadRegister(ControlReg) & 0x07;
				if (lastBits)
				{   
					*backLen = (n-1)*8 + lastBits;   
				}
				else
				{   
					*backLen = n*8;   
				}
				if (n == 0)
				{   
					n = 1;    
				}
				if (n > 16)
				{   
					n = 16;   
				}
				
				for (i=0; i<n; i++)
				{   
					backData[i] = rc522ReadRegister(FIFODataReg);    
				}
			}
		}
		else
		{   
			status = MI_ERR;  
		}
	}
	
//SetBitMask(ControlReg,0x80);           //timer stops
//WriteReg(CommandReg, PCD_IDLE); 

	return status;
}

unsigned char Request(unsigned char reqMode, unsigned char *TagType)
{
	unsigned char status;  
	unsigned int backBits;

	rc522WriteRegister(BitFramingReg, 0x07);		//TxLastBists = BitFramingReg[2..0]	???
	
	TagType[0] = reqMode;
	status = ToCard(PCD_TRANSCEIVE, TagType, 1, TagType, &backBits);

	if ((status != MI_OK) || (backBits != 0x10))
	{    
          status = MI_ERR;
	}
   
	return status;
}

unsigned char kartIDOku(unsigned char *serNum)
{
	unsigned char status;
	unsigned char i;
	unsigned char serNumCheck=0;
	unsigned int unLen;


//ClearBitMask(Status2Reg, 0x08);		//TempSensclear
//ClearBitMask(CollReg,0x80);			//ValuesAfterColl
	rc522WriteRegister(BitFramingReg, 0x00);//TxLastBists = BitFramingReg[2..0]
	serNum[0] = PICC_ANTICOLL;
	serNum[1] = 0x20;
	status = ToCard(PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);

	if (status == MI_OK)
	{
		for (i=0; i<4; i++)
		{   
			serNumCheck ^= serNum[i];
		}
		if (serNumCheck != serNum[i])
		{   
			status = MI_ERR;    
		}
	}

	//SetBitMask(CollReg, 0x80);		//ValuesAfterColl=1

	return status;
}
