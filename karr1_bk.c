/*
 * lcd.c:
 *	Text-based LCD driver.
 *	This is designed to drive the parallel interface LCD drivers
 *	based in the Hitachi HD44780U controller and compatables.
 *
 * Copyright (c) 2012 Gordon Henderson.
 ***********************************************************************
 * This file is part of wiringPi:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with wiringPi.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

//#include <glib.h>
//#include <glib/gprintf.h>
#include <unistd.h>


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <unistd.h>
#include <string.h>
#include <time.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <wiringPi.h>
#include <lcd.h>
//#include <linux/i2c-dev.h>
//#include <wiringPiI2C.h>
//unsigned char EncoderRead()
//{
//
//}
int Cnt = 0;
unsigned char CurVal;
unsigned char PrevVal;
int mcnt=0;


// I2C definitions

#define I2C_SLAVE	0x0703
#define I2C_SMBUS	0x0720	/* SMBus-level access */

#define I2C_SMBUS_READ	1
#define I2C_SMBUS_WRITE	0

// SMBus transaction types

#define I2C_SMBUS_QUICK		    0
#define I2C_SMBUS_BYTE		    1
#define I2C_SMBUS_BYTE_DATA	    2 
#define I2C_SMBUS_WORD_DATA	    3
#define I2C_SMBUS_PROC_CALL	    4
#define I2C_SMBUS_BLOCK_DATA	    5
#define I2C_SMBUS_I2C_BLOCK_BROKEN  6
#define I2C_SMBUS_BLOCK_PROC_CALL   7		/* SMBus 2.0 */
#define I2C_SMBUS_I2C_BLOCK_DATA    8

// SMBus messages

#define I2C_SMBUS_BLOCK_MAX	32	/* As specified in SMBus standard */	
#define I2C_SMBUS_I2C_BLOCK_MAX	32	/* Not specified but we use same structure */

// Structures used in the ioctl() calls

int ft0,ft1,ft2,ft3,ft4,ft5;

union i2c_smbus_data
{
  uint8_t  byte ;
  uint16_t word ;
  uint8_t  block [I2C_SMBUS_BLOCK_MAX + 2] ;	// block [0] is used for length + one more for PEC
} ;

struct i2c_smbus_ioctl_data
{
  char read_write ;
  uint8_t command ;
  int size ;
  union i2c_smbus_data *data ;
} ;

static inline int i2c_smbus_access (int fd, char rw, uint8_t command, int size, union i2c_smbus_data *data)
{
  struct i2c_smbus_ioctl_data args ;

  args.read_write = rw ;
  args.command    = command ;
  args.size       = size ;
  args.data       = data ;
  return ioctl (fd, I2C_SMBUS, &args) ;
}


/*
 * wiringPiI2CRead:
 *	Simple device read
 *********************************************************************************
 */

int wiringPiI2CRead (int fd)
{
  union i2c_smbus_data data ;

  if (i2c_smbus_access (fd, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data))
    return -1 ;
  else
    return data.byte & 0xFF ;
}


/*
 * wiringPiI2CReadReg8: wiringPiI2CReadReg16:
 *	Read an 8 or 16-bit value from a regsiter on the device
 *********************************************************************************
 */

int wiringPiI2CReadReg8 (int fd, int reg)
{
  union i2c_smbus_data data;

  if (i2c_smbus_access (fd, I2C_SMBUS_READ, reg, I2C_SMBUS_BYTE_DATA, &data))
    return -1 ;
  else
    return data.byte & 0xFF ;
}

int wiringPiI2CReadReg16 (int fd, int reg)
{
  union i2c_smbus_data data;

  if (i2c_smbus_access (fd, I2C_SMBUS_READ, reg, I2C_SMBUS_WORD_DATA, &data))
    return -1 ;
  else
    return data.word & 0xFFFF ;
}


/*
 * wiringPiI2CWrite:
 *	Simple device write
 *********************************************************************************
 */

int wiringPiI2CWrite (int fd, int data)
{
  return i2c_smbus_access (fd, I2C_SMBUS_WRITE, data, I2C_SMBUS_BYTE, NULL) ;
}

