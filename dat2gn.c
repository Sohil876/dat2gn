/*********************************************************
 * dat2gn - kawak's dat file converter to gngeo's format *
 *********************************************************
 * dat2gn reads a dat file in standart input and produces
 * the corresponding romrc file suitable for gngeo on the
 * standart output.
 *
 * This piece of code is weird, but it seems to work.
 * Crypted ROMs are not supported ATM.
 *
 * Author :
 *  Jean-Matthieu COULON
 *  <j@chaperon.homelinux.org>
 * 
 ********************************************************/

/* TODO : encrypted ROMs support
 * TODO : check boundaries of arrays
 * TODO : better syntax checking
*/

#define VERSION "20050124"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

struct romfile {
	char type; // p, s, m, v or c
	int slot; // from 0 to 7 or more ...
	char name[16];
	unsigned int offset; // Offset in memory space
	unsigned int size; // Size in memory space
	unsigned char key; // Crypt key
};


char romname[12];
char romlongname[1024];
	
struct romfile rom[256];
int romcount=0;
	
int crypt=0;
char key[8];
int gameid;

/* Splits the Key: Value pairs and puts Value into destination */
void putvalue(char* dest, char* src) {
	char* p;
//	fprintf(stderr, "putvalue(%s) --> ", src);

	p = strchr(src,':');
	p++; // skip ':'
	
	while( *p==' ' ) p++; // skip whitespaces
	// TODO:remove trailing whitespaces

	strcpy(dest, p);
//	fprintf(stderr, "%s\n", dest);
}

/* Split ROM lines (romname.ext,OFFSET,SIZE,CRC,... */
void splitline(char* name, unsigned int* offset, unsigned int* size, char* line) {
	while(*line && *line!=',') {
		*name = *line;
		name++;
		line++;
	}
	
	name=0;
	if(line) line++;
	
	sscanf(line, "%x,%x,", offset, size);
}

/* Get the slot number by guessing it from the ROM name
 * Example : getslot("262-c3.bin") will return 3
*/
int getslot(char* romname) {
	romname=strchr(romname, '.'); // Find the dot in filename
	do --romname; while(*romname >= '0' && *romname <= '9'); // Find starting point of the number
	return atoi(++romname);
}

/* Reads stdin and interprets it as a DAT file.
 * rom[i] is filled with information read.
*/
void readdat() {
	char line[1024];
	char tmpbuf[1024];

	char type=0;

	while(!feof(stdin)) {
		fgets(line, 1024, stdin);
		line[(strlen(line)-1)&0x7FF]=0; // Erase \n at end of line
	
		 // If it is a DOS file, erase the \r also
		if(line[(strlen(line)-1)&0x7FF]=='\r')
			line[(strlen(line)-1)&0x7FF]=0;
	
		if(line[0]==0 || line[0]=='%') // Empty line !
			{}                     // (% is a field separator)
	
		/********** First section **********/
		else if(type==0 && strncasecmp(line, "RomName", 7)==0)
			putvalue(romname, line);

		else if(type==0 && strncasecmp(line, "Game", 4)==0)
			putvalue(romlongname, line);
		
		/********** Section change **********/
		else if(line[0]=='[') {
//			fprintf(stderr, "Mode : %s", line);

			if(!strncasecmp(line, "[program]", 1024))
				type='p';
			
			else if(!strncasecmp(line, "[text]", 1024))
				type='s';
			
			else if(!strncasecmp(line, "[Z80]", 1024))
				type='m';
			
			else if(!strncasecmp(line, "[samples]", 1024))
				type='v';
			
			else if(!strncasecmp(line, "[graphics]", 1024))
				type='c';
			
			else if(!strncasecmp(line, "[system]", 1024))
				type=1;

//			fprintf(stderr, " = %c\n", type);
		}

		/********** System section *********/
		else if(type==1) {
			putvalue(tmpbuf, line);
			if(!strncasecmp(line, "GfxCrypt", 8))
				crypt=atoi(tmpbuf);
			
			else if(!strncasecmp(line, "GfxKey", 6))
				strncpy(key, tmpbuf, 2);

			else if(!strncasecmp(line, "CartrigeID", 10))
				gameid=atoi(tmpbuf);
		}

		/********** ROM sections **********/
		else if(type>' ') {
//			fprintf(stderr, "Rom : ");
			splitline( rom[romcount].name,
			           &(rom[romcount].offset),
				   &(rom[romcount].size),
				   line);
			
//			fprintf(stderr, "%s (0x%x,0x%x) - ",
//					rom[romcount].name,
//					rom[romcount].offset,
//					rom[romcount].size);
			
			rom[romcount].type=type;
			rom[romcount].slot=getslot(rom[romcount].name);

//			fprintf(stderr, "slot=%d\n", rom[romcount].slot);

			romcount++;
		}
	}

}

