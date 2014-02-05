#include <nds.h>
#include <stdio.h>
#include <fat.h>

static void eraseChip();
static void eraseSector(vu32* base);
static u32 deviceID();

void burnRom(const char* inf, int eraseFirst) {
	FILE* in = fopen(inf, "rb");
	if (!in) {
		return;
	}
	if (eraseFirst) {
		eraseChip();
	}
	u32 word;

	printf("Flashing ROM...\n");
	vu32* first = (vu32*) 0x08001554;
	vu32* second = (vu32*) 0x08000aa8;
	vu32* base = (vu32*) 0x08000000;

	while (fread(&word, 4, 1, in)) {
		if (!eraseFirst && !((int) base & 0x7FFFF)) {
			eraseSector(base);
		}
		*first = 0x00aa00aa;
		*second = 0x00550055;
		*first = 0x00a000a0;
		*base = word;
		while (*base != word);
		++base;
	}

	fclose(in);
	printf("Done!\n");
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

static void eraseChip() {
	printf("Erasing flash...\n");
	vu32* first = (vu32*) 0x08001554;
	vu32* second = (vu32*) 0x08000aa8;
	*first = 0x00aa00aa;
	*second = 0x00550055;
	*first = 0x00800080;

	*first = 0x00aa00aa;
	*second = 0x00550055;
	*first = 0x00100010;

	printf("Waiting for flash to settle...\n");
	printf("(This will take several minutes)\n");
	vu32* base = (vu32*) 0x08000000;
	while (*base != -1);
	printf("Done!\n");
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
		burnRom("fat:/rom.gba", 0);
	}

	while(1){
		swiWaitForVBlank();
	}

	return 0;
}
