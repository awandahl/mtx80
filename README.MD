

Hi Peder,

Very interested in having your feedback on the 80m MTX  and  the  ROX80.

Since your previous contact, I have been involved in a RSGB project which involved building 20-80m receivers
and 10-80m transmitters, which was quite an education!

The receiver - 
this was not the ROX80 I sold to you, but one with the same front-end.
The project manager wanted a through-hole design.
I also found the original ROX80 was tricky to assemble so wanted something a bit simpler.
I attach a copy of the manual for you to look at.

We found the use of a VXO with a ceramic resonator to be a more complicated subject than expected.
You can see in the manual my comments on that subject.
Surprised you got 100KHz bandspread using a red LED. 
I got the 3.51-3.60MHz  bandwidth by using the BBY66 varicap, which gives about 110pf at 0v and about 10pF at 5v.
A lot of older varicaps need a much higher reverse voltage to get the lowest capacity. 
I know some resonators will pull really easily, but I found those ones tend to drift or jump frequency as you tune near and below 3.58Mhz
so (eg.) you might  never get to tune 3.56Mhz or 3.57MHz !
Basically, I found that only Murata resonators gave reliable and repeatable results. 
I also discovered  the ferrite material used in the series inductor in series  has a big effect on the output power of the oscillator.
If the oscillator output drops too  low, the SA612 will operate with lower gain.
I built both rod and loop antenna versions of the receiver. Unsurprisingly,  the loop antenna gives significantly more sensitivity.

The transmitter.
For the RSGB project I had to design a new variation of the MTX80 transmitter because the project manager wanted to be able to have switchable frequency selection.
However I use the same fixed frequency MTX80 as you in my regular set of 12 TXs.
One thing I found was that on a few occasions when I tried to tune the ATU, in-the-field, the PIC reset itself. 
It was then a big problem to restart the TX whilst maintaining sync with other TXs.
As a result I now pre-set the ATU tuning and leave it untouched, after activating the TX, just in case !!
I suspected the compact design of the MTX80. Maybe  high RF getting into the PIC?  Maybe helped by hand capacitance when  tuning ????
Have you had this problem?  what enclosure have you used ? any photos of your TXs?


Comments appreciated

Dave G3ZOI.
 

------

Hi Peder,

I have revised the software to add the shutdown after 3 hours.
I measured a current of 2.2mA after shutdown.

I have compiled with standard settings. You can edit and re-compile as before, as you want.

In the 'constants' section.
     You can change the shutdown time.
    The line with  'timeLimit=18;' sets the run time to 3 hours. The hours have to be multiplied by 6, because it counts in 10min units

In the section  EEPROM LOAD 
      You can change the default callsign from 'ARDF'
      I have added SM0GMS and SM0HPL options.
      Edit, so only one line starts with '#rom '
      All others should start with '//' , which makes the whole line a comment statement and is ignored when compiling.



.In the 'void hunt_ini(void)' function section
   line 282: change nfoxes=2; to nfoxes=3;
   line 287: similar

hope that helps

73 Dave G3ZOI

![P1030488s](https://user-images.githubusercontent.com/62021989/180816382-b0e4e3c9-c564-41f7-8b08-de51294d2b89.JPG)

![DP1070563](https://user-images.githubusercontent.com/62021989/180816455-6e1a8562-d00f-4641-a982-c7652b89422a.JPG)

![AP1070557](https://user-images.githubusercontent.com/62021989/180816486-2e028221-edb9-4fd2-8f45-4b12b5aeb6f1.JPG)




