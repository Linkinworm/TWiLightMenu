#include <nds.h>
#include <stdio.h>
#include <maxmod9.h>

#include "common/systemdetails.h"
#include "common/dsimenusettings.h"
#include "graphics/ThemeTextures.h"
#include "autoboot.h"

extern const char *unlaunchAutoLoadID;
extern char unlaunchDevicePath[256];
extern void unlaunchSetHiyaBoot();


extern bool rocketVideo_playVideo;
extern bool music;
extern bool showdialogbox;
extern int dbox_Ypos;


extern u16 bmpImageBuffer[256*192];
u16* sdRemovedImage = (u16*)0x026E0000;

extern u16 convertToDsBmp(u16 val);

void loadSdRemovedImage(void) {
	FILE* file = fopen((sys().arm7SCFGLocked() ? "nitro:/graphics/sdRemovedSimple.bmp" : "nitro:/graphics/sdRemoved.bmp"), "rb");
	if (file) {
		// Start loading
		fseek(file, 0xe, SEEK_SET);
		u8 pixelStart = (u8)fgetc(file) + 0xe;
		fseek(file, pixelStart, SEEK_SET);
		fread(bmpImageBuffer, 2, 0x18000, file);
		u16* src = bmpImageBuffer;
		int x = 0;
		int y = 191;
		for (int i=0; i<256*192; i++) {
			if (x >= 256) {
				x = 0;
				y--;
			}
			u16 val = *(src++);
			sdRemovedImage[y*256+x] = tex().convertToDsBmp(val);
			x++;
		}
	}
	fclose(file);
}

void checkSdEject(void) {
	if (*(u8*)(0x023FF002) == 0 || !isDSiMode()) return;
	
	// Show "SD removed" screen
	rocketVideo_playVideo = false;
	music = false;
	if (showdialogbox) {
		showdialogbox = false;
		dbox_Ypos = 192;
	}
	mmEffectCancelAll();

	videoSetMode(MODE_3_2D | DISPLAY_BG3_ACTIVE);
	videoSetModeSub(MODE_3_2D | DISPLAY_BG3_ACTIVE);

	REG_BLDY = 0;

	dmaCopyWordsAsynch(0, sdRemovedImage, BG_GFX, 0x18000);
	dmaFillWords(0, BG_GFX_SUB, 0x18000);

	while(1) {
		scanKeys();
		if (keysDown() & KEY_B) {
			if (ms().consoleModel < 2 && ms().launcherApp != -1) {
				memcpy((u8*)0x02000800, unlaunchAutoLoadID, 12);
				*(u16*)(0x0200080C) = 0x3F0;		// Unlaunch Length for CRC16 (fixed, must be 3F0h)
				*(u16*)(0x0200080E) = 0;			// Unlaunch CRC16 (empty)
				*(u32*)(0x02000810) = (BIT(0) | BIT(1));		// Load the title at 2000838h
																// Use colors 2000814h
				*(u16*)(0x02000814) = 0x7FFF;		// Unlaunch Upper screen BG color (0..7FFFh)
				*(u16*)(0x02000816) = 0x7FFF;		// Unlaunch Lower screen BG color (0..7FFFh)
				memset((u8*)0x02000818, 0, 0x20+0x208+0x1C0);		// Unlaunch Reserved (zero)
				int i2 = 0;
				for (int i = 0; i < (int)sizeof(unlaunchDevicePath); i++) {
					*(u8*)(0x02000838+i2) = unlaunchDevicePath[i];		// Unlaunch Device:/Path/Filename.ext (16bit Unicode,end by 0000h)
					i2 += 2;
				}
				while (*(u16*)(0x0200080E) == 0) {	// Keep running, so that CRC16 isn't 0
					*(u16*)(0x0200080E) = swiCRC16(0xFFFF, (void*)0x02000810, 0x3F0);		// Unlaunch CRC16
				}
			} 
			fifoSendValue32(FIFO_USER_02, 1);	// ReturntoDSiMenu
			swiWaitForVBlank();
		}
		if (*(u8*)(0x023FF002) == 2 && !sys().arm7SCFGLocked()) {
			if (ms().consoleModel < 2) {
				unlaunchSetHiyaBoot();
			}
			memcpy((u32*)0x02000300, autoboot_bin, 0x020);
			fifoSendValue32(FIFO_USER_02, 1);	// ReturntoDSiMenu
			swiWaitForVBlank();
		}
		swiWaitForVBlank();
	}
}