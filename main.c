#include<stm32f10x.h>
#define DEBOUNCE_CYCLES 50

#define BEAT_ARR 18
#define DOWN_BEAT_ARR 14

// Seven segment codes with 'a' as the least significant bit
static uint32_t sevenSegmentCodes[] = {
	0x3f,
	0x06,
	0x5b,
	0x4f,
	0x66,
	0x6d,
	0x7d,
	0x07,
	0x7f,
	0x6f,
	0x77,
	0x7c,
	0x39,
	0x5e,
	0x79,
	0x71
};

#define NUMBER_OF_TEMPOS 10
//predefined tempos to toggle between
static int32_t tempos[] = {
	60,
	72,
	88,
	100,
	112,
	128,
	136,
	144,
	152,
	160
};

//tempo_number is the index of tempos that is current
static int tempo_number = 5;
//tempo is the current tempo and must be a member of tempos[]
static int tempo = 128;
//beats is the number of beats per measure and cur_beat is the current beat
static int beats = 0, cur_beat = 0;

//handler for timer 7 that alternates between digits to display in the 7-segment display
void TIM7_IRQHandler (void) {
		static int LEDnum;
		uint32_t ODR;
	
		//clear interrupt flag
		TIM7->SR &= ~TIM_SR_UIF;

		//increment LEDnum
		LEDnum = (LEDnum+1) % 4;
		//set all of pins 8-11 to Hi-Z except for the current LED which should sink
		ODR = 0x0F00 ^ (1<<(8+LEDnum));
		//display the proper digit
		if (LEDnum == 0) {
			ODR += sevenSegmentCodes[beats];
		}	else if(LEDnum == 3) {
			ODR += sevenSegmentCodes[tempo % 10];
		} else if (LEDnum == 2){
			ODR += sevenSegmentCodes[tempo/(10) % 10];
		} else {
			ODR += sevenSegmentCodes[tempo/(100) % 10];
		}
		GPIOA->ODR = ODR;
}

//handler for timer 2 that starts each beat
void TIM2_IRQHandler (void) {
	//clear interrupt flag
	TIM2->SR &= ~TIM_SR_UIF;
	
	//turn on the LED
	GPIOC->ODR |= GPIO_ODR_ODR9;
	
	//set the correct frequency for the buzzer
	if (beats == 0) {
		TIM3->ARR = BEAT_ARR;
		TIM3->CCR1 = BEAT_ARR/2;
	} else {
		if (cur_beat == 0) {
			TIM3->ARR = DOWN_BEAT_ARR;
			TIM3->CCR1 = DOWN_BEAT_ARR/2;
		} else {
			TIM3->ARR = BEAT_ARR;
			TIM3->CCR1 = BEAT_ARR/2;
		}
		//increment the current beat only if there are at least one beats per measure
		cur_beat = (cur_beat+1)%beats;
	}
	//start timer 3, which outputs a pwm wave for the buzzer
	TIM3->CR1 |= TIM_CR1_CEN;
	
	//set TIM4, which stops the beat
	TIM4->CR1 |= TIM_CR1_CEN;
}

//Timer 4 interrupt handler which stops each beat
void TIM4_IRQHandler (void) {
	//clear the interrupt flag
	TIM4->SR &= ~TIM_SR_UIF;

	//turn off TIM4
	TIM4->CR1 &= ~TIM_CR1_CEN;
	
	//turn off the buzzer
	TIM3->CR1 &= ~TIM_CR1_CEN;
	
	//turn off the LED
	GPIOC->ODR &= ~GPIO_ODR_ODR9;
}

int main() {
	
	char tempoButtonState = 1, beatsButtonState = 2;
	int count;
	
	//Enable clocking for GPIO A/B/C, TIM 7, TIM 4, TIM 3, TIM2, AFIO
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN |RCC_APB2ENR_IOPBEN|RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;
	RCC->APB1ENR |= RCC_APB1ENR_TIM7EN | RCC_APB1ENR_TIM3EN | RCC_APB1ENR_TIM4EN | RCC_APB1ENR_TIM2EN;
	
	//SET GPIO pins 8 and 9 to output
	GPIOC->CRH = 0x44444433;
	//Set GPIOA 0-7 to outputs
	GPIOA->CRL = 0x33333333;
	//Set GPIOA 8-11 to input open drain
	GPIOA->CRH = 0x44447777;
	//set GPIOC pin 6 to alt function push pull
	GPIOC->CRL = 0x4B444444;
	//remap Timer 3s outputs
	AFIO->MAPR = 0xC00;
	
	//Timer 7 settings
	TIM7->PSC = 2399; //effecive frequency is now 10,000hz
	TIM7->ARR = 10; 
	TIM7->DIER |= TIM_DIER_UIE; //set interrupts
	TIM7->CR1 |= TIM_CR1_CEN; //start counting
	
	//Timer 3 settings
	TIM3->PSC = 2399; //effecive frequency is now 10,000hz
	TIM3->ARR = BEAT_ARR; 
	TIM3->CCMR1 = 0x68; //set output compare mode to PWM mode on channel 1
	TIM3->CCR1 = BEAT_ARR/2;
	TIM3->CCER = 1; // enable output channel 1
	TIM3->CR1 |= TIM_CR1_ARPE; 
	TIM3->EGR |= TIM_EGR_UG; //update event so shadow registers are filled

	//Timer 2 settings
	TIM2->PSC = 2399; //effecive frequency is now 10,000hz
	TIM2->ARR = (uint16_t)((60.0/tempo)*10000); 
	TIM2->DIER |= TIM_DIER_UIE; //
	TIM2->CR1 |= TIM_CR1_CEN;
	
	//Timer 4 settings
	TIM4->PSC = 2399;
	TIM4->ARR = 1000; //lets try this for a .1 second buzz
	TIM4->DIER |= TIM_DIER_UIE;
	
	//unmask timer 7,4,2 interrupt
	NVIC_EnableIRQ(TIM7_IRQn);
	NVIC_EnableIRQ(TIM2_IRQn);
	NVIC_EnableIRQ(TIM4_IRQn);
	
	//MAIN loop
	while(1){
		count = 0;
		//debouncing loop for tempo input
		while ((GPIOB->IDR & 1) != tempoButtonState) {
			count++;
			if (count > DEBOUNCE_CYCLES) {
				//update button state
				tempoButtonState = (GPIOB->IDR & 1);
				if (!tempoButtonState) {
					//advance to the next tempo
					tempo_number = (tempo_number + 1) % NUMBER_OF_TEMPOS;
					tempo = tempos[tempo_number];
					//update TIM2 with the new tempo
					TIM2->ARR = (uint16_t)((60.0/tempo)*10000); 
				}
			}
		}	
		count = 0;
		//debouncing loop for beat input
		while ((GPIOB->IDR & 2) != beatsButtonState) {
			count++;
			if (count > DEBOUNCE_CYCLES) {
				beatsButtonState = (GPIOB->IDR & 2);
				if (!beatsButtonState) {
					//advance beat and reset current beat to prevent glitches
					beats = (beats + 1) % 10;
					cur_beat = 0;
				}
			}
		}	
	}
}
