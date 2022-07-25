/*
---------- by G3ZOI - MTX80  -------------
Based on Original Code by VE2EMM		Date: Jan 2003
Modified by G3ZOI                 		Date: Mar 2018

Added comfort/pre-event transmissions at 5sec intervals

------------------  Project Details -------------------

PIC16F628 Xtal =4.194304 mHz

-------------------------------------------------------

	DIT = 1 period
	DAH = 3 periods
	Space between character = 1 periods
	Space between letters = 3 periods
	Space between words = 7 periods

----- COMPILIER OPTIONS --------------------------


WAIT TIME OPTIONS
-----------------
  WAIT_1HR
     1 - 31 hr in 1 hr steps

  WAIT_30MIN
       1/2hr to 1-15/2 hr in 1/2hr steps

  WAIT_GAP
       1/2hr to  7-1/2 hr in 1/2 steps then
       16hr - 31hr in 1 hr steps

EEPROM
------
  byte[0]  = MO number & speed  (0 = SLOW TX1)


REM OUT THE UNWANTED DEFINES
*/

// WAIT FORMAT
#define WAIT_1HR
//#define WAIT_30MIN
//#define WAIT_GAP

// PIC DEVICE
#include <16F628A.h>

// EEPROM LOAD : TX-IDENT & CALLSIGN
#define EEfrom 0x00
#rom 0x2100={0x00,0x06,0x0a,0x09,0x14,0xff}// Tx1+ARDF

//-----------------------------------------------------
#fuses XT,NOWDT,PUT,NOLVP, NOBROWNOUT, NOPROTECT, NOMCLR
#use delay (clock=4194304)
#use fast_io(A)
#use fast_io(B)

//----------------- port variables ------------------


#define greenLed pin_A0
#define syncPort pin_A1
#define keyPort pin_A4
#define progPort pin_A5

//---------------------------------------------------------
int1 const led_on = 1;
int1 const led_off = 0;

int8 const call[3]={0x00,0x87,0xff};// call=3 dashes if nothing else set
int1 flash_led=0;
int1 flag_led=led_off;
int1 tx_ok=0;
int1 sync_set=0;
int1 sync_state=0;
int1 got_call=0;
int1 keyed=0;
int1 send_ident=1;
int1 is_foxo=0;

int8 i=0;// temporary variables
int8 j=0;

int8 tic=16; // timer interrupt counter = 1/16 sec
int8 seconds=0;
int8 cycle_nr=1;// cycle count in a 10min period
int8 wait10=0; //delay time in 10 minute units
int8 countdown=0; //delay countdown in 10 min units
int8 cw_char=0;
int8 speed_cw=10;//default speed of CW in WPM
int16 num_periods=0;
int8 dip_8=0;//full dip switch
int8 dip_3=0;//
int8 nfoxes=5;//number of foxes in the hunt
int8 fox_cycle=0;// active slot in the TX cycle
//int16 cycle_time=0;//
int8 tx_num=0;//transmission id (0 to 5)
int8 tx_mem=0; //from eeprom location(0)
int8 start_slot=0;//(0 to 8)
int8 on_time=30;//default transmit period in seconds
int8 call_len=0;//
int8 end_len=0;
int8 key_until=255;//seconds to abort keying
int8 n5mins=0;//timer counter. Number of on-time periods in 5 minutes;
int8 n10mins=0;// timer counter. Number of on-time periods in 10 minutes;
int8 tmpw;
int8 tmps;
int8 tx_mode=0;
int8 comfort=6; // 1hour (in 10 min units)

//--------------------- constants ---------

int8 id[6] ={0x00,0x02,0x04,0x08,0x10,0x20}; //MO,MOe,i,s,h,5 (classic)
int8 fo[12]={0x06,0x11,0x15,0x09,0x02,0x14,  //A,B,C,D,E,F (foxOring)
             0x0b,0x10,0x04,0x1e,0x0d,0x12}; //G,H,I,J,K,L (foxOring)
int8 de[3]={0x09,0x02,0x00};// DE
// cw speeds
int8 wpm_fox; // active speed
int8 const wpm_slow=10;
int8 const wpm_fast=15;
int8 const wpm_mem=10;
int8 const wpm_end=30;



//----------------- Declare functions ---------------------------------

void pic_ini(void);
void hunt_ini(void);
void timer_ini(void);
void wait_time(void);
void hunt_active(void);//hunt in progress
void clock(void);      //timer interrupt
void wave(void);       //timer1 interrupt
void timing(void);// cycle timing
void sync_tx(void);
void save_callsign(void);//load call-sign
void save_TXnum();
void call_length(void);//
void one_letter(char cw_char);
void key_down(char nom_dit);//key CW tone
void key_up(char nom_dit);//key CW spaces
void key_len(char nom_dit); //key up/down period
void send_callsign();// end of transmission id
void send_id(void);// MOx or foxO code
void tune_up(void);
void rapid_flash(void);



