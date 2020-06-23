//#define TESTING
#define F_CPU 1000000
#include "USI_TWI_Master.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>


enum direction {NONE, HRZ, VRT, BOTH};

enum gameState {IDLE, PLAYING, GAME_OVER, DEMO, TEST};
	
enum gameOutcome {WON, LOST};

void initializeTestDisplay();
void clearScreen();
void drawBall(uint8_t x, uint8_t y, uint8_t paddleX);
void drawBricks();
enum direction checkCollision(uint8_t x, uint8_t y, uint8_t ballDir);
void removeBrick(uint8_t column, uint8_t row);
enum direction checkPaddleHit(uint8_t ballX, uint8_t paddleX, uint8_t ballDir);
const uint8_t ballSprite[] PROGMEM = {0x00, 0x00, 0x18, 0x2C, 0x34, 0x18, 0x00, 0x00}; 

//Array representing what column/row the edges of the ball are in.
//Global so it can be preserved so we can check the previous position
uint8_t edgePositions [4] = {0}; //left column, right column, top row, bottom row

uint8_t brickStatus[4][5] = {{3,3,3,3,3},{3,3,3,3,3},{3,3,3,3,3},{3,3,3,3,3}}; //for harder to break bricks (implement later)
	
uint8_t numBricks;

//Stores initial brick pattern
const uint8_t brickSprites[2][64] PROGMEM = {
										{0x00, 
										0x00, 0x00, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
										0x00, 0x00, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
										0x00, 0x00, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
										0x00, 0x00, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
										0x00, 0x00, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 
										0x00, 0x00, 0x00},
							
										{0x00, 
										0x00, 0x00, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
										0x00, 0x00, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
										0x00, 0x00, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
										0x00, 0x00, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
										0x00, 0x00, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 
										0x00, 0x00, 0x00}
							
									   };
									
const uint8_t winScreen[64] PROGMEM = { 0x00, 0x04, 0x0A, 0x04, 0x00, 0x50, 0x20, 0x52, 0x07, 0x02, 0x00, 0x00,
										0x70, 0x08, 0x0F, 0x08, 0x70, 0x00, 0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00,
										0x7E, 0x01, 0x01, 0x01, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x07, 
										0x38, 0x07, 0x78, 0x00, 0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00, 0x7F, 0x60, 
										0x1C, 0x03, 0x7F, 0x00, 0x7D, 0x00, 0x00, 0x00, 0x00, 0x20, 0x70, 0x20,
										0x00, 0x04, 0x0A, 0x04};
										
const uint8_t loseScreen[64] PROGMEM = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x08, 0x0F,
										 0x08, 0x70, 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E, 0x00, 0x1E, 0x01, 0x01,
										 0x01, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x7F, 0x00, 0x00,
										 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E, 0x00, 0x09, 0x15, 0x15, 0x15, 0x12,
										 0x00, 0x7E, 0x11, 0x11, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
										 0x00, 0x00, 0x00, 0x00};
							

uint8_t gameField[2][64]; //Stores current brick pattern 

#define DISPLAY_SIZE_X 64
#define DISPLAY_SIZE_Y 48

#define BALL_HEIGHT 8 
#define BALL_WIDTH 8

