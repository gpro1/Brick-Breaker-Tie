
#define F_CPU 1000000
#include "USI_TWI_Master.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

enum direction {NONE, HRZ, VRT, BOTH};

enum gameState {IDLE, PLAYING, LOST, DEMO};

void initializeTestDisplay(); //RE-WRITE FOR SMALLER DISPLAY
void drawBall(uint8_t x, uint8_t y);
void drawBricks();
enum direction checkCollision(uint8_t x, uint8_t y);
void removeBrick(uint8_t column, uint8_t row);
uint8_t ballSprite[] PROGMEM = {0x00, 0x00, 0x18, 0x2C, 0x34, 0x18, 0x00, 0x00}; 

//A weird array representing what column/row the edges of the ball are in. Did it this way so I could use an array to check collisions instead of another huge if statement
//Global so it can be preserved so we can check the previous position
uint8_t edgePositions [4] = {0}; //left column, right column, top row, bottom row

uint8_t brickStatus[4][5] = {{3,3,3,3,3},{3,3,3,3,3},{3,3,3,3,3},{3,3,3,3,3}};

//Stores initial brick pattern
uint8_t brickSprites[2][64] PROGMEM = {
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
							

uint8_t gameField[2][64]; //Stores current brick pattern 

//uint8_t ballSprite[]= {0x00, 0x00, 0x18, 0x2C, 0x34, 0x18, 0x00, 0x00};

//uint8_t bricks[20] = {0};
#define DISPLAY_SIZE_X 64
#define DISPLAY_SIZE_Y 48

#define BALL_HEIGHT 8 //make smaller
#define BALL_WIDTH 8




int main (void){ 
	
	USI_TWI_Master_Initialise();
	initializeTestDisplay();
	
	enum gameState state;
	state = IDLE;
	
	uint8_t ballPosX = 0;
	uint8_t ballPosY = 0;
	
	//uint8_t ball2PosX = 0;
	//uint8_t ball2PosY = 13;
	
	uint8_t increasing = 1;
	uint8_t increasing2 = 1;

	for(;;){	//Infinite loop	
	
		switch(state){
			
			case IDLE: //Reset all the things. LATER: Wait for user input to begin.
				
				state = PLAYING;
				ballPosX = 0;
				ballPosY = 0;
				increasing = 1;
				increasing2 = 1;
				uint8_t i,j;
				for (j=0;j<2;j++){
					for(i=0;i<64;i++){
						gameField[j][i] = pgm_read_byte(&brickSprites[j][i]);
					}
				}
				for (j=0;j<4;j++){
					for(i=0;i<5;i++){
						brickStatus[j][i] = 3;
					}
					edgePositions[j] = 0;
				}
				drawBall(ballPosX, ballPosY);
				drawBricks();
				//CLEAR SCREEN??
				break;
				
			case PLAYING:
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
				}
				if(ballPosY == 0){
					//increasing = 1;
					state = LOST;
				}
				if(ballPosX >= DISPLAY_SIZE_X - BALL_WIDTH+2){
					increasing2 = 0;
				}
				if(ballPosX == 0){
					increasing2 = 1;
				}
				
				//Check for brick collisions if ball is high enough
				if(ballPosY >= 32 - BALL_HEIGHT){
					enum direction newCollisions;
					newCollisions = checkCollision(ballPosX, ballPosY);
					if(newCollisions == HRZ){
						increasing2 ^= 0x01;
					}
					else if(newCollisions == VRT){
						increasing ^= 0x01;
					}
					else if(newCollisions == BOTH){
						increasing ^= 0x01;
						increasing2 ^= 0x02;
					}
				}
				
				
				//redraw ball
				drawBall(ballPosX, ballPosY);
				
				_delay_ms(200);
				break;
				
			case LOST:
				state = IDLE;
				break;
			case DEMO: 
				state = IDLE;
				break;	
			default:
				state = IDLE;
				break;
		}
	
	}
		
	return 0;
}

//This function is currently hard coded to the number/position/shape of the bricks as well as the current ball sprite
//Maybe change to checkpotentialcollsiion? Sends back potential collisions and main can check ball direction?
enum direction checkCollision(uint8_t x, uint8_t y){
	
	//These will store the limits of the VISIBLE part of the sprite.
	//Calculations are hard coded to my current sprite as I probably will not change it. 
	uint8_t spriteTop = y+5;
	uint8_t spriteLeft = x+5;
	uint8_t spriteRight = x+2;
	uint8_t spriteBottom = y+2;
	uint8_t numCollisions = 0;
	uint8_t i,j;
	
	uint8_t prevEdgePositions[4];//left column, right column, top row, bottom row
	//Reset edge positions
	for(i = 0; i < 4; i++){
		prevEdgePositions[i] = edgePositions[i];
		edgePositions[i] = 0;
	}
	
	
		