//****************************** Main Programme ********************

main()
{
   delay_ms(1);
   flash_led=0;

   pic_ini();
   hunt_ini();

   output_bit(greenLed,led_on);
   i=input(syncPort)+input(progPort)*2; //0-3

   switch(i)
   {
      case 2:
          sync_tx();
      break;

      case 1:
          save_TXnum();
      break;

      case 0:
          save_callsign();
      break;
   }

   // start event
   timer_ini(); // start timing
	wait_time(); // wait loop
	hunt_active(); // event run loop
}




//************ End of main ************


void rapid_flash(void) {
     delay_ms(100);
     output_bit(greenLed, !input(greenLed)); //rapid flash led;
}



void pic_ini(void)//--------------------------------------
{
	output_A(0x00);
	output_B(0x00);
	set_tris_A(0b00101110);// 1,2,3,5 input
	set_tris_B(0b11111111);// all input
	port_b_pullups(true);

	// Setup Main Timer
	set_rtcc(0);
	setup_counters(rtcc_internal,rtcc_div_256); // = 16 ints per second
	enable_interrupts(INT_RTCC);

   enable_interrupts(GLOBAL);
}




void timer_ini(void)
{
   tic=16;
   seconds=0;
   cycle_nr=1;
   fox_cycle=1;
   tx_ok=1;
   key_until=255;
}




void hunt_ini(void)//---------------------------------------
{

// read eeprom
   tx_mem = read_eeprom(0);  // (0-11)
   tx_num = (tx_mem+1)%6; //0-5

   if (tx_mem>5)
       wpm_fox=wpm_fast;
   else
       wpm_fox=wpm_slow;


// read the dip switches into dip_8
    dip_8=input_b();
    dip_8=dip_8^0xFF;       //read dip state bits and invert
    dip_3=dip_8&0b00000111; //set unwanted bits to zero



      switch(dip_3) // dip 0,1,2 sets event mode
      {
        case 0: // IARU Classic 60
          nfoxes=5;
          on_time=60;
        break;

        case 1: //
          nfoxes=5; //Classic 30
          on_time=30;
        break;

        case 2: // IARU Sprint
          nfoxes=5;
          on_time=12;
        break;

        case 3: // Practice Classic 60
          nfoxes=2;
          on_time=60;
        break;

        case 4: // // Practice Classic 30
          nfoxes=2;
          on_time=30;
        break;

        case 5: // Practice Sprint
          nfoxes=2;
          on_time=12;
        break;

        case 6: // FoxOring Continuous
          is_foxo=1;
          nfoxes=1;
          on_time=60;
          wpm_fox=wpm_slow;
        break;

        case 7: // FoxOring 30/30
          is_foxo=1;
          nfoxes=2;
          on_time=30;
          wpm_fox=wpm_slow;
        break;
      }

   // MO beacon
   if ((tx_num==0)&& (dip_3<6))
   { nfoxes=1;
     on_time=120;
   }



   n10mins=600/on_time; // number of tx slots in 10mins.
   n5mins=n10mins/2;    // number of tx slots in  5mins.

   countdown=0; //to start immediately, (updated in sync_tx)
   start_slot=1;//to start immediately, (updated in sync_tx)


// WAIT TIME
    wait10=(dip_8&0b11111000); //set unwanted bits to zero
    wait10=wait10>>3;          //drop 3 right end bits

// WAIT TIME OPTIONS
#ifdef WAIT_1HR
    wait10=wait10*6 ;      //convert to 10min units
#endif

#ifdef WAIT_30MIN
    wait10=wait10*3 ;      //convert to 10min units
#endif

#ifdef WAIT_GAP
    if(bit_test(wait10,4)) //if 5th bit set then delay range=16-31hrs
      wait10*=2;           //multiple delay by 2
    wait10*=3;             //convert to 10min units
#endif

    port_b_pullups(false); // disable to save power

}




void sync_tx(void)//--------------------------------------
{
   flag_led=!flag_led;

   while(!input(syncPort)) rapid_flash();// wait until sync switch open
   timing();
   countdown=wait10;//from dip setting
   if (tx_num>0) start_slot=tx_num % nfoxes; // otherwise start_slot=1
   if (start_slot==0) start_slot=nfoxes;
}