int main (void){ 
	
	USI_TWI_Master_Initialise();
	initializeTestDisplay();
	
	enum gameState state;
	enum gameOutcome outcome;
	
	state = IDLE;
	
	#ifdef TESTING
	state = TEST;
	#endif
	
	uint8_t ballPosX = 0;
	uint8_t ballPosY = 0;
	uint8_t paddlePos = 0; 
	
	uint8_t increasing = 1;
	uint8_t increasing2 = 1;
	
	uint8_t demo_mode = 0;
	uint8_t demo_dir  = 0;
	int8_t ballTarget = 0;

	for(;;){	
	
		switch(state){
			
			case IDLE: 
				
				ballPosX = 31;
				ballPosY = 10;
				paddlePos = 30;
				increasing = 1;
				increasing2 = 1;
				numBricks = 20;
				uint8_t i,j;
				uint8_t demo_count = 0;
				
				//Populate brick sprites from program memory
				for (j=0;j<2;j++){
					for(i=0;i<64;i++){
						gameField[j][i] = pgm_read_byte(&brickSprites[j][i]);
					}
				}
				
				//Reset brick collision/status constructs
				for (j=0;j<4;j++){
					for(i=0;i<5;i++){
						brickStatus[j][i] = 3;
					}
					edgePositions[j] = 0;
				}
				
				//Re-draw screen
				clearScreen();
				drawBall(ballPosX, ballPosY, paddlePos);
				drawBricks();
				drawPaddle(paddlePos);
					
				//Wait for user input to start game. If ~10s passes, start demo mode.		
				ADMUX |= (0x03)|(1 << ADLAR); 
				ADCSRA |= (1 << ADEN); //Enable adc
				TCCR1 |= 0x0F; //Prescale to 1/16384
				TCNT1 = 0x00;
				OCR1A = 0xFF;
				_delay_us(10);
				for(;;){
					ADCSRA |= (1 << ADSC);
					while(ADCSRA & (1 <<ADSC));

					if(ADCH > 20 && ADCH < 40) //Left on joystick
					{
						increasing2 = 1;
						state = PLAYING;
						break;
					}
					else if(ADCH > 155 && ADCH < 175) //Right on joystick
					{
						increasing2 = 0;
						state = PLAYING;
						break;
					}
					else if(TIFR & 0x40){
						
						TIFR |= 0x40;
						TCNT1 = 0x00;
						demo_count++;
						if (demo_count == 2){ //Timer expired (go to demo mode)
							demo_mode = 1;
							increasing2 = demo_dir;
							demo_dir ^= 0x01;
							state = PLAYING;
							break;
						}
					}
					_delay_ms(20);
					
				}
				
				
				break;
				
			case PLAYING:
			
				TCCR0A |= 0x02; //CTC mode;
				TCCR0B |= 0x05; //prescale 1024
				OCR0A = 98; //approx 100ms period
				uint8_t roofCollision = 0;
					
				//Game loop (timed event loop, time slice = ~100ms)
				while(1){
					
					//Wait for tick
					while(!(TIFR & (1<<OCF0A)));
					TIFR |= (1<<OCF0A);
					
					//Move the ball
					if(increasing){
						ballPosY++;
					}
					else{
						ballPosY--;
					}
					if(increasing2){
						ballPosX++;
					}
					else{
						ballPosX--;
					}
					
					//Check for boundary collisions
					if(ballPosY >= DISPLAY_SIZE_Y - BALL_HEIGHT+2){
						increasing = 0;
						roofCollision = 1;
					}
					if(ballPosY == 0){
						outcome = LOST;
						state = GAME_OVER;
						break;
					}
					if(ballPosX >= DISPLAY_SIZE_X - BALL_WIDTH+2){
						increasing2 = 0;
					}
					if(ballPosX == 0){
						increasing2 = 1;
					}
					
					//Check for user input + move paddle
					if (demo_mode == 0){
					
						ADCSRA |= (1 << ADSC);
						while(ADCSRA & (1 <<ADSC));
					
						if(ADCH > 20 && ADCH < 40) //left on joystick
						{
							if (paddlePos < 54){
								paddlePos++;
							}
							drawPaddle(paddlePos);
						}
						else if(ADCH > 155 && ADCH < 175) //right on joystick
						{
							if (paddlePos > 0){
								paddlePos--;
							}
							drawPaddle(paddlePos);
						}
					}
					else{ //Demo mode, move towards target
						
						int8_t paddleDiff = (int8_t)paddlePos - ballTarget;
						if (paddleDiff > 0){
							if (paddlePos > 0){
								paddlePos--;
							}
							drawPaddle(paddlePos);
						}
						else if (paddleDiff < 0){
							if (paddlePos < 54){
								paddlePos++;
							}
							drawPaddle(paddlePos);
						}
						
					}
					
					//Check for brick collisions if ball is high enough
					if(ballPosY >= 32 - BALL_HEIGHT){
						enum direction newCollisions;
						newCollisions = checkCollision(ballPosX, ballPosY, (increasing | (increasing2 << 1)));
						int8_t ballDelta = 0;
						if(newCollisions == HRZ){
							increasing2 ^= 0x01;
						}
						else if(newCollisions == VRT){
							increasing ^= 0x01;
						}
						else if(newCollisions == BOTH){
							increasing ^= 0x01;
							increasing2 ^= 0x01;
						}
						
						if (demo_mode == 1){ //re-calculate target for demo mode
							if (newCollisions != NONE || roofCollision == 1){
								roofCollision = 0;
								ballDelta = (ballPosY-6);
								ballDelta *= increasing2?1:-1;
								ballTarget = ballDelta + (int8_t)ballPosX;
								if (ballTarget > (DISPLAY_SIZE_X - BALL_WIDTH+2)){
									ballTarget = (2*(DISPLAY_SIZE_X -BALL_WIDTH+2)) - ballTarget;
								}
								else if (ballTarget < 0){
									ballTarget = ballTarget * (-1);
								}
							}
						}
					
						if (numBricks == 0){
							outcome = WON;
							state = GAME_OVER;
							break;
						}
						
					}
					else if(ballPosY <= 6){ //check paddle collisions if ball is low enough
						
						enum direction newCollisions;
						newCollisions = checkPaddleHit(ballPosX, paddlePos, increasing2);
						if(newCollisions == VRT){
							increasing ^= 0x01;
						}
						else if(newCollisions == BOTH){
							increasing ^= 0x01;
							increasing2 ^= 0x01;
						}
						if(newCollisions != NONE){
							ballTarget = 28;
						}
						
					}
					else{ //Reset edge positions
						edgePositions[0] = 0;
						edgePositions[1] = 0;
						edgePositions[2] = 0;
						edgePositions[3] = 0;
					}
					
					//redraw ball
					drawBall(ballPosX, ballPosY, paddlePos);	
						
				}
			
				break;
				
			case GAME_OVER:
				clearScreen();
				uint8_t USI_Buf[66] = {0};
				USI_Buf[0] = (0x3D<<1);
				USI_Buf[1] = 0x01;
				
				//Set starting & ending column
				USI_Buf[2] = 0x21;
				USI_Buf[3] = 0x20; //start at 32
				USI_Buf[4] = 0x5F; //Stop at 95
				USI_TWI_Start_Read_Write(USI_Buf, 5);
				
				//Set starting & ending page
				USI_Buf[2] = 0x22;
				USI_Buf[3] = 0x03; //Start at 3
				USI_Buf[4] = 0x03; //End at 3
				USI_TWI_Start_Read_Write(USI_Buf, 5);
				
				USI_Buf[1] = 0x40;
						
				for (i = 0; i < 64; i++){
					
					if (outcome == WON){
						USI_Buf[i+2] = pgm_read_byte(&winScreen[63-i]);
					}
					else{
						USI_Buf[i+2] = pgm_read_byte(&loseScreen[63-i]);
					}
				}				
				
				USI_TWI_Start_Read_Write(USI_Buf, 66);
				
				USI_Buf[1] = 0x01;
				for (i = 0; i < 6; i++){ //Flash the screen for fun
					if (outcome == WON){
						USI_Buf[2] = 0xA7;
						USI_TWI_Start_Read_Write(USI_Buf, 3);
						_delay_ms(150);
						USI_Buf[2] = 0xA6;
						USI_TWI_Start_Read_Write(USI_Buf, 3);
						_delay_ms(150);
					}
				}
				
				
				_delay_ms(5000);
				state = IDLE;
				
				break;
				
			case TEST: 
			
				print8BitNum(10);

							
				break;
			default:
				state = IDLE;
				break;
		}
	
	}
		
	return 0;
}

//This function is currently hard coded to the number/position/shape of the bricks as well as the current ball sprite
//~45ms worst case (3x removeBrick, other code negligible) 
enum direction checkCollision(uint8_t x, uint8_t y, uint8_t ballDir){
	
	//limits of the VISIBLE part of the sprite (hard-coded to sprite).
	uint8_t spriteTop = y+5;
	uint8_t spriteLeft = x+5;
	uint8_t spriteRight = x+2;
	uint8_t spriteBottom = y+2;
	
	uint8_t numCollisions = 0;
	int8_t i,j;
	uint8_t column = 0;
	uint8_t row = 0;
	
	uint8_t prevEdgePositions[4];//left column, right column, top row, bottom row
	//Reset edge positions
	for(i = 0; i < 4; i++){
		prevEdgePositions[i] = edgePositions[i];
		edgePositions[i] = 0;
	}
	
	enum direction collisionDir;
	collisionDir = NONE;
	
	//Find out which "brick space" each edge of the ball is in
	
	//Vertical
	if(spriteTop < 35){
		edgePositions[2] = 1;
		edgePositions[3] = 1; //not really true, but convenient
	}
	else if(spriteTop < 39){
		edgePositions[2] = 2;
		edgePositions[3] = 1;
	}
	else if(spriteTop < 43){
		edgePositions[2] = 3;
		edgePositions[3] = 2;		
	}
	else if(spriteTop < 47){
		edgePositions[2] = 4;
		edgePositions[3] = 3;		
	}
	else if(spriteTop == 47){
		edgePositions[2] = 4; //kind of true
		edgePositions[3] = 4;
	}
	
	//Horizontal
	if(spriteLeft < 14){
		edgePositions[0] = 1;
		edgePositions[1] = 1;
	}
	else if (spriteLeft < 26){
		edgePositions[0] = 2;
		if(spriteRight < 14){
			edgePositions[1] = 1;
		}
		else{
			edgePositions[1] = 2;
		}
	}
	else if (spriteLeft < 38){
		edgePositions[0] = 3;
		if(spriteRight < 26){
			edgePositions[1] = 2;
		}
		else{
			edgePositions[1] = 3;
		}
	}
	else if (spriteLeft < 50){
		edgePositions[0] = 4;
		if(spriteRight < 38){
			edgePositions[1] = 3;
		}
		else{
			edgePositions[1] = 4;
		}
	}
	else if (spriteLeft < 62){
		edgePositions[0] = 5;
		if(spriteRight < 50){
			edgePositions[1] = 4;
		}
		else{
			edgePositions[1] = 5;
		}
	}
	else if (spriteLeft >= 62){
		edgePositions[0] = 5;
		edgePositions[1] = 5;
	}
		
	//Iterate through the brick positions that the ball is in and remove those bricks (if they exist)
	for (j = 1; j >= 0; j--){
		column = edgePositions[j];
		for (i = 3; i >= 2; i--){		
			row = edgePositions[i];
			if(column > 0 && row > 0){
				if(brickStatus[row-1][column-1] == 3){
					brickStatus[row-1][column-1] = 0; 
					removeBrick(column,row);
					numCollisions++;
				}
			}
		}
	}
	
	//Figure out collisions direction
	j = !((ballDir & 0x02) == 0x02);
	i = 2 + !((ballDir & 0x01) == 0x01);
	if(edgePositions[i] != prevEdgePositions[i] && numCollisions > 0){ 
		collisionDir = VRT;		
	}
	if(edgePositions[j] != prevEdgePositions[j] && collisionDir == NONE && numCollisions > 0){
		collisionDir = HRZ;		
	}
	else if(edgePositions[j] != prevEdgePositions[j] && collisionDir == VRT && numCollisions > 1){
		collisionDir = BOTH;
	}
	
	if (numBricks > numCollisions){
		numBricks = numBricks - numCollisions;
	}
	else{
		numBricks = 0;
	}
	
