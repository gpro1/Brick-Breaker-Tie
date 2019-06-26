//First "working" version! Bricks can be broken, but ball is too big and will erase bricks near to it. Creating another version to erase the previous ball before drawing the new one. 


#define F_CPU 1000000
#include "USI_TWI_Master.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

//enum direction {NONE, HRZ, VRT, BOTH};

void initializeTestDisplay(); //RE-WRITE FOR SMALLER DISPLAY
void drawBall(uint8_t x, uint8_t y);
void drawBricks();
void checkCollision(uint8_t x, uint8_t y);
void removeBrick(uint8_t column, uint8_t row);
uint8_t ballSprite[] PROGMEM = {0x00, 0x00, 0x18, 0x2C, 0x34, 0x18, 0x00, 0x00};

uint8_t brickStatus[4][5] = {{3,3,3,3,3},{3,3,3,3,3},{3,3,3,3,3},{3,3,3,3,3}};

uint8_t brickSprites[] PROGMEM = {0x00, 
							0x00, 0x00, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
							0x00, 0x00, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
							0x00, 0x00, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
							0x00, 0x00, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
							0x00, 0x00, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 
							0x00, 0x00, 0x00};
							

							
							
//uint8_t ballSprite[]= {0x00, 0x00, 0x18, 0x2C, 0x34, 0x18, 0x00, 0x00};

//uint8_t bricks[20] = {0};
#define DISPLAY_SIZE_X 64
#define DISPLAY_SIZE_Y 48

#define BALL_HEIGHT 8
#define BALL_WIDTH 8




int main (void){ 
	
	USI_TWI_Master_Initialise();
	initializeTestDisplay();
	
	uint8_t ballPosX = 0;
	uint8_t ballPosY = 0;
	
	uint8_t ball2PosX = 0;
	uint8_t ball2PosY = 13;
	
	uint8_t increasing = 1;
	uint8_t increasing2 = 1;
	
	drawBall(ballPosX, ballPosY);
	drawBricks();

	for(;;){	//Infinite loop	
	
	
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
		if(ballPosY >= DISPLAY_SIZE_Y - BALL_HEIGHT){
			increasing = 0;
		}
		if(ballPosY == 0){
			increasing = 1;
		}
		if(ballPosX >= DISPLAY_SIZE_X - BALL_WIDTH){
			increasing2 = 0;
		}
		if(ballPosX == 0){
			increasing2 = 1;
		}
		
		//Check for brick collisions if ball is high enough
		if(ballPosY >= 32 - BALL_HEIGHT){
			
			checkCollision(ballPosX, ballPosY);
			//NOTE FOR FUTURE GREG: The bottom corner of the ball sprite is the position reference pixel. (Aka it is in the sprite, not outside)
		}
		
		//drawBricks();
		//redraw ball
		drawBall(ballPosX, ballPosY);
		
		
		
		_delay_ms(16);
	
		
	
	}
		
	return 0;
}

