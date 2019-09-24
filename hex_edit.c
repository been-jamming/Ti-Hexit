#define USE_TI89
#define SAVE_SCREEN

#include <tigcclib.h>

unsigned char editing;
unsigned char memory_violation;
char virtual[LCD_SIZE];

char get_high_char(unsigned char byte){
	if((byte>>4) <= 9){
		return '0' + (byte>>4);
	} else {
		return 'A' + (byte>>4) - 10;
	}
}

char get_low_char(unsigned char byte){
	if((byte&0x0F) <= 9){
		return '0' + (byte&0x0F);
	} else {
		return 'A' + (byte&0x0F) - 10;
	}
}

void display_byte(unsigned char byte, int x, int y, int attr){
	char high_char;
	char low_char;

	high_char = get_high_char(byte);
	low_char = get_low_char(byte);

	DrawChar(x, y, high_char, attr);
	DrawChar(x + 6, y, low_char, attr);
}

int is_hex_digit(char c){
	if(c >= '0' && c <= '9'){
		return c - '0';
	} else if(c >= 'A' && c <= 'F'){
		return c - 'A' + 10;
	} else if(c >= 'a' && c <= 'f'){
		return c - 'a' + 10;
	}

	return -1;
}

void set_status(const char *status){
	FontSetSys(F_4x6);
	DrawStr(0, 92, status, A_NORMAL);
	FontSetSys(F_6x8);
}

unsigned long int enter_address(unsigned long int previous_address){
	unsigned long int current_address = 0;
	unsigned char count = 0;
	int key;
	int digit;

	DrawStr(0, 0, "0x        ", A_REVERSE);
	LCD_restore(virtual);
	while(count < 8){
		key = ngetchx();
		if((digit = is_hex_digit(key)) != -1){
			current_address = current_address*0x10 + digit;
			DrawChar(6*count + 12, 0, key, A_REVERSE);
			LCD_restore(virtual);
			count++;
		} else if(key == KEY_BACKSPACE && count){
			count--;
			DrawChar(6*count + 12, 0, ' ', A_REVERSE);
			LCD_restore(virtual);
		} else if(key == KEY_ESC){
			return previous_address;
		}
	}

	return current_address;
}

unsigned char enter_byte(unsigned char *output){
	int digit1;
	int digit2;
	int key;

	key = ngetchx();
	if((digit1 = is_hex_digit(key)) == -1){
		return 1;
	}
	key = ngetchx();
	if((digit2 = is_hex_digit(key)) == -1){
		return 1;
	}
	*output = (digit1<<8) | digit2;

	return 0;
}

void update_display(unsigned long int starting_address, unsigned long int current_address){
	unsigned int i;
	unsigned int j;

	ClrScr();
	DrawChar(0, 0, '0', A_NORMAL);
	DrawChar(6, 0, 'x', A_NORMAL);
	display_byte(starting_address>>24, 12, 0, A_NORMAL);
	display_byte((starting_address>>16)&0xFF, 24, 0, A_NORMAL);
	display_byte((starting_address>>8)&0xFF, 36, 0, A_NORMAL);
	display_byte(starting_address&0xFF, 48, 0, A_NORMAL);

	DrawChar(78, 0, '0', A_REVERSE);
	DrawChar(84, 0, 'x', A_REVERSE);
	display_byte(current_address>>24, 90, 0, A_REVERSE);
	display_byte((current_address>>16)&0xFF, 102, 0, A_REVERSE);
	display_byte((current_address>>8)&0xFF, 114, 0, A_REVERSE);
	display_byte(current_address&0xFF, 126, 0, A_REVERSE);

	for(i = 0; i < 10; i++){
		for(j = 0; j < 8; j++){
			if(starting_address + i*8 + j == current_address){
				TRY
					display_byte(peekIO(current_address), j*18, (i + 1)*8, A_REVERSE);
				ONERR
					DrawChar('0', j*18, (i + 1)*8, A_REVERSE);
					DrawChar('0', j*18 + 6, (i + 1)*8, A_REVERSE);
				ENDTRY
			} else {
				TRY
					display_byte(peekIO(starting_address + i*8 + j), j*18, (i + 1)*8, A_NORMAL);
				ONERR
					DrawChar('0', j*18, (i + 1)*8, A_NORMAL);
					DrawChar('0', j*18 + 6, (i + 1)*8, A_NORMAL);
				ENDTRY
			}
		}
	}
	if(!editing){
		set_status("ENTER to write | MODE to go to mem");
	} else if(memory_violation){
		set_status("Protected memory violation");
	} else {
		set_status("Enter hex data");
	}
}

void _main(){
	int key;
	unsigned long int starting_address;
	unsigned long int current_address;
	unsigned long int next_address;
	unsigned char byte_entered;

	editing = 0;
	ST_busy(ST_IDLE);
	starting_address = 0;
	current_address = 0;
	memset(virtual, 0, sizeof(char)*LCD_SIZE);
	PortSet(virtual, 239, 127);
	update_display(starting_address, current_address);
	LCD_restore(virtual);
	while(editing || (key = ngetchx()) != KEY_ESC){
		if(!editing){
			switch(key){
				case KEY_UP:
					current_address -= 8;
					break;
				case KEY_DOWN:
					current_address += 8;
					break;
				case KEY_RIGHT:
					current_address++;
					break;
				case KEY_LEFT:
					current_address--;
					break;
				case KEY_MODE:
					next_address = enter_address(current_address);
					if(next_address != current_address){
						current_address = next_address;
						starting_address = current_address - current_address%8;
					}
					break;
				case KEY_ENTER:
					editing = 1;
					memory_violation = 0;
					break;
			}
		} else {
			if(!enter_byte(&byte_entered)){
				TRY
					pokeIO(current_address, byte_entered);
				ONERR
					memory_violation = 1;
				ENDTRY
				current_address++;
			} else {
				editing = 0;
			}
		}

		if(starting_address > current_address){
			starting_address = current_address - current_address%8;
		} else if(starting_address < current_address - current_address%8 - 72 && current_address >= 80){
			starting_address = current_address - current_address%8 - 72;
		}
		update_display(starting_address, current_address);
		LCD_restore(virtual);
	}

	PortRestore();
}

