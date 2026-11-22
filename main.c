#include "MK64F12.h"

#include <stdint.h>

#include <stdbool.h>

#include <stdlib.h>
 
volatile uint32_t msTicks = 0;
 
void SysTick_Handler(void) {

    msTicks++;

}
 
void delay_ms(uint32_t ms) {

    uint32_t start = msTicks;

    while ((msTicks - start) < ms);

}
 
void delay_short(void) {

    for (volatile int i = 0; i < 60; i++);

}
 
// Common Anode 7-Segment codes (0-9) and digit select (left -> right)

const uint8_t segDigits[10] = { 0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90 };

const uint8_t segSelect[4]  = { 0xF1, 0xF2, 0xF4, 0xF8 };

uint8_t displayBuf[4]       = { 0x8C, 0xC7, 0x88, 0x91 }; // "PLAY"
 
const char keymap[4][3] = {

    {'1', '2', '3'},

    {'4', '5', '6'},

    {'7', '8', '9'},

    {'*', '0', '#'}

};
 
// Fix 5V PNP Buzzer on 3.3V K64F: Use High-Z Input for OFF, Output LOW for ON

void buzzerOff(void) {

    PTA->PDDR &= ~(1U << 1); // Set PTA1 as High-Z Input (cuts off 5V PNP base current)

}
 
void buzzerOn(void) {

    PTA->PCOR = (1U << 1);   // Set output latch LOW

    PTA->PDDR |= (1U << 1);  // Enable PTA1 as Output LOW

}
 
void initHardware(void) {

    // Enable Clock for PORT A, B, C, D

    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTB_MASK |

                  SIM_SCGC5_PORTC_MASK | SIM_SCGC5_PORTD_MASK;
 
    // 1. 7-Segment Display (74HC595): Latch=PTB23(D4), CLK=PTC3(D7), DATA=PTC12(D8)

    PORTB->PCR[23] = PORT_PCR_MUX(1);

    PORTC->PCR[3]  = PORT_PCR_MUX(1);

    PORTC->PCR[12] = PORT_PCR_MUX(1);

    PTB->PDDR |= (1U << 23);

    PTC->PDDR |= (1U << 3) | (1U << 12);
 
    // 2. 4 LEDs (Lives): D1=PTD1, D2=PTD3, D3=PTD2, D4=PTD0

    PORTD->PCR[0] = PORT_PCR_MUX(1);

    PORTD->PCR[1] = PORT_PCR_MUX(1);

    PORTD->PCR[2] = PORT_PCR_MUX(1);

    PORTD->PCR[3] = PORT_PCR_MUX(1);

    PTD->PDDR |= (1U << 0) | (1U << 1) | (1U << 2) | (1U << 3);
 
    // 3. Buzzer = PTA1(D3) -> GPIO mode, no pull resistors, High-Z (OFF)

    PORTA->PCR[1] = PORT_PCR_MUX(1);

    buzzerOff();
 
    // 4. Difficulty Switches: S1=PTB3(A1), S2=PTB10(A2), S3=PTB11(A3)

    PORTB->PCR[3]  = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;

    PORTB->PCR[10] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;

    PORTB->PCR[11] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;

    PTB->PDDR &= ~((1U << 3) | (1U << 10) | (1U << 11));
 
    // 5. Keypad Rows (Output): R1=PTA2(~5), R2=PTC2(~6), R3=PTC4(~9), R4=PTC10(A5)

    PORTA->PCR[2]  = PORT_PCR_MUX(1);

    PORTC->PCR[2]  = PORT_PCR_MUX(1);

    PORTC->PCR[4]  = PORT_PCR_MUX(1);

    PORTC->PCR[10] = PORT_PCR_MUX(1);

    PTA->PDDR |= (1U << 2);

    PTC->PDDR |= (1U << 2) | (1U << 4) | (1U << 10);
 
    // 6. Keypad Columns (Input + Pull-Up): C1=PTC11(A4), C2=PTB9(D2), C3=PTC17(D1)

    PORTC->PCR[11] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;

    PORTB->PCR[9]  = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;

    PORTC->PCR[17] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;

    PTC->PDDR &= ~((1U << 11) | (1U << 17));

    PTB->PDDR &= ~(1U << 9);

}
 
