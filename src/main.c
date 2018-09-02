#include <nds.h>
#include <stdio.h>
#include <fat.h>
#include <dirent.h>

static void eraseSector(vu32* base);
static u32 deviceID();
extern void substr(char *buffer, size_t buflen, char const *source, int len);

void burnRom(struct dirent **namelist, int n) {

	printf("Copying menu rom...\n");
	vu32* start = (vu32*) 0x08000000;
	u32 *menu = (u32*)malloc(16384);
        char *title = (char*)malloc(32);
	int i = 0;

	for (i = 0; i < 4096; i++) {
		menu[i] = *start;
		++start;
	}

	u32 word;

	printf("Menu rom copied.\n");
	vu32* first = (vu32*) 0x08001554;
	vu32* second = (vu32*) 0x08000aa8;
	vu32* base = (vu32*) 0x08000000;

	printf("Flashing menu...\n");
	eraseSector(base);
	for (i = 0; i < 4096; i++) {
		*first = 0x00aa00aa;
		*second = 0x00550055;
		*first = 0x00a000a0;
		*base = menu[i];
		while (*base != menu[i]);
		++base;
	}

	for (i = 0; i < n; i++)
	{
		int index = 0;
		int letter = 0;
		char inf[80];

		strcpy(inf, "fat:/gba/");
		strcat(inf, namelist[i]->d_name);
	        FILE* in = fopen(inf, "rb");
	        if (!in) {
	                return;
	        }

		// calculate size of rom for header
		fseek(in, 0, SEEK_END);
		int file_size = ftell(in);
		fseek(in, 0, SEEK_SET);
		while (file_size % 0x1000 != 0)
			file_size++;
		u32 shift = file_size * 0x10;

		u32 size_part_1 = (u32) (0x00008200 + shift);
		u32 size_part_2 = (u32) 0x0000e010;

		if (i > 0)
		{
			while(((int) base & 0xFFFFFF) % 0x1000 != 0)
			{
	                        if (!((int) base & 0x1FFFF)) {
        	                        printf("Flashing sector at %08lX\n", (u32) base);
                	                eraseSector(base);
                        	}
		                *first = 0x00aa00aa;
		                *second = 0x00550055;
                		*first = 0x00a000a0;
                		*base = 0x00;
                		while (*base != 0x00);
                		++base;
			}
		}

		memset(title, '\0', 32);
		int length = strlen(namelist[i]->d_name);
		substr(title, 32, namelist[i]->d_name, length - 4);

		while (fread(&word, 4, 1, in)) {
			if (!((int) base & 0x1FFFF)) {
				printf("Flashing sector at %08lX\n", (u32) base);
				eraseSector(base);
			}
			*first = 0x00aa00aa;
			*second = 0x00550055;
			*first = 0x00a000a0;
			if (index > 35 && index < 44)
			{
				word = 0;
				word |= (u32)title[letter+3] << 24;
				word |= (u32)title[letter+2] << 16;
				word |= (u32)title[letter+1] << 8;
				word |= (u32)title[letter];
				letter += 4;
			}
			else if (index == 45)
			{
				word = size_part_1;
			}
			else if (index == 46)
			{
				word = size_part_2;
			}
			*base = word;
			while (*base != word);
			++base;
			index++;
		}
		fclose(in);
	}

	printf("Done!\n");
}

void substr(char *buffer, size_t buflen, char const *source, int len)
{
    size_t srclen = strlen(source);
    size_t nbytes = 0;
    size_t offset = 0;
    size_t sublen;

    if (buflen == 0)    /* Can't write anything anywhere */
        return;
    if (len > 0)
    {
        sublen = len;
        nbytes = (sublen > srclen) ? srclen : sublen;
        offset = 0;
    }
    else if (len < 0)
    {
        sublen = -len;
        nbytes = (sublen > srclen) ? srclen : sublen;
        offset = srclen - nbytes;
    }
    if (nbytes >= buflen)
        nbytes = 0;
    if (nbytes > 0)
        memmove(buffer, source + offset, nbytes);
    buffer[nbytes] = '\0';
}

static int parse_ext(const struct dirent *dir)
{
	if(!dir)
		return 0;

	if(dir->d_type == DT_REG) 
	{
		const char *ext = strrchr(dir->d_name,'.');
		if((!ext) || (ext == dir->d_name))
			return 0;
		else {
			if(strcasecmp(ext, ".gba") == 0)
				return 1;
		}
	}
	return 0;
}

void setMode(u16 major, u16 minor) {
	*(vu16*) 0x09FFFFFE = 0x5959;
	*(vu16*) 0x09FFFFFC = major;
	*(vu16*) 0x09FFFFFA = minor;
	*(vu16*) 0x09FFFFF8 = 0;
	*(vu16*) 0x09FFFFFE = 0;
}

static u32 deviceID() {
	vu32* first = (vu32*) 0x08001554;
	vu32* second = (vu32*) 0x08000aa8;
	vu32* base = (vu32*) 0x08000004;
	*first = 0x00aa00aa;
	*second = 0x00550055;
	*first = 0x00900090;

	u32 id = *base;

	*first = 0x00aa00aa;
	*second = 0x00550055;
	*first = 0x00f000f0;
	return id;
}

static void eraseSector(vu32* base) {
	vu32* first = (vu32*) 0x08001554;
	vu32* second = (vu32*) 0x08000aa8;
	*first = 0x00aa00aa;
	*second = 0x00550055;
	*first = 0x00800080;

	*first = 0x00aa00aa;
	*second = 0x00550055;
	*base = 0x00300030;

	while (*base != -1);
}

int main(void) {
	consoleDemoInit();
	fatInitDefault();

	videoSetMode(MODE_FB0);

	vramSetBankA(VRAM_A_LCD);

	sysSetCartOwner(1);
	setMode(15, 0);

	u32 code = deviceID();
	if (code != 0x227e227e) {
		printf("Not an EZF Advance!\n");
	} else {
		printf("Detected EZF Advance!\n");
		struct dirent **namelist;
		int n;

		n = scandir("fat:/gba/.", &namelist, parse_ext, alphasort);
		if (n < 0) {
			perror("scandir");
			return 1;
		}
		else {
			if (n == 0)
				printf("No GBA files found in /gba directory.");
			else
				burnRom(namelist, n);
			free(namelist);
		}
	}

	while(1){
		swiWaitForVBlank();
	}

	return 0;
}