	enum direction collisionDir;
	collisionDir = NONE;
	
	//Find out one "brick space" the ball is in
	//once this space is found, check ONLY the next one.
	//This must be done twice (horizontal and vertically
	//Once you find a space the brick is in, check if there is a block there, and add it as a collision (lets have an array?)
	//At the end, check all the collisions, erase those blocks, and calculate the direction to be returned so the main can change the ball direction. 
	
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
		edgePositions[2] = 4; //kind of true??
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
	for (j = edgePositions[1]; j <= edgePositions[0]; j++){
		for (i = edgePositions[3]; i <= edgePositions[2]; i++){
			if(i>0 && j>0){
				if(brickStatus[i-1][j-1] == 3){
					brickStatus[i-1][j-1] = 0; //maybe more appropriate inside the function?
					removeBrick(j,i);
					//numCollisions++;
					if(edgePositions[i] != prevEdgePositions[i] && collisionDir == NONE){
						collisionDir = VRT;		
					}
					else if(edgePositions[i] != prevEdgePositions[i] && collisionDir == HRZ){
						collisionDir = BOTH;
					}
					/*if(edgePositions[j] != prevEdgePositions[j] && collisionDir == NONE){
						collisionDir = HRZ;		
					}
					else if(edgePositions[i] != prevEdgePositions[i] && collisionDir == VRT){
						collisionDir = BOTH;
					}*/ //Maybe implement horizontal later? Issues with this implementation
				}
			}
		}
	
	}

	
	return(collisionDir);
	
}

//Removes the brick in column,row. Removed from "gameField" and updates screen
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
void drawBall(uint8_t x, uint8_t y){
	
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
	
	//DrAW THE BALL!!
	USI_Buf[1] = 0x40;
	for(i = 0; i < j; i++){
		
		if(i<8){ 
			USI_Buf[i+2] = (pgm_read_byte(&ballSprite[i]))<<(pagePixel); //Draw the ball in buffer
			if(page >= 4) USI_Buf[i+2] |= gameField[page-4][i+x]; //OR in surrounding bricks
		}
		else{ //Only happens when ball is on 2 pages
			USI_Buf[i+2] = (pgm_read_byte(&ballSprite[i-8]))>>(8-pagePixel);
			if(page >= 3) USI_Buf[i+2] |= gameField[page-3][i-8+x];
		}
		
		
		
	}
	USI_TWI_Start_Read_Write(USI_Buf, pagePixel?18:10); //8 extra bytes if the ball is on 2 pages
	
}

//Initialize function for the 128x64 display. Draws a boundary to represent the real display I'm going to use
void initializeTestDisplay(){
	
	uint8_t USI_Buf[80] = {0}; 
	
	DDRB |= (1 << PB1);
	PORTB &= ~(1<<PORTB1); //Pull display reset low for 10us
	_delay_us(10);
	PORTB |= (1 << PORTB1); //back to high	
	
	USI_Buf[0] = (0x3D<<1)|0;
	USI_Buf[1] = 0x01;
	
	//Turn on charge pump regulator
	USI_Buf[2] = 0x8D;;
	USI_Buf[3] = 0x14;
	USI_TWI_Start_Read_Write(USI_Buf, 4);
		
	//turn on display
	USI_Buf[2] = 0xAF;
	USI_TWI_Start_Read_Write(USI_Buf, 3);
		
	//Set horizontal page addressing mode
	USI_Buf[2] = 0x20;
	USI_Buf[3] = 0x00;
	USI_TWI_Start_Read_Write(USI_Buf, 4);	
	
	//Set starting & ending column for drawing border on big screen 
	USI_Buf[2] = 0x21;
	USI_Buf[3] = 0x1F; //start at 31
	USI_Buf[4] = 0x60; //Stop at 96
	USI_TWI_Start_Read_Write(USI_Buf, 5);
	
	//Set starting & ending page for drawing border on big screen
	USI_Buf[2] = 0x22;
	USI_Buf[3] = 0x00; //Start at 0
	USI_Buf[4] = 0x06; //End at 6
	USI_TWI_Start_Read_Write(USI_Buf, 5);
	
	uint8_t i = 0;
	uint8_t bufIndex = 0;
	uint8_t j = 0;
	USI_Buf[1] = 0x40;
	
	//Draw border
	for (j = 0; j < 7; j++){
		for (i = 0; i<66;i++){
	
			if(i == 0 || i == 65){
				USI_Buf[bufIndex+2] = 0xff;
			}
			else if (j==6){
				USI_Buf[bufIndex+2] = 0x01;
			}
			else{
				USI_Buf[bufIndex+2] = 0x00;
			}
			bufIndex++;
		}
		USI_TWI_Start_Read_Write(USI_Buf, 68);
		i = 0;
		bufIndex = 0;
	}
	
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