void wait_time(void)//--------------------- wait before the hunt
{
	if(countdown>0)
	{
      flash_led = 1;
      speed_cw=wpm_end;

      while(countdown>0)
      {
        timing();
        // comfort signal
        if( (wait10>comfort) &&// only if more than 1 hour delay
            (countdown<=comfort) && //
            (send_ident==1) && // every 5mins
            (seconds==(tx_num -1) * 5) // offset each tx in 5 second steps
          ) { // send comfort signal
            send_id();//MOx
            send_ident=0;
          }
        if(!input(syncPort))tune_up();
      }

	}
   flash_led = 0;
   output_bit(greenLed, led_off);

}



void hunt_active(void)//---------------- hunt loop ---------------------
{
	while(true)
	{
	   timing();
      if(((start_slot==fox_cycle)||(start_slot==0)) && (tx_ok==1))
      {
        //flash_led = 0;
        //output_bit(greenLed, led_on);

        if(got_call==0) {
          call_length();
        }
        speed_cw=wpm_fox;
        key_until=on_time;

        if (send_ident==1) {
           key_until=key_until-call_len;
        }
        else if(is_foxo==0) key_until=key_until-1;

	 	  speed_cw=wpm_fox;
        while(seconds<key_until) send_id();//MOx

        // send call sign or 'K' or nothing for foxOring
        key_until=255;
        speed_cw=wpm_end;
        if (send_ident==1) {
           send_callsign();
           send_ident=0;
        }
        else
          if(is_foxo==0) one_letter(0x0d); //send K to finish

        tx_ok=0;//only reset by timing();
	   }//end of TX-on period
      else {
        flash_led=1;
        if(!input(syncPort))tune_up();
      }

	}//main loop
}//hunt_active

//***********************************************************************

//-------- interrupt function  ------------------
#int_TIMER0//
clock()
{
   if (flash_led) output_bit(greenLed, led_off); // max 1/16" on
	if(--tic==0)   //interrupt second count
	{
      // flash once in every 3 seconds
      if (flash_led && ((seconds % 3) == 0)) output_bit(greenLed, led_on );
		tic=16;      // 16 tics = 1 second
		++seconds;   // increment seconds
	}
}


//---------------------Other Functions -----------------------

void timing(void)// increment longer timing units from interupt overflow
{
	if(seconds>=on_time)
	{
		seconds=seconds-on_time;// seconds=0 or adjust for overrun
	   if(++cycle_nr>n10mins)  // countdown timer in 10min steps
	   {  cycle_nr=1;
		   if(countdown>0)countdown--;
	   }
      if((cycle_nr % n5mins)==1) send_ident=1; // send ident every 5mins
      // fox cycle
      fox_cycle=cycle_nr%nfoxes;
      if (fox_cycle==0) fox_cycle=nfoxes;

      tx_ok=1;

   }
}



void save_txnum()//----Save MO ID -------
{
   speed_cw=wpm_mem;
   key_up(3);//word space
   delay_ms(100);
   tx_mem=read_eeprom(0);
	while (true) // until switch off
	{
     output_bit(greenLed, led_off);
     tx_num=(tx_mem+1)%6;
     if ((tx_mem>5) && !is_foxo)
        speed_cw=wpm_fast;
     else
        speed_cw=wpm_slow;

     send_id();
     write_eeprom(0,tx_mem);
	  while(input(syncPort)) rapid_flash(); //wait until button is depressed
     ++tx_mem;
     if (tx_mem>=sizeof(fo)) {tx_mem=0;}
   }
}




void save_callsign(void)//----- Save Callsign ID ------
{
   port_b_pullups(true);
   speed_cw=wpm_slow;
   one_letter(0x09);//d
   one_letter(0x02);//e
   key_up(3);//space
   send_callsign();
   // save new callsign at byte 1
	i=1;
	do
	{
	  while(input(syncPort)) rapid_flash(); //wait until button is depressed
     output_bit(greenLed, led_off);
	  dip_8=input_b()^ 0xff;// invert dip switch code
     switch(dip_8){

       case(0x00):
       break; // do nothing

       case(0x01):{// load DE+space

          for(j=0;j<=2;j++)
          {
            write_eeprom(i,de[j]);
            write_eeprom(i+1,0xff);//next character=stop
            i++;
          }
       }
       break;

       case(0x80):{// load default call
          j=0;
          while(call[j]!=0xff)
          {
            write_eeprom(i,call[j]);
            write_eeprom(i+1,0xff);//next character=stop
            j++;
            i++;
          }
       }
       break;

       case(0xff):
          write_eeprom(i,dip_8);//save stop character
       break;

       default: {
          write_eeprom(i,dip_8);//save character
          write_eeprom(i+1,0xff);//next character=stop
	       i++;
       }
     }//end switch

     // send new callsign

     send_callsign();// send current callsign


   } while(dip_8!=0xff);//do loop
}