/*
 * wiringPiI2CWriteBlock:
 *	Simple device write
 *********************************************************************************
 */
/*
int wiringPiI2CWriteBlock (int fd, int nbyte, int *data)
{
  union i2c_smbus_data data ;

  data.word = value ;
  return i2c_smbus_access (fd, I2C_SMBUS_WRITE, reg, I2C_SMBUS_BLOCK_DATA, &data) ;

  //return i2c_smbus_access (fd, I2C_SMBUS_WRITE, data, I2C_SMBUS_BYTE, NULL) ;
}
*/

/*
 * wiringPiI2CWriteReg8: wiringPiI2CWriteReg16:
 *	Write an 8 or 16-bit value to the given register
 *********************************************************************************
 */

int wiringPiI2CWriteReg8 (int fd, int reg, int value)
{
  union i2c_smbus_data data ;

  data.byte = value ;
  return i2c_smbus_access (fd, I2C_SMBUS_WRITE, reg, I2C_SMBUS_BYTE_DATA, &data) ;
}

int wiringPiI2CWriteReg16 (int fd, int reg, int value)
{
  union i2c_smbus_data data ;

  data.word = value ;
  return i2c_smbus_access (fd, I2C_SMBUS_WRITE, reg, I2C_SMBUS_WORD_DATA, &data) ;
}


/*
 * wiringPiI2CSetupInterface:
 *	Undocumented access to set the interface explicitly - might be used
 *	for the Pi's 2nd I2C interface...
 *********************************************************************************
 */

int wiringPiI2CSetupInterface (const char *device, int devId)
{
  int fd ;

  if ((fd = open (device, O_RDWR)) < 0)
    return wiringPiFailure (WPI_ALMOST, "Unable to open I2C device: %s\n", strerror (errno)) ;

  if (ioctl (fd, I2C_SLAVE, devId) < 0)
    return wiringPiFailure (WPI_ALMOST, "Unable to select I2C device: %s\n", strerror (errno)) ;

  return fd ;
}


/*
 * wiringPiI2CSetup:
 *	Open the I2C device, and regsiter the target device
 *********************************************************************************
 */

int wiringPiI2CSetup (const int devId)
{
  int rev ;
  const char *device ;

  rev = piBoardRev () ;

  if (rev == 1)
    device = "/dev/i2c-0" ;
  else
    device = "/dev/i2c-1" ;

  return wiringPiI2CSetupInterface (device, devId) ;
}




/*
 * myInterrupt:
 *********************************************************************************
*/

void EncInterrupt1 (void)
{
  char ValP5,ValC5,ValC4,ValP4,lcnt=0;
  mcnt=0;
  //  delay(1);mcnt++;
  ValP4=digitalRead(4);
  delay(1);mcnt++;
  ValC4=digitalRead(4);
  while(((ValC4!=0)||(ValP4!=0))&&(lcnt<10))
    {
    ValP4=ValC4;
    delay(1);mcnt++;
    lcnt++;
    ValC4=digitalRead(4);
    }

  //if((ValC4==0)&&(ValP4==0)&&(lcnt<=10)){
  if((ValC4==0)&&(ValP4==0)){
     ValP5=digitalRead(5);
     delay(1);mcnt++;
     ValC5=digitalRead(5);
     lcnt=0;
     while((ValC5!=ValP5)&&(lcnt<10))
       {
	 ValP5=ValC5;
	 delay(1);mcnt++;
	 ValC5=digitalRead(5);
	 lcnt++;
       }
     if(ValC5==0) Cnt++;
     else Cnt--;
   
    }
   // delay(5);
  }

// ************************************************
void EncInterrupt2 (void)
{
  char ValP5,ValC5,ValC4,ValP4,lcnt=0;
  mcnt=0;
  
    ValP5=digitalRead(5);
     delay(1);mcnt++;
     ValC5=digitalRead(5);
     lcnt=0;
     while((ValC5!=ValP5)&&(lcnt<10))
       {
	 ValP5=ValC5;
	 delay(1);mcnt++;
	 ValC5=digitalRead(5);
	 lcnt++;
       }
     if(ValC5==0) Cnt++;
     else Cnt--;
   
    }


// ************************************************
int main (void)
{
  int i, j ,k;
  int fd1, fd2 ; 
  int wifimode =0;

  char message1 [256] ;
  char message2 [256] ;
  char buf1 [30] ;
  char buf2 [30] ;
  char sBuf[512];
  unsigned char i2cdata[3];
  unsigned char I2COutBufA[7]={0xc7,0xc8,0x01,0x00,0x10,0x71,0x20};
  unsigned char I2COutBufB[7]={0xc7,0xc8,0x01,0x00,0x20,0x61,0x20};

//	const gchar *buffer;

  //  unsigned char CurVal;
  //  unsigned char PrevVal;
  //  int Cnt=0;

  struct tm *t ;
  time_t tim ;

  printf ("WMSO28 Mode Selector\n\r");
//  printf ("GPIO %x %x %x %x\n\r",getAlt(0),getAlt(1),getAlt(2),getAlt(3));

  if (wiringPiSetup () == -1)
    exit (1) ;

  if (wiringPiISR (4, INT_EDGE_FALLING, &EncInterrupt1) == -1)
    exit (1);
  //{
  //    fprintf (stderr, "Unable to setup ISR: %s\n", strerror (errno)) ;
  //    return 1 ;
  //  }


  fd1 = lcdInit (2, 16, 4, 11, 10, 0,1,2,3,0,0,0,0) ;

  if (fd1 == -1)
  {
    printf ("lcdInit 1 failed\n") ;
    return 1 ;
  }

	pinMode (4, INPUT) ; 	// Pin 4 EncA  
	pinMode (5, INPUT) ; 	// Pin 5 EncB
	pinMode (6, INPUT) ; 	// Pin 6 EncSw
	CurVal = digitalRead(5);
	PrevVal = CurVal;

//  sleep (1) ;

	lcdPosition (fd1, 0, 0) ; lcdPuts (fd1, "   I2C test    ");
//  sleep (2) ;
	k=0;
/*	ft0 = wiringPiI2CSetup(0x10);
	if (ft0 == -1)
	{
		printf("I2Cinit global failed\n\r");
		return 1;
	}
	printf("I2C 0x20 %x\n\r",ft0);

	ft1 = wiringPiI2CSetup(0x11);
	if (ft1 == -1)
	{
		printf("I2Cinit target 1 failed\n\r");
		return 1;
	}
	printf("I2C 0x21 %x\n\r",ft1);
*/
//	if ((ft2 = wiringPiI2CSetup(0x12))<0)
//	{
//		printf("I2Cinit target 2 failed\n\r");
//		return 1;
//	}
//	printf("I2C 0x12 %x\n\r",ft2);

//	ft2 = wiringPiI2CSetup(0x12);


  printf("loading setup\n");

  ft2 = open("/dev/i2c-1",O_RDWR);
	if(ft2<0){
		printf("Error opening file: %s\n",strerror(errno));
		return 1;
	}
	if(ioctl(ft2,I2C_SLAVE,0x12)<0){
		printf("ioctl error: %s\n",strerror(errno));
		return 1;
	}

//	usleep(10);

  i2cdata[0]=0x40;//start delay
  i2cdata[1]=0x00;
  i2cdata[2]=0x80;

	if(write(ft2,&i2cdata,3)<0)
	{
		printf("Error writing 1-1: %s\n",strerror(errno));
	}
//	usleep(10);

  i2cdata[0]=0x00;//color/hit/timeout
  i2cdata[1]=0x24;
  i2cdata[2]=0x80;

	if(write(ft2,&i2cdata,3)<0)
	{
		printf("Error writing 1-2: %s\n",strerror(errno));
	}

//	usleep(10);

  i2cdata[0]=0x41;//start delay
  i2cdata[1]=0x01;
  i2cdata[2]=0x00;

	if(write(ft2,&i2cdata,3)<0)
	{
		printf("Error writing 2-1: %s\n",strerror(errno));
	}
//	usleep(10);

  i2cdata[0]=0x01;//color/hit/timeout
  i2cdata[1]=0x48;
  i2cdata[2]=0x80;

	if(write(ft2,&i2cdata,3)<0)
	{
		printf("Error writing 2-2: %s\n",strerror(errno));
	}

//  close(ft2);


	sleep(2);
  printf("Arming\n");

  ft0 = open("/dev/i2c-1",O_RDWR);
	if(ft0<0){
		printf("Error opening file: %s\n",strerror(errno));
		return 1;
	}
	if(ioctl(ft0,I2C_SLAVE,0x10)<0){
		printf("ioctl error: %s\n",strerror(errno));
		return 1;
	}

	usleep(100);

  i2cdata[0]=0xc9;
  i2cdata[1]=0xc9;
  i2cdata[2]=0xc9;

	if(write(ft0,&i2cdata,3)<0)
	{
		printf("Error writing arm: %s\n",strerror(errno));
	}

//	pinMode (8, INPUT) ; 	// Pin 2 SDA


	while(1) printf("%x\r",digitalRead(8));



//  i2cdata[0]=0x00;
//  i2cdata[0]=0x24;
//  i2cdata[0]=0x20;

//	if(write(ft2,&i2cdata,3)!=1)
//	{
//		printf("Error writing 2: %s\n",strerror(errno));
//	}

/*
	printf("I2C 0x12 %x\n\r",ft2);

	while(wiringPiI2CWriteReg16(ft2,0x40,0x1000)<0);

  sleep(1);
	while(wiringPiI2CWriteReg16(ft2,0x00,0x2420)<0);


	printf("I2C found\n\r");
*/
/*
	if(!ft1){
		for(i=0;i<7;i++){
		wiringPiI2CWrite(ft1,I2COutBufA[i]);
		getchar();
		}
		printf("I2C data A transmitted\n\r");
	}
	
	if(!ft2){
	for(i=0;i<7;i++){
		wiringPiI2CWrite(ft2,I2COutBufB[i]);
		getchar();
	}
	printf("I2C data B transmitted\n\r");
	}
	  sleep (1) ;
	getchar();
	wiringPiI2CWrite(ft0,0xc9);
*/
//	printf("I2C armed\n\r");





/*
  while(!k)
  {
    i = 0 ;
    j = 0 ;
	if(!k) sprintf (buf2, " Mode Selected  ");
	else sprintf (buf2, "                ");

    while(!k)
    {

      //      CurVal = (digitalRead(4)<<1)|digitalRead(5);



      //      tim = time (NULL) ;
      //      t = localtime (&tim) ;

      //      sprintf (buf1, "    %02d:%02d:%02d    ", t->tm_hour, t->tm_min, t->tm_sec) ;
	if(((Cnt/2)*2)==Cnt) wifimode=0;
	else wifimode = 1;

	if(!wifimode)
  		 sprintf (buf1, "    Wifi Host   ");
	else
  		 sprintf (buf1, "   Wifi Device  ");

//   sprintf (buf1, "    %04d/%04d      ", Cnt,mcnt);
      if (digitalRead (6) == 1){
	lcdPosition (fd1, 0, 1) ;
	lcdPuts (fd1, buf1);
      }
      else{
	Cnt = 0;
	lcdPosition (fd1, 0, 0) ;
	lcdPuts (fd1,buf1);
	lcdPosition (fd1, 0, 1) ;
	lcdPuts (fd1,buf2);
	k=1;
      }
 //     sprintf (buf1, "%02d/%02d/%02d", t->tm_mday, t->tm_mon + 1, t->tm_year+1900) ;
 //     lcdPosition (fd1, 0, 3) ;
 //     lcdPuts (fd1, buf1) ;

      delay (1) ;
    }

  }
*/
/*
 if(!wifimode)   
    sprintf (sBuf,"sh /root/wmso_host.sh");
  else
    sprintf (sBuf,"sh /root/wmso_device.sh");

  system(sBuf);
*/

  return 0 ;
}