	return(collisionDir);
}

//Checks if the ball has hit the paddle, and returns "vrt" or "both" depending if the ball changes direction horizontally.
//This function should only be called if the ball is within Y range of the paddle. ( ballY <= 6 I think should be a hit)
//requires ball X position, Paddle X position and ball horizontal direction
enum direction checkPaddleHit(uint8_t ballX, uint8_t paddleX, uint8_t ballDir){
	
	//Calculations are hard coded to my current sprite as I probably will not change it. 
	uint8_t leftRightHit = 2; //1 for left, 0 for right
	uint8_t spriteRight = ballX+2;

	enum direction collisionDir;
	collisionDir = NONE;
	
	if((spriteRight >= (paddleX-4)) && (spriteRight <= paddleX + 1)){ //right side hit
		leftRightHit = 0;
	}
	else if(spriteRight == (paddleX + 2)){
		leftRightHit = ballDir;
	}
	else if((spriteRight >= (paddleX+3)) && (spriteRight <= paddleX + 10)) {//left hit
		leftRightHit = 1;
	}
	else{
		return collisionDir; //Ball did not hit (not within x range)
	}
	
	if(ballDir ^ leftRightHit){
		collisionDir = BOTH;
	}
	else
	{
		collisionDir = VRT;
	}
	return collisionDir;
}

//Removes the brick in column,row. Removed from "gameField" and updates screen
//Bytes written: 22 worst case
//Time estimate (transmission only): 10.532 (~15ms with code to be safe)
void removeBrick(uint8_t column, uint8_t row){
	uint8_t page;
	uint8_t startColumn = 3 + ((column-1)*12);
	uint8_t USI_Buf[12] = {0};
	uint8_t i;
	uint8_t r1;
	uint8_t r2;
	
	//Determine the position of the brick in the context of the screen (and set the limits for drawing)
	USI_Buf[0] = (0x3D<<1);
	USI_Buf[1] = 0x01;
	if(row < 3){
		page = 4;
		r1 = 1;
		r2 = 2;
	}
	else{
		page = 5;
		r1 = 3;
		r2 = 4;
	}
	
	USI_Buf[2] = 0x21;
	USI_Buf[3] = 0x20 + startColumn;
	USI_Buf[4] = 0x20 + startColumn + 10;
	USI_TWI_Start_Read_Write(USI_Buf, 5);
	
	//Set starting & ending page for bricks
	USI_Buf[2] = 0x22;
	USI_Buf[3] = page; 
	USI_Buf[4] = page; 
	USI_TWI_Start_Read_Write(USI_Buf, 5);
		
	//Update screen, update gameField
	USI_Buf[1] = 0x40;
	
	for(i = 0; i < 10; i++){		
		
		if(brickStatus[r1-1][column-1] == 3){
			USI_Buf[i+2] = 0x07;
		}
		else{
			USI_Buf[i+2] = 0x00;
		}
		
		if(brickStatus[r2-1][column-1] == 3){
			USI_Buf[i+2] |= 0x70;
		}//megan was here 
		
		gameField[page-4][startColumn+i] = USI_Buf[i+2];
		
	}
	USI_TWI_Start_Read_Write(USI_Buf,12);
	
}

//Sort of slow, so only used on startup currently. Will be useful for starting a new game. 
void drawBricks(){
	
		
	uint8_t USI_Buf[130] = {0};
	USI_Buf[0] = (0x3D<<1);
	USI_Buf[1] = 0x01;
	//Set starting & ending column for bricks
	USI_Buf[2] = 0x21;
	USI_Buf[3] = 0x20; //start at 32
	USI_Buf[4] = 0x5F; //Stop at 95
	USI_TWI_Start_Read_Write(USI_Buf, 5);
	
	//Set starting & ending page for bricks
	USI_Buf[2] = 0x22;
	USI_Buf[3] = 0x04; //Start at 4
	USI_Buf[4] = 0x05; //End at 5
	USI_TWI_Start_Read_Write(USI_Buf, 5);
	
	USI_Buf[1] = 0x40;
	uint8_t i = 0;
	uint8_t j = 0;
	uint8_t bufIndex = 0;

	
	for(i=0; i<2; i++){
		for(j=0; j<64;j++){		
			USI_Buf[bufIndex+2] = gameField[i][j];
			bufIndex++;
		}
	}
	USI_TWI_Start_Read_Write(USI_Buf,130);
	
}