void shiftOutByte(uint8_t val) {

    for (int i = 0; i < 8; i++) {

        if (val & (0x80 >> i)) {

            PTC->PSOR = (1U << 12);

        } else {

            PTC->PCOR = (1U << 12);

        }

        PTC->PSOR = (1U << 3);

        delay_short();

        PTC->PCOR = (1U << 3);

    }

}
 
void refreshDisplay(void) {

    for (int d = 0; d < 4; d++) {

        PTB->PCOR = (1U << 23);

        shiftOutByte(displayBuf[d]);

        shiftOutByte(segSelect[d]);

        PTB->PSOR = (1U << 23);

        delay_ms(2);

    }

}
 
void waitWithDisplay(uint32_t durationMs) {

    for (uint32_t t = 0; t < durationMs; t += 8) {

        refreshDisplay();

    }

}
 
void updateLives(int lives) {

    if (lives >= 1) PTD->PCOR = (1U << 1); else PTD->PSOR = (1U << 1); // D1

    if (lives >= 2) PTD->PCOR = (1U << 3); else PTD->PSOR = (1U << 3); // D2

    if (lives >= 3) PTD->PCOR = (1U << 2); else PTD->PSOR = (1U << 2); // D3

    if (lives >= 4) PTD->PCOR = (1U << 0); else PTD->PSOR = (1U << 0); // D4

}
 
void beep(uint32_t durationMs) {

    buzzerOn();

    waitWithDisplay(durationMs);

    buzzerOff();

}
 
void setAllRowsHigh(void) {

    PTA->PSOR = (1U << 2);

    PTC->PSOR = (1U << 2) | (1U << 4) | (1U << 10);

}
 
void setRowLow(int r) {

    setAllRowsHigh();

    if (r == 0) PTA->PCOR = (1U << 2);

    else if (r == 1) PTC->PCOR = (1U << 2);

    else if (r == 2) PTC->PCOR = (1U << 4);

    else if (r == 3) PTC->PCOR = (1U << 10);

}
 
bool isColLow(int c) {

    if (c == 0) return (PTC->PDIR & (1U << 11)) == 0; // C1 (A4)

    if (c == 1) return (PTB->PDIR & (1U << 9))  == 0; // C2 (D2)

    if (c == 2) return (PTC->PDIR & (1U << 17)) == 0; // C3 (D1)

    return false;

}
 
char scanKeypad(void) {

    for (int r = 0; r < 4; r++) {

        setRowLow(r);

        delay_short();

        for (int c = 0; c < 3; c++) {

            if (isColLow(c)) {

                delay_ms(12); // Debounce 12 ms

                if (isColLow(c)) {

                    setAllRowsHigh();

                    return keymap[r][c];

                }

            }

        }

    }

    setAllRowsHigh();

    return '\0';

}
 
int readDifficultySwitch(void) {

    if ((PTB->PDIR & (1U << 3))  == 0) return 1; // S1

    if ((PTB->PDIR & (1U << 10)) == 0) return 2; // S2

    if ((PTB->PDIR & (1U << 11)) == 0) return 3; // S3

    return 0;

}
 
// Countdown 1 -> 2 -> 3 with buzzer beep ONLY here

void playCountdown(void) {

    for (int count = 1; count <= 3; count++) {

        displayBuf[0] = 0xFF;

        displayBuf[1] = segDigits[count];

        displayBuf[2] = 0xFF;

        displayBuf[3] = 0xFF;

        beep(70);

        waitWithDisplay(630);

    }

    buzzerOff();

    for (int i = 0; i < 4; i++) displayBuf[i] = 0xFF;

    waitWithDisplay(150);

}
 
