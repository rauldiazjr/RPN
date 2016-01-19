#include "lpc214x.h"
#include "lcd.h"
#include "stdlib.h"
#include <stdlib.h>
#include <ctype.h>

/*****************Global Variables***************** */
int   chr_indx = 0; 
char  buf_str[20] ; 
char  char_buf; 

int   val; 
int   stack_indx = 0; 
int   stack[5]; 

unsigned char doneflg = 0; 
unsigned char isrflg = 0; 

/****************** FUNCTION DECLARTIONS ******************/
char* itoa(int i, char b[]);   /*itoa: converts int to ascii*/
int atoi(const char *str);	   /* atoi: convert ascii to integer  */
int isoperator(char oprator);  /* CHECKS FOR VALID OPERATOR CHAR */
int bufcharIsLast(char temp);  /*Checks for valid input from master */
void pop_exec(void) ;	       /*pops 2 off stack and executes incoming operation */ 
void LCD_showstack(void) ;     /* Shows value of stack on LCD */ 

/****************** System Configurations ******************/
void initSPI(void);
void initPLL(void);
void SPI_ISR(void)__irq;
void initVIC(void);

/****************** Begin Main Program ******************/
int main (void) 
{
  /*Set up system */
  initPLL(); 
  initSPI(); 
  initVIC(); 
  init_lcd();
  lcd_clear();

	while (1)  
	{
		if(isrflg) 
		{  
			isrflg = 0;		
			/*===========================================================
				Null was received first, ignore and reset character array
			============================================================*/ 
			if (buf_str[0] == 0 )
				chr_indx = 0; 
			
			/*===========================================================
				Null was received, process character array
			============================================================*/
			else 
			if(bufcharIsLast(buf_str[chr_indx])) {
				 chr_indx = 0; 
				
				/*===========================================================
					String is a number
				============================================================*/				 
				 if(!isoperator(buf_str[0]))	{ 				
					 val = atoi(buf_str)	; 
				
					 if(stack_indx < 5) 
						stack[stack_indx++] = val;
					
					 //else stack full error message 				
				 }
				 
				 /*===========================================================
					String is an operation 
				============================================================*/	
				 else {		
					pop_exec(); //POP->POP->exec->PUSH 		
					
				 }
				 
				 /* Update LCD */ 
				 LCD_showstack();		  				 				 
				 				 
				/*Clear buffer string for next input */				 
				 buf_str[0] = buf_str[1] = buf_str[2] = buf_str[3]= buf_str[4]= 0; 

			}
			/*===========================================================
				Null not received, increment character index for next byte
			============================================================*/
			else
				chr_indx = chr_indx+1; 
		}
	}

}
/*===========================================================

Description: 
	Send Master function will convert the integer value 
	received into a string of characters in order to send 
	to a master device one byte at a time. 
	This function will use the GPIO to generate an interrupt 
	to the master for its current indexed byte in the 
	character array. At which point, will pause further execution
	until the ISR flag is set from the SPI ISR. 
	The index is incremented and the isdigit function in 
	the c standard library will determine a valid value. 
============================================================*/	
void send_master(int val) { 	
    char* buf; 
    char buffer[20]; 
    int i; 
    buf = itoa(val, buffer); 
 		
    do
    { 
		if(wrflag)  {
		
			i = S0SPSR;
			S0SPDR = *buf;
		    buf++; 
			IO0SET |= (1<<1); 	// Send intr to master
			IO0CLR |= (1<<1);   // Turn off intr to master.	
		  
			while(!isrflg);	    // wait for spi intr from master
			isrflg = 0;   		//Reset ISR flag
				
		}	
		
    }while(isdigit(*buf));
	
	/* Send null block */ 	
	i = S0SPSR;
	S0SPDR = 0x00;		// Send null to master to signal end
	IO0SET |= (1<<1); 	// Generate Interrupt to master
	IO0CLR |= (1<<1);   // Turn off Interrupt to master.		  
}
/*===========================================================
	Returns 1 if an invalid input was received. 
	A valid input is considered to be : 
				+ , -, *, / , or ascii 0-9 
===========================================================*/ 
int bufcharIsLast(char temp) {  
	if(isoperator(temp))		
		return 0;
	else
	if(temp <= 0x39 && temp >=0x30) //ascii digit between 0-9
		return 0; 
	else
		return 1; 

}
/*===========================================================
Description: 
	Pop And Execute function will pop 2 values from the stack 
	determine the operation to be executed, return the result
	to the top of stack and send the result to the master. 
===========================================================*/
void pop_exec(void) {	 

	 int val1, val2, finalval; 
	 char op; 
	 op = buf_str[0] ;
	 val1 = stack[--stack_indx]; 
	 val2 = stack[--stack_indx]; 

	 switch(op) {
	 	case '+': finalval = val1 + val2; 
				  break;
		case '-': finalval = val1 - val2; 
				  break;
		case '*': finalval = val1 * val2; 
				  break; 
		case '/': finalval = val1 / val2; 
				  break; 
		default : finalval = 0;
		}
	 
	 stack[stack_indx++] = finalval; 
	 send_master(finalval); 
}
/*===========================================================
Description: 
	The Integer to ASCII function will return a character 
	pointer to an array of characters representing the int
	value that was sent in the argument list. The second
	argument array is used as a buffer to hold and reference 
	the string of characters. 
Reference: 
	//http://cboard.cprogramming.com/
===========================================================*/

char* itoa(int i, char b[]){
    char const digit[] = "0123456789";
    char* p = b;
	  int shifter; 
    if(i<0){
        *p++ = '-';
        i *= -1;
    }
    shifter = i;
    do{ //Move to where representation ends
        ++p;
        shifter = shifter/10;
    }while(shifter);
    *p = '\0';
    do{ //Move back, inserting digits as u go
        *--p = digit[i%10];
        i = i/10;
    }while(i);
    return b;
}

/*===========================================================
Description: 
	ASCII to Integer function will convert and ascii string 
	to an integer value. 
Reference: 
//https://code.google.com/p/propgcc/source/
//browse/lib/stdlib/
===========================================================*/
int atoi(const char *str)
{
  int num = 0;
  int neg = 0;
  while (isspace(*str)) str++;
  if (*str == '-')
    {
      neg=1;
      str++;
    }
  while (isdigit(*str))
    {
      num = 10*num + (*str - '0');
      str++;
    }
  if (neg)
    num = -num;
  return num;
}

/*===========================================================
Description: 
	Checks if character is and operation:   +, -, *, / 
===========================================================*/
int isoperator(char oprator) {
	if(oprator <= 0x2F && oprator >= 0x2A)	 
		return 1; 
	else
		return 0; 
}

/*===========================================================
Description: 
	The LCD showstack function will translate the all values 
	on the stack into a character strings and display the values 
	on the LCD. 
===========================================================*/
void LCD_showstack(void) { 
	int   i, val; 
	char  buffer[20];
	char  *string ;
	lcd_clear();
	lcd_gotoxy(0, 0);
	
	for(i = 0; i<stack_indx; i++) { 
		val = stack[i]; 
		string = itoa(val, buffer);
  		while(*string != '\0' )	{  	
   			lcd_putchar( *string );
    		string++;
			if(	 *string == '\0' ) 
				lcd_putchar( ' ' );					
  		}
	}
}

/*********** SPI Config Block ********************************
//
//	SPI Slave configuration block & Pin Config Block 
// 
****************************************************************/	
void initSPI(void) { 

	unsigned char cpol_cpha = 0x18;  //Set Clk Polarity and Phase mode
	unsigned char spi_intr = 0x90; 	 //Set SPI intr enable 	; LSB first mode
	S0SPCR = spi_intr | cpol_cpha ;  //Set SPI control register 
	PINSEL0 |= 0x5500; 		         // SCLK0, MISO0, MOSI0, SSEL0 pin enable
	IO0DIR |= (1<<1);	
}

/*********** PLL Config Block ************************************
// 		CCLK = 60 Mhz 
// 		PCLK = 30 Mhz
// 		Full Enable MAM
// 		4 Clk Fetch 
****************************************************************/
void initPLL(void) {
	
	PLL0CFG = 0x24;
	PLL0CON = 0x01;
	PLL0FEED = 0xAA;
	PLL0FEED = 0x55;
	while(!(PLL0STAT & 1<<10)){};   //Set 60 Mhz CCLK
	PLL0CON = 0x03; 
	PLL0FEED = 0xAA;
	PLL0FEED = 0x55;
	VPBDIV =  0x02; 				//Set 30Mhz PCLK	
	MAMCR = 2;
	MAMTIM =  4;
}


/*********** ISR for SPI ********************************
//
//	Interrupt service routine
//
****************************************************************/	
void SPI_ISR(void)__irq { 

	 //check SPIF bit 
	 while(S0SPSR & 0x80) {	
		buf_str[chr_indx] = S0SPDR; 
		isrflg = 1; 
	 }	 
	 
	 S0SPINT = 0x01; 	 //clear SPIF bit
	 VICVectAddr = 0x00;

}
/*********** VIC Config Block ********************************
//
//	Vectored Interrupt Controller used to enable and assign 
//	slot X to timer0 (VIC Channel # for timer0 = 0x20)	
// 
****************************************************************/	
void initVIC(void) {
	VICVectAddr0=(unsigned)SPI_ISR;	// Pointer Interrupt Function (ISR)
	VICVectCntl0 = 0x20 | 0xA; 				// 0x20 enables Vectored IRQ slot 0xA enables SPI IRQ slot.
	VICIntEnable |= (1<<10); 				// Enable SPI interrupt
}