//Re-Draws the ball at x,y. Ball sprite has black border (to erase previously drawn ball) so this function also re-draws bricks that would have been erased by this.
//The bottom corner of the ball sprite is the position reference pixel. (Aka it is in the sprite, not outside)
//Right now requires paddle position to re-draw paddle... Maybe turn ball position, paddle positions into global variables later
//Bytes written: 28 worst case
//Time estimate (transmission only): 13.286 ms (~17ms with code to be safe)
void drawBall(uint8_t x, uint8_t y, uint8_t paddleX){
	
	uint8_t USI_Buf[18] = {0};
	USI_Buf[0] = (0x3D<<1);
	USI_Buf[1] = 0x01;
	
	//Stores where the ball is vertically (which pages?)
	uint8_t page = (uint8_t)(y/8);
	uint8_t pagePixel = y%8;
	uint8_t i,j;
	
	//Set starting & ending column for drawing ball
	USI_Buf[2] = 0x21;
	USI_Buf[3] = 0x20 + x;
	USI_Buf[4] = 0x27 + x; 
	USI_TWI_Start_Read_Write(USI_Buf, 5);
	
	//select page the ball is on
	USI_Buf[2] = 0x22;
	USI_Buf[3] = page; 
	if(pagePixel){ //This determines if the ball is on 1 or 2 pages
		USI_Buf[4] = page + 1; 
		j = 16;
	}
	else{
		USI_Buf[4] = page;
		j = 8;
	}
	USI_TWI_Start_Read_Write(USI_Buf, 5);
	
	//Draw the ball
	USI_Buf[1] = 0x40;
	for(i = 0; i < j; i++){
		
		if(i<8){ 
			USI_Buf[i+2] = (pgm_read_byte(&ballSprite[i]))<<(pagePixel); //Draw the ball in buffer
			if(page >= 4){
				USI_Buf[i+2] |= gameField[page-4][i+x]; //OR in surrounding bricks
			}
			else if(page == 0){
				if((x+i) >= paddleX && (x+i) <= (paddleX + 9)){
					USI_Buf[i+2] |= 0x70; //OR in paddle
				}
			}
		}
		else{ //Only happens when ball is on 2 pages
			USI_Buf[i+2] = (pgm_read_byte(&ballSprite[i-8]))>>(8-pagePixel);
			if(page >= 3) {
				USI_Buf[i+2] |= gameField[page-3][i-8+x];
			}
		}
		
		
		
	}
	USI_TWI_Start_Read_Write(USI_Buf, pagePixel?18:10); //8 extra bytes if the ball is on 2 pages
	
}


//Bytes written: 24 worst case
//(~ 15ms with code to be safe)
void drawPaddle(uint8_t x){
	
		uint8_t USI_Buf[18] = {0};
		uint8_t i = 0;
		USI_Buf[0] = (0x3D<<1);
		USI_Buf[1] = 0x01;
		//Set starting & ending column
		USI_Buf[2] = 0x21;
		USI_Buf[3] = 0x1F + x;
		USI_Buf[4] = 0x2A + x; 
		USI_TWI_Start_Read_Write(USI_Buf, 5);
		//select page
		USI_Buf[2] = 0x22;
		USI_Buf[3] = 0; 
		USI_Buf[4] = 0; 
		USI_TWI_Start_Read_Write(USI_Buf, 5);
		
		USI_Buf[1] = 0x40;
		USI_Buf[2] = 0x00;
		USI_Buf[13] = 0x00;
		for(i; i < 10; i++){
			USI_Buf[3+i] = 0x70;
		}
		USI_TWI_Start_Read_Write(USI_Buf, 14);
}

