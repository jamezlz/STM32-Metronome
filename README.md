# STM32-Metronome

This project uses a STM32VLDISCOVER board, a 4 digit 7-segment display, 2 pushbuttons, and a piezo buzzer to make a metronome. The metronome features 10 tempos that can be toggled between. The metronome also accents every 0-9th beat.

This project was made in Keil uvision and uses the CMSIS core as well as basic startup and controller drivers.

### Pin Assignments

* PA0 - 7 segment 'A'
* PA1 - 7 segment 'B'
* PA2 - 7 segment 'C'
* PA3 - 7 segment 'D'
* PA4 - 7 segment 'E'
* PA5 - 7 segment 'F'
* PA6 - 7 segment 'G'
* PA8 - 7 segment digit 1 common cathode
* PA9 - 7 segment digit 2 common cathode
* PA10 - 7 segment digit 3 common cathode
* PA11 - 7 segment digit 4 common cathode
* PC6 - piezo buzzer signal 
* PB0 - tempo button (active low)
* PB1 - beats button (active low)