void tune_up(void) // only call when TX is idle
{
  flash_led = 0;
  output_bit(greenLed,led_on);
  while (!input(syncPort)) // tune up when port closed
  {
    output_high(keyPort);
  }
  output_low(keyPort);
  flash_led = 1;
  output_bit(greenLed, led_off);
}



//************************ generation CW ******************


send_callsign ()//
{
   key_up(7);//space
   i=1;
   while(read_eeprom(i)!=0xff)
   {
	  one_letter(read_eeprom(i));
	  i++;
	}
}


void send_id()
{
     key_up(3);      // character space
     if (is_foxo)
        one_letter(fo[tx_mem]);
     else
     {
        key_up(3);          //extra to word space
        one_letter(0X07);   //M
        one_letter(0x0f);   //O
        one_letter(id[tx_num]);
     }
}



one_letter(char cw_char)//------ send a character -----------
{
   if ((seconds<key_until) || ((seconds==key_until) && tic>=12))// minimum 1 second to send
   {
	   if(cw_char==0) key_up(3);
	   if(cw_char>128)
      {// long dahs
	      while(cw_char>1)
	     {
		    if(shift_right(&cw_char,1,0))key_down(speed_cw); //one second
		    key_up(1);
	     }
      }

	   while(cw_char>1)
	   {
		   if(shift_right(&cw_char,1,0))
		   {
			   key_down(3);//dah
			   key_up(1);
		   }
		   else
		   {
			   key_down(1);//dit
			   key_up(1);
		   }
	   }
      key_up(2);
   }
}


void call_length(void) { // time to send callsign
   speed_cw=wpm_end;
   j=seconds;// current time
   if(read_eeprom(1)!=0xff){
      send_callsign();
      key_up(3);// margin
   }
   call_len=seconds-j; //lapsed time
   got_call=1;
}


void key_len(char nom_dit){  //key length up/down
  num_periods = (1000/speed_cw)*nom_dit;
}


void key_down(char nom_dit)//------------ dits --------------
{
     key_len(nom_dit);
     output_high(keyPort);
	  while((num_periods--) != 0) delay_ms(1);
     output_low(keyPort);
}

void key_up(char nom_dit)//---------- spaces -------------
{
     key_len(nom_dit);
     output_low(keyPort);
	  while((num_periods--) != 0) delay_ms(1);
}

//************************ end of CW routines ******************

//----------------------- end of programme ----------------------

/*
=== SNIP ==================================================================

 MORSE ENCODING ...

 One morse character per BYTE, bitwise, LSB to MSB.

 0 = dit, 1 = dah.  The byte is shifted to the right bit by bit, until the
 last 1 is left, this 1 is an END OF CHARACTER indicator.
 A maximum of 7 elements can be encoded, (error) is excluded.

     CODE               HEX    Comments
    ------              ---    --------------------
 KN -.--.   00101101	    2d    Go only
 SK ...-.-  01101000  	 68	 Clear
 AR .-.-.   00101010  	 2a	 Over, end of message
 BT -...-   00110001  	 31	 Pause
 AS .-...   00100010	    22	 Wait, stand by
  / -..-.   00101001  	 29
  0 -----   00111111  	 3f
  1 .----   00111110  	 3e
  2 ..---   00111100  	 3c
  3 ...--   00111000  	 38
  4 ....-   00110000  	 30
  5 .....   00100000  	 20
  6 -....   00100001  	 21
  7 --...   00100011  	 23
  8 ---..   00100111  	 27
  9 ----.   00101111  	 2f
  A .-      00000110  	 06
  B -...    00010001  	 11
  C -.-.    00010101  	 15
  D -..     00001001  	 09
  E .       00000010  	 02
  F ..-.    00010100  	 14
  G --.     00001011  	 0b
  H ....    00010000  	 10
  I ..      00000100  	 04
  J .---    00011110  	 1e
  K -.-     00001101  	 0d
  L .-..    00010010  	 12
  M --      00000111  	 07
  N -.      00000101  	 05
  O ---     00001111  	 0f
  P .--.    00010110  	 16
  Q --.-    00011011  	 1b
  R .-.     00001010  	 0a
  S ...     00001000  	 08
  T -       00000011  	 03
  U ..-     00001100  	 0c
  V ...-    00011000  	 18
  W .--     00001110  	 0e
  X -..-    00011001  	 19
  Y -.--    00011101  	 1d
  Z --..    00010011  	 13
  SPACE	   00000000  	 00   Word space (special exception)
  EOM       11111111  	 ff	End of message  (special exception)

=== SNIP ==================================================================
*/