//Initialize function for display
void initializeTestDisplay(){
	
	uint8_t USI_Buf[80] = {0}; 
	
	DDRB |= (1 << PB1);
	PORTB &= ~(1<<PB1); //Pull display reset low for 10us
	_delay_us(10);
	PORTB |= (1 << PB1); //back to high	
	
	USI_Buf[0] = (0x3D<<1)|0;
	USI_Buf[1] = 0x01;
	
	//Turn on charge pump regulator
	USI_Buf[2] = 0x8D;
	USI_Buf[3] = 0x14;
	USI_TWI_Start_Read_Write(USI_Buf, 4);
		
	//turn on display
	USI_Buf[2] = 0xAF;
	USI_TWI_Start_Read_Write(USI_Buf, 3);
		
	//Set horizontal page addressing mode
	USI_Buf[2] = 0x20;
	USI_Buf[3] = 0x00;
	USI_TWI_Start_Read_Write(USI_Buf, 4);	
	
	clearScreen();
	
	USI_Buf[1] = 0x01;
	
	//Set starting & ending column for small display (64x48)
	USI_Buf[2] = 0x21;
	USI_Buf[3] = 0x20; //start at 32
	USI_Buf[4] = 0x5F; //Stop at 95
	USI_TWI_Start_Read_Write(USI_Buf, 5);
	
	//Set starting & ending page for small display (64x48)
	USI_Buf[2] = 0x22;
	USI_Buf[3] = 0x00; //Start at 0
	USI_Buf[4] = 0x05; //End at 5
	USI_TWI_Start_Read_Write(USI_Buf, 5);
	
}

//Clears the screen
void clearScreen(){
	
	uint8_t USI_Buf[80] = {0}; 
		
	USI_Buf[0] = (0x3D<<1)|0;
	USI_Buf[1] = 0x01;
	
	//Set starting & ending column for small display (64x48)
	USI_Buf[2] = 0x21;
	USI_Buf[3] = 0x20; //start at 32
	USI_Buf[4] = 0x5f; //Stop at 95
	USI_TWI_Start_Read_Write(USI_Buf, 5);
	
	//Set starting & ending page for small display (64x48)
	USI_Buf[2] = 0x22;
	USI_Buf[3] = 0x00; //Start at 0
	USI_Buf[4] = 0x05; //End at 5
	USI_TWI_Start_Read_Write(USI_Buf, 5);
	
	uint8_t i = 0;
	uint8_t j = 0;
	USI_Buf[1] = 0x40;
	
	//Clear
	for (j = 0; j < 6; j++){
		for (i = 0; i<64;i++){
			USI_Buf[i+2] = 0x00;		
		}
		USI_TWI_Start_Read_Write(USI_Buf, 66);
		i = 0;	
	}	
}

#ifdef TESTING
void print8BitNum(uint8_t num){
	
		uint8_t USI_Buf[18] = {0};
		uint8_t i = 0;
		uint8_t j = 1;
		USI_Buf[0] = (0x3D<<1);
		USI_Buf[1] = 0x01;
		//Set starting & ending column
		USI_Buf[2] = 0x21;
		USI_Buf[3] = 0x31;
		USI_Buf[4] = 0x39; 
		USI_TWI_Start_Read_Write(USI_Buf, 5);
		//select page
		USI_Buf[2] = 0x22;
		USI_Buf[3] = 1; 
		USI_Buf[4] = 1; 
		USI_TWI_Start_Read_Write(USI_Buf, 5);
		
		USI_Buf[1] = 0x40;
		for(i; i < 8; i++){
			if(num & j){
				USI_Buf[2+i] = 0XFF;
			}
			else{
				USI_Buf[2+i] = 0X00;
			}
			j = j << 1;
		}
		USI_Buf[10] = 0xC3;
		USI_TWI_Start_Read_Write(USI_Buf, 11);
		
		
}
#endif