//This function is currently hard coded to the number/position/shape of the bricks as well as the current ball sprite
//Maybe change to checkpotentialcollsiion? Sends back potential collisions and main can check ball direction?
void checkCollision(uint8_t x, uint8_t y){
	
	//These will store the limits of the VISIBLE part of the sprite.
	//Calculations are hard coded to my current sprite as I probably will not change it.
	uint8_t spriteTop = y+6;
	uint8_t spriteLeft = x+6;
	uint8_t spriteRight = x+2;
	uint8_t spriteBottom = y+2;
	//uint8_t ball_in_rows [2] = {0};
	//uint8_t ball_in_column[2] = {0};
	uint8_t edgePositions [4] = {0}; //left column, right column, top row, bottom row
	
	uint8_t i,j;
	
	//uint8_t left_in_column = 0;
	//uint8_t right_in_column = 0;
	//uint8_t top_in_row = 0;
	//uint8_t bottom_in_row = 0;
	
	
	
	
	//enum direction collisionDir = none;
	
	//Find out one "brick space" the ball is in
	//once this space is found, check ONLY the next one.
	//This must be done twice (horizontal and vertically
	//Once you find a space the brick is in, check if there is a block there, and add it as a collision (lets have an array?)
	//At the end, check all the collisions, erase those blocks, and calculate the direction to be returned so the main can change the ball direction. 
	
	//Vertical
	if(spriteTop < 35){
		//ball_in_rows[0] = 1;
		edgePositions[2] = 1;
		edgePositions[3] = 1; //not really true, but convenient
	}
	else if(spriteTop < 39){
		//ball_in_rows[0] = 2;
		//ball_in_rows[1] = 1;
		edgePositions[2] = 2;
		edgePositions[3] = 1;
	}
	else if(spriteTop < 43){
		//ball_in_rows[0] = 3;
		//ball_in_rows[1] = 2;
		edgePositions[2] = 3;
		edgePositions[3] = 2;		
	}
	else if(spriteTop < 47){
		//ball_in_rows[0] = 4;
		//ball_in_rows[1] = 3;	
		edgePositions[2] = 4;
		edgePositions[3] = 3;		
	}
	else if(spriteTop == 47){
		//ball_in_rows[0] = 4;
		edgePositions[2] = 4; //kind of true??
		edgePositions[3] = 4;
	}
	
	//Horizontal
	if(spriteLeft < 14){
		//ball_in_column[0] = 1;
		edgePositions[0] = 1;
		edgePositions[1] = 1;
	}
	else if (spriteLeft < 26){
		//ball_in_column[0] = 2;
		edgePositions[0] = 2;
		if(spriteRight < 14){
			//ball_in_column[1] = 1;
			edgePositions[1] = 1;
		}
		else{
			edgePositions[1] = 2;
		}
	}
	else if (spriteLeft < 38){
		//ball_in_column[0] = 3;
		edgePositions[0] = 3;
		if(spriteRight < 26){
			edgePositions[1] = 2;
			//ball_in_column[1] = 2;
		}
		else{
			edgePositions[1] = 3;
		}
	}
	else if (spriteLeft < 50){
		//ball_in_column[0] = 4;
		edgePositions[0] = 4;
		if(spriteRight < 38){
			//ball_in_column[1] = 3;
			edgePositions[1] = 3;
		}
		else{
			edgePositions[1] = 4;
		}
	}
	else if (spriteLeft < 62){
		//ball_in_column[0] = 5;
		edgePositions[0] = 5;
		if(spriteRight < 50){
			//ball_in_column[1] = 4;
			edgePositions[1] = 4;
		}
		else{
			edgePositions[1] = 5;
		}
	}
	else if (spriteLeft >= 62){
		//ball_in_column[0] = 5;
		edgePositions[0] = 5;
		edgePositions[1] = 5;
	}
	
	for (j = edgePositions[1]; j <= edgePositions[0]; j++){
		for (i = edgePositions[3]; i <= edgePositions[2]; i++){
			if(i>0 && j>0){
				if(brickStatus[i-1][j-1] == 3){
					brickStatus[i-1][j-1] = 0;
					removeBrick(j,i);
				}
			}
		}
	}
	
	//return(NONE);
	
}

void removeBrick(uint8_t column, uint8_t row){
	uint8_t page;
	uint8_t startColumn = 3 + ((column-1)*12);
	uint8_t USI_Buf[12] = {0};
	uint8_t i;
	uint8_t r1;
	uint8_t r2;
	
	
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
		
	}
	USI_TWI_Start_Read_Write(USI_Buf,12);
	
	
	
}

//I might not need this function but right now it's for testing.
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
	USI_Buf[3] = 0x04; //Start at 5
	USI_Buf[4] = 0x05; //End at 5
	USI_TWI_Start_Read_Write(USI_Buf, 5);
	
	USI_Buf[1] = 0x40;
	uint8_t i = 0;
	/*uint8_t brickCount = 1;
	for(i=0; i < 64; i++){
		if(i == 0 || i == 2 || i == 64 || i == 63){
			USI_Buf[i+2] = 0x00;
		}
		else if(brickCount == 1 || brickCount == 12){
			USI_Buf[i+2] = 0x00;
			if (brickCount == 12) brickCount = 0;
			brickCount++;
		}
		else{
			USI_Buf[i+2] = 0x77;
			brickCount++;
		}
		
	}
	USI_TWI_Start_Read_Write(USI_Buf,66);*/
	
	for(i=0; i<128; i++){
		USI_Buf[i+2] = pgm_read_byte(&brickSprites[((i<64)?i:i-64)]);
	}
	USI_TWI_Start_Read_Write(USI_Buf,130);
	
}

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
	if(pagePixel){
		USI_Buf[4] = page + 1; 
		j = 16;
	}
	else{
		USI_Buf[4] = page;
		j = 8;
	}
	USI_TWI_Start_Read_Write(USI_Buf, 5);
	
	USI_Buf[1] = 0x40;
	
	
	for(i = 0; i < j; i++){
		
		if(i<8){
			USI_Buf[i+2] = (pgm_read_byte(&ballSprite[i]))<<(pagePixel);
		}
		else{
			USI_Buf[i+2] = (pgm_read_byte(&ballSprite[i-8]))>>(8-pagePixel);
		}
		
	}
	USI_TWI_Start_Read_Write(USI_Buf, pagePixel?18:10);
	
}

void initializeTestDisplay(){
	
	uint8_t USI_Buf[80] = {0}; //Initialize buffer to all 0
	
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