/* Generates a romrc file suitable for gngeo from the information
 * in rom[i] structures
*/
void printromrc() {
	int i;
	unsigned int size;

	printf("game %s MVS \"%s\"\n", romname, romlongname);

	// SFIX (type s)
	size=0;
	for(i=0; i<romcount; i++)
		if(rom[i].type=='s')
			size+=rom[i].size;
	printf("SFIX 0x%x\n", size);

	for(i=0; i<romcount; i++)
		if(rom[i].type=='s')
			printf("%s 0x%x 0x%x NORM\n", rom[i].name, rom[i].offset, rom[i].size);

	printf("END\n");
	
	// SM1 (type m)
	size=0;
	for(i=0; i<romcount; i++)
		if(rom[i].type=='m')
			size+=rom[i].size;
	printf("SM1 0x%x\n", size);

	for(i=0; i<romcount; i++)
		if(rom[i].type=='m')
			printf("%s 0x%x 0x%x NORM\n", rom[i].name, rom[i].offset, rom[i].size);

	printf("END\n");
	
	// SOUND1 (type v)
	size=0;
	for(i=0; i<romcount; i++)
		if(rom[i].type=='v')
			size+=rom[i].size;
	printf("SOUND1 0x%x\n", size);

	for(i=0; i<romcount; i++)
		if(rom[i].type=='v')
			printf("%s 0x%x 0x%x NORM\n", rom[i].name, rom[i].offset, rom[i].size);

	printf("END\n");
	
	//CPU (type p)
	size=0;
	for(i=0; i<romcount; i++)
		if(rom[i].type=='p')
			size+=rom[i].size;
	printf("CPU 0x%x\n", size);

	for(i=0; i<romcount; i++)
		if(rom[i].type=='p')
			printf("%s 0x%x 0x%x NORM\n", rom[i].name, rom[i].offset, rom[i].size);

	printf("END\n");
	
	// GFX (type c) - possibly encrypted
	size=0;
	for(i=0; i<romcount; i++)
		if(rom[i].type=='c')
			size+=rom[i].size;
	printf("GFX 0x%x\n", size);

	for(i=0; i<romcount; i++)
		if(rom[i].type=='c')
			printf("%s 0x%x 0x%x ALTERNATE\n", rom[i].name, rom[i].offset, rom[i].size);

	printf("END\n");

	// Encryption data would go here

	printf("END\n\n");
}

int main(int argc, char** argv) {
	if(argc!=1) {
		fprintf(stderr, "%s: Wrong argument count.\n",argv[0]);
		fprintf(stderr, "Usage: %s < file.dat >> /usr/share/gngeo/romrc\n",argv[0]);
		fprintf(stderr, "%s version %s :\n", argv[0], VERSION);
		fprintf(stderr, "Converts a Kawaks DAT file into gngeo's format.\nRemember that you must legally own the game to play it.\n");
		exit(1);
	}

	readdat();
	printromrc();
}