int main(void) {

    SystemCoreClockUpdate();

    SysTick_Config(SystemCoreClock / 1000U);

    initHardware();

    buzzerOff();
 
    unsigned int seedCounter = 0;

    int highScore = 0;
 
    while (1) {

        // 1. Start Screen: "PLAY" + 4 Lives

        buzzerOff();

        displayBuf[0] = 0x8C; // P

        displayBuf[1] = 0xC7; // L

        displayBuf[2] = 0x88; // A

        displayBuf[3] = 0x91; // Y

        updateLives(4);
 
        int selectedMode = 0;

        while (selectedMode == 0) {

            refreshDisplay();

            seedCounter++;

            selectedMode = readDifficultySwitch();

        }
 
        // Wait until switch is released before starting

        while (readDifficultySwitch() != 0) {

            refreshDisplay();

            seedCounter++;

        }
 
        int numDigits = 2;

        int baseTimeMs = 3000;

        int minTimeMs  = 1600;

        int stepDropMs = 200;
 
        if (selectedMode == 1) {

            numDigits  = 2;    // S1: 2 digits

            baseTimeMs = 3000; // 3.0 seconds

            minTimeMs  = 1600;

            stepDropMs = 200;

        } else if (selectedMode == 2) {

            numDigits  = 3;    // S2: 3 digits

            baseTimeMs = 6000; // 6.0 seconds

            minTimeMs  = 3200;

            stepDropMs = 350;

        } else if (selectedMode == 3) {

            numDigits  = 4;    // S3: 4 digits

            baseTimeMs = 9000; // 9.0 seconds

            minTimeMs  = 4800;

            stepDropMs = 500;

        }
 
        srand(seedCounter);
 
        // 2. Countdown 1 - 2 - 3 (Buzzer sounds here only)

        playCountdown();
 
        // 3. Main Gameplay (Silent Buzzer)

        int lives = 4;

        int score = 0;

        int combo = 0;

        int target[4] = {0};
 
        while (lives > 0) {

            // Calculate current time limit (gets slightly faster every 3 points)

            int level = score / 3;

            int currentLimitMs = baseTimeMs - (level * stepDropMs);

            if (currentLimitMs < minTimeMs) {

                currentLimitMs = minTimeMs;

            }
 
            // Generate random digits for active slots

            for (int i = 0; i < 4; i++) {

                if (i < numDigits) {

                    target[i] = rand() % 10;

                    displayBuf[i] = segDigits[target[i]];

                } else {

                    displayBuf[i] = 0xFF;

                }

            }
 
            int currentIndex = 0;

            int elapsedMs = 0;

            bool roundSuccess = false;

            char lastKey = '\0';

            int releaseCount = 0;
 
            while (elapsedMs < currentLimitMs) {

                refreshDisplay();

                elapsedMs += 8;
 
                char k = scanKeypad();

                if (k != '\0') {

                    releaseCount = 0;

                    if (k != lastKey) {

                        lastKey = k;

                        if (k >= '0' && k <= '9') {

                            if ((k - '0') == target[currentIndex]) {

                                displayBuf[currentIndex] = 0xFF; // Whacked! Hide digit

                                currentIndex++;

                                if (currentIndex >= numDigits) {

                                    roundSuccess = true;

                                    break;

                                }

                            } else {

                                roundSuccess = false; // Wrong digit pressed

                                break;

                            }

                        }

                    }

                } else {

                    // Require 2 consecutive empty scans before allowing same key again

                    releaseCount++;

                    if (releaseCount >= 2) {

                        lastKey = '\0';

                    }

                }

            }
 
            if (roundSuccess) {

                score++;

                combo++;

                if (score > highScore) {

                    highScore = score;

                }

                // Bonus: 5 streak restores 1 life

                if (combo >= 5) {

                    combo = 0;

                    if (lives < 4) {

                        lives++;

                        updateLives(lives);

                    }

                }

            } else {

                combo = 0;

                lives--;

                updateLives(lives);

            }
 
            // Short pause between rounds

            for (int i = 0; i < 4; i++) displayBuf[i] = 0xFF;

            waitWithDisplay(250);

        }
 
        // 4. Game Over: Alternate between Score ("SCxx") and High Score ("HIxx")

        int toggleTimer = 0;

        bool showHigh = false;
 
        while (readDifficultySwitch() == 0) {

            if (!showHigh) {

                displayBuf[0] = 0x92; // S

                displayBuf[1] = 0xC6; // C

                displayBuf[2] = segDigits[(score / 10) % 10];

                displayBuf[3] = segDigits[score % 10];

            } else {

                displayBuf[0] = 0x89; // H

                displayBuf[1] = 0xF9; // I

                displayBuf[2] = segDigits[(highScore / 10) % 10];

                displayBuf[3] = segDigits[highScore % 10];

            }
 
            refreshDisplay();

            toggleTimer += 8;

            if (toggleTimer >= 1200) {

                toggleTimer = 0;

                showHigh = !showHigh;

            }

        }
 
        // Wait until switch released before returning to PLAY

        while (readDifficultySwitch() != 0) {

            refreshDisplay();

        }

    }

}
 
