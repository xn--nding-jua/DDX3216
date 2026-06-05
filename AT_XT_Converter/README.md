
# AT/XT-Converter

PS/2 -> PC/XT Arduino based Keyboard adapter originally by kesrut and updated by AndersBNielsen: https://github.com/AndersBNielsen/pcxtkbd

This version has IO changed to "open collector" to minimize the risk of shorting pins by mistake(frying the XT) and to conform with standard.

Original version also had an issue that would cause an IBM XT to enter "Manufacturing mode" after POST so keyboard would only work 50% of the time in Cassette Basic.

Video about the subject of AndersBNielsen, including the issues mentioned above and the different keyboard protocols: Building an IBM PC XT from SCRATCH https://youtu.be/fGLiPh0byCo

Please add https://github.com/techpaul/PS2KeyAdvanced Arduino library to compile.

More Information: http://www.ccgcpu.com/2019/02/14/the-xt-part-4-adapting-a-modernish-keyboard/


## Hardware-Connection

    Connector to XT-System:
          ****   *****
       *      ***      *       1 = XT_CLK  -> Pin D2
     *                   *     2 = XT_DATA -> Pin D5
    *    3           1    *    3 = not connected
    *                     *    4 = GND
     *     5       4     *     5 = VCC (5V)
       *       2        *
           **********

    Connector to AT-System:
          ****   *****
       *      ***      *       1 = KBDCLK  -> Pin D3
     *                   *     2 = KBDDATA -> Pin D4
    *    3           1    *    3 = KBDRST (not used / unconnected)
    *                     *    4 = GND
     *     5       4     *     5 = VCC (5V)
       *       2        *
           **********

    In case you want to connect an USB-keyboard with PS/2-support:

    ***********************************   1 = VCC (5V)
    *                                 *   2 = D- = PS2DATA -> Pin D4
    *                                 *   3 = D+ = PS2CLK  -> Pin D3
    *      1      2     3     4       *   4 = GND
    ***********************************


Detailed Schematics can be found here:
https://github.com/AndersBNielsen/pcxtkbd/blob/master/PS2XTadapter/PS2XTadapter.pdf