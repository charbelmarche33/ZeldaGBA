/*
 * tiles.c
 * program which demonstraes tile mode 0
 */

#include <stdio.h>
 
/* include the image we are using */
#include "background.h"

/* include the link sprite image we are using */
#include "link.h"

/* include the tile map we are using */
#include "map.h"
#include "map2.h"

/* the width and height of the screen */
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160

/* the three tile modes */
#define MODE0 0x00
#define MODE1 0x01
#define MODE2 0x02

/* enable bits for the four tile layers */
#define BG0_ENABLE 0x100
#define BG1_ENABLE 0x200
#define BG2_ENABLE 0x400
#define BG3_ENABLE 0x800

/* flags to set sprite handling in display control register */
#define SPRITE_MAP_2D 0x0
#define SPRITE_MAP_1D 0x40
#define SPRITE_ENABLE 0x1000


/* the control registers for the four tile layers */
volatile unsigned short* bg0_control = (volatile unsigned short*) 0x4000008;
volatile unsigned short* bg1_control = (volatile unsigned short*) 0x400000a;
volatile unsigned short* bg2_control = (volatile unsigned short*) 0x400000c;
volatile unsigned short* bg3_control = (volatile unsigned short*) 0x400000e;

/* palette is always 256 colors */
#define PALETTE_SIZE 256

/* there are 128 sprites on the GBA */
#define NUM_SPRITES 128

/* the display control pointer points to the gba graphics register */
volatile unsigned long* display_control = (volatile unsigned long*) 0x4000000;

/* the address of the color palette */
volatile unsigned short* bg_palette = (volatile unsigned short*) 0x5000000;
volatile unsigned short* sprite_palette = (volatile unsigned short*) 0x5000200;

/* the memory location which controls sprite attributes */
volatile unsigned short* sprite_attribute_memory = (volatile unsigned short*) 0x7000000;

/* the memory location which stores sprite image data */
volatile unsigned short* sprite_image_memory = (volatile unsigned short*) 0x6010000;

/* the memory location which stores sprite image data */
volatile unsigned short* sprite_image_memory1 = (volatile unsigned short*) 0x6050000;


/* the button register holds the bits which indicate whether each button has
 * been pressed - this has got to be volatile as well
 */
volatile unsigned short* buttons = (volatile unsigned short*) 0x04000130;

/* scrolling registers for backgrounds */
volatile short* bg0_x_scroll = (unsigned short*) 0x4000010;
volatile short* bg0_y_scroll = (unsigned short*) 0x4000012;
volatile short* bg1_x_scroll = (unsigned short*) 0x4000014;
volatile short* bg1_y_scroll = (unsigned short*) 0x4000016;
volatile short* bg2_x_scroll = (unsigned short*) 0x4000018;
volatile short* bg2_y_scroll = (unsigned short*) 0x400001a;
volatile short* bg3_x_scroll = (unsigned short*) 0x400001c;
volatile short* bg3_y_scroll = (unsigned short*) 0x400001e;


/* the bit positions indicate each button - the first bit is for A, second for
 * B, and so on, each constant below can be ANDED into the register to get the
 * status of any one button */
#define BUTTON_A (1 << 0)
#define BUTTON_B (1 << 1)
#define BUTTON_SELECT (1 << 2)
#define BUTTON_START (1 << 3)
#define BUTTON_RIGHT (1 << 4)
#define BUTTON_LEFT (1 << 5)
#define BUTTON_UP (1 << 6)
#define BUTTON_DOWN (1 << 7)
#define BUTTON_R (1 << 8)
#define BUTTON_L (1 << 9)


/* return a pointer to one of the 32 screen blocks (0-31) */
volatile unsigned short* screen_block(unsigned long block) {
    /* they are each 2K big */
    return (volatile unsigned short*) (0x6000000 + (block * 0x800));
}

 /* function to set text on the screen at a given location */
void set_text(char* str, int row, int col) {                    
    /* find the index in the texmap to draw to */
    int index = row * 32 + col;

    /* the first 32 characters are missing from the map (controls etc.) */
    int missing = 32; 

    /* pointer to text map */
    volatile unsigned short* ptr = screen_block(31);

    /* for each character */
    while (*str) {
        /* place this character in the map */
        ptr[index] = *str - missing;

        /* move onto the next character */
        index++;
        str++;
    }   
} 

/* the scanline counter is a memory cell which is updated to indicate how
 * much of the screen has been drawn */
volatile unsigned short* scanline_counter = (volatile unsigned short*) 0x4000006;

/* wait for the screen to be fully drawn so we can do something during vblank */
void wait_vblank() {
    /* wait until all 160 lines have been updated */
    while (*scanline_counter < 160) { }
}


/* this function checks whether a particular button has been pressed */
unsigned char button_pressed(unsigned short button) {
    /* and the button register with the button constant we want */
    unsigned short pressed = *buttons & button;

    /* if this value is zero, then it's not pressed */
    if (pressed == 0) {
        return 1;
    } else {
        return 0;
    }
}


/* return a pointer to one of the 4 character blocks (0-3) */
volatile unsigned short* char_block(unsigned long block) {
    /* they are each 16K big */
    return (volatile unsigned short*) (0x6000000 + (block * 0x4000));
}


/* flag for turning on DMA */
#define DMA_ENABLE 0x80000000

/* flags for the sizes to transfer, 16 or 32 bits */
#define DMA_16 0x00000000
#define DMA_32 0x04000000

/* pointer to the DMA source location */
volatile unsigned int* dma_source = (volatile unsigned int*) 0x40000D4;

/* pointer to the DMA destination location */
volatile unsigned int* dma_destination = (volatile unsigned int*) 0x40000D8;

/* pointer to the DMA count/control */
volatile unsigned int* dma_count = (volatile unsigned int*) 0x40000DC;

/* copy data using DMA */
void memcpy16_dma(unsigned short* dest, unsigned short* source, int amount) {
    *dma_source = (unsigned int) source;
    *dma_destination = (unsigned int) dest;
    *dma_count = amount | DMA_16 | DMA_ENABLE;
}


/* function to setup background 0 for this program */
void setup_background() {

    /* load the palette from the image into palette memory*/
    /*for (int i = 0; i < PALETTE_SIZE; i++) {
        bg_palette[i] = background_palette[i];
    }*/

    /* load the image into char block 0 (16 bits at a time) */
    volatile unsigned short* dest = char_block(0);
    /*unsigned short* image = (unsigned short*) background_data;
    for (int i = 0; i < ((background_width * background_height) / 2); i++) {
        dest[i] = image[i];
    }*/

    /* load the palette from the image into palette memory*/
    memcpy16_dma((unsigned short*) bg_palette, (unsigned short*) background_palette, PALETTE_SIZE);

    /* load the image into char block 0 */
    memcpy16_dma((unsigned short*) char_block(0), (unsigned short*) background_data,
            (background_width * background_height) / 2);
    
    /* set all control the bits in this register */
    *bg0_control = 2 |    /* priority, 0 is highest, 3 is lowest */
        (0 << 2)  |       /* the char block the image data is stored in */
        (0 << 6)  |       /* the mosaic flag */
        (1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
        (25 << 8) |       /* the screen block the tile data is stored in */
        (1 << 13) |       /* wrapping flag */
        (0 << 14);        /* bg size, 0 is 256x256 */
    
    dest = screen_block(25);
    for (int i = 0; i < (map2_width * map2_height); i++) {
        dest[i] = map2[i];
    }
    /* load the tile data into screen block 16 */
    //memcpy16_dma((unsigned short*) screen_block(16), (unsigned short*) map2, map2_width * map2_height);


    /* set all control the bits in this register */
    *bg1_control = 0 |    /* priority, 0 is highest, 3 is lowest */
        (0 << 2)  |       /* the char block the image data is stored in */
        (0 << 6)  |       /* the mosaic flag */
        (1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
        (24 << 8) |       /* the screen block the tile data is stored in */
        (1 << 13) |       /* wrapping flag */
        (0 << 14);        /* bg size, 0 is 256x256 */
    
    /* load the tile data into screen block 30 */
    dest = screen_block(24);
    for (int i = 0; i < (map_width * map_height); i++) {
        dest[i] = map[i];
    }
	
	/* bg2 is our actual text background */
    *bg2_control = 1 |    /* priority, 0 is highest, 3 is lowest */
        (0 << 2)  |       /* the char block the image data is stored in */
        (0 << 6)  |       /* the mosaic flag */
        (1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
        (31 << 8) |       /* the screen block the tile data is stored in */
        (1 << 13) |       /* wrapping flag */
        (0 << 14);        /* bg size, 0 is 256x256 */
		
	/* clear the text map to be all blanks */
    dest = screen_block(31);
    for (int i = 0; i < 32 * 32; i++) {
        dest[i] = 0;
    }

}


/* a sprite is a moveable image on the screen */
struct Sprite {
    unsigned short attribute0;
    unsigned short attribute1;
    unsigned short attribute2;
    unsigned short attribute3;
};

/* array of all the sprites available on the GBA */
struct Sprite sprites[NUM_SPRITES];
int next_sprite_index = 0;

/* the different sizes of sprites which are possible */
enum SpriteSize {
    SIZE_8_8,
    SIZE_16_16,
    SIZE_32_32,
    SIZE_64_64,
    SIZE_16_8,
    SIZE_32_8,
    SIZE_32_16,
    SIZE_64_32,
    SIZE_8_16,
    SIZE_8_32,
    SIZE_16_32,
    SIZE_32_64
};

/* function to initialize a sprite with its properties, and return a pointer */
struct Sprite* sprite_init(int x, int y, enum SpriteSize size,
        int horizontal_flip, int vertical_flip, int tile_index, int priority) {

    /* grab the next index */
    int index = next_sprite_index++;

    /* setup the bits used for each shape/size possible */
    int size_bits, shape_bits;
    switch (size) {
        case SIZE_8_8:   size_bits = 0; shape_bits = 0; break;
        case SIZE_16_16: size_bits = 1; shape_bits = 0; break;
        case SIZE_32_32: size_bits = 2; shape_bits = 0; break;
        case SIZE_64_64: size_bits = 3; shape_bits = 0; break;
        case SIZE_16_8:  size_bits = 0; shape_bits = 1; break;
        case SIZE_32_8:  size_bits = 1; shape_bits = 1; break;
        case SIZE_32_16: size_bits = 2; shape_bits = 1; break;
        case SIZE_64_32: size_bits = 3; shape_bits = 1; break;
        case SIZE_8_16:  size_bits = 0; shape_bits = 2; break;
        case SIZE_8_32:  size_bits = 1; shape_bits = 2; break;
        case SIZE_16_32: size_bits = 2; shape_bits = 2; break;
        case SIZE_32_64: size_bits = 3; shape_bits = 2; break;
    }

    int h = horizontal_flip ? 1 : 0;
    int v = vertical_flip ? 1 : 0;

    /* set up the first attribute */
    sprites[index].attribute0 = y |             /* y coordinate */
                            (0 << 8) |          /* rendering mode */
                            (0 << 10) |         /* gfx mode */
                            (0 << 12) |         /* mosaic */
                            (1 << 13) |         /* color mode, 0:16, 1:256 */
                            (shape_bits << 14); /* shape */

    /* set up the second attribute */
    sprites[index].attribute1 = x |             /* x coordinate */
                            (0 << 9) |          /* affine flag */
                            (h << 12) |         /* horizontal flip flag */
                            (v << 13) |         /* vertical flip flag */
                            (size_bits << 14);  /* size */

    /* setup the second attribute */
    sprites[index].attribute2 = tile_index |   // tile index */
                            (priority << 10) | // priority */
                            (0 << 12);         // palette bank (only 16 color)*/

    /* return pointer to this sprite */
    return &sprites[index];
}

/* update all of the spries on the screen */
void sprite_update_all() {
    /* copy them all over */
    memcpy16_dma((unsigned short*) sprite_attribute_memory, (unsigned short*) sprites, NUM_SPRITES * 4);
}

/* setup all sprites */
void sprite_clear() {
    /* clear the index counter */
    next_sprite_index = 0;

    /* move all sprites offscreen to hide them */
    for(int i = 0; i < NUM_SPRITES; i++) {
        sprites[i].attribute0 = SCREEN_HEIGHT;
        sprites[i].attribute1 = SCREEN_WIDTH;
    }
}

/* set a sprite postion */
void sprite_position(struct Sprite* sprite, int x, int y) {
    /* clear out the y coordinate */
    sprite->attribute0 &= 0xff00;

    /* set the new y coordinate */
    sprite->attribute0 |= (y & 0xff);

    /* clear out the x coordinate */
    sprite->attribute1 &= 0xfe00;

    /* set the new x coordinate */
    sprite->attribute1 |= (x & 0x1ff);
}

/* move a sprite in a direction */
void sprite_move(struct Sprite* sprite, int dx, int dy) {
    /* get the current y coordinate */
    int y = sprite->attribute0 & 0xff;

    /* get the current x coordinate */
    int x = sprite->attribute1 & 0x1ff;

    /* move to the new location */
    sprite_position(sprite, x + dx, y + dy);
}

/* change the vertical flip flag */
void sprite_set_vertical_flip(struct Sprite* sprite, int vertical_flip) {
    if (vertical_flip) {
        /* set the bit */
        sprite->attribute1 |= 0x2000;
    } else {
        /* clear the bit */
        sprite->attribute1 &= 0xdfff;
    }
}

/* change the vertical flip flag */
void sprite_set_horizontal_flip(struct Sprite* sprite, int horizontal_flip) {
    if (horizontal_flip) {
        /* set the bit */
        sprite->attribute1 |= 0x1000;
    } else {
        /* clear the bit */
        sprite->attribute1 &= 0xefff;
    }
}

int is_sprite_facing_left(struct Sprite* sprite){
	if(sprite->attribute1 & 0x1000){
		return 1;
	}
	return 0;
}

/* change the tile offset of a sprite */
void sprite_set_offset(struct Sprite* sprite, int offset) {
    /* clear the old offset */
    sprite->attribute2 &= 0xfc00;

    /* apply the new one */
    sprite->attribute2 |= (offset & 0x03ff);
}

/* setup the sprite image and palette */
void setup_sprite_image() {
    /* load the palette from the image into palette memory*/
    memcpy16_dma((unsigned short*) sprite_palette, (unsigned short*) link_palette, PALETTE_SIZE);

    /* load the image into char block 0 */
    memcpy16_dma((unsigned short*) sprite_image_memory, (unsigned short*) link_data, (link_width * link_height) / 2);
}

/* a struct for the link's logic and behavior*/ 
struct Link {
    /* the actual sprite attribute info */
    struct Sprite* sprite;

    /* the x and y postion, in 1/256 pixels */
    int x, y;

    /* the link's y velocity in 1/256 pixels/second */
    int yvel;

    /* the link's y acceleration in 1/256 pixels/second^2 */
    int gravity; 

    /* which frame of the animation he is on */
    int frame;

    /* the number of frames to wait before flipping */
    int animation_delay;

    /* the animation counter counts how many frames until we flip */
    int counter;

    /* whether the link is moving right now or not */
    int move;

    /* whether the link is doing a sword attack or nah */
    int sword;

    /* the number of pixels away from the edge of the screen the link stays */
    int border;

    /* if the link is currently falling */
    int falling;
};

/* a struct for the dark link's logic and behavior*/ 
struct DarkLink {
    /* the actual sprite attribute info */
    struct Sprite* sprite;

    /* the x and y postion, in 1/256 pixels */
    int x, y;
	
    /* the link's y velocity in 1/256 pixels/second */
    int yvel;

    /* the link's y acceleration in 1/256 pixels/second^2 */
    int gravity; 

    /* which frame of the animation he is on */
    int frame;

    /* the number of frames to wait before flipping */
    int animation_delay;

    /* the animation counter counts how many frames until we flip */
    int counter;

    /* whether the link is moving right now or not */
    int move;

    /* whether the link is doing a sword attack or nah */
    int sword;

    /* the number of pixels away from the edge of the screen the link stays */
    int border;

    /* if the link is currently falling */
    int falling;	
};

/* initialize the link */
void link_init(struct Link* link) {
    link->x = 40 << 8;
    link->y = 13 << 8;
    link->yvel = 0;
    link->gravity = 50;
    link->border = 20;
    link->frame = 0;
    link->move = 0;
    link->sword = 0;
    link->counter = 0;
    link->falling = 0;
    link->animation_delay = 16;
    link->sprite = sprite_init(link->x >> 8, link->y >> 8, SIZE_32_32, 0, 0, link->frame, 0);
}

/* initialize the dark link*/
void dlink_init(struct DarkLink* dlink, int x){
    dlink->x = x << 8;
    dlink->y = 13 << 8;
    dlink->yvel = 0;
    dlink->gravity = 50;
    dlink->border = 20;
    dlink->frame = 224;
    dlink->move = 0;
    dlink->sword = 0;
    dlink->counter = 0;
    dlink->falling = 0;
    dlink->animation_delay = 16;
    dlink->sprite = sprite_init(dlink->x >> 8, dlink->y >> 8, SIZE_32_32, 0, 0, dlink->frame, 0);
}

/* move the link left or right returns if it is at edge of the screen */
int link_left(struct Link* link) {
    /* face left */
    sprite_set_horizontal_flip(link->sprite, 1);
    link->move = 1;

    /* if we are at the left end, just scroll the screen */
    if ((link->x >> 8) < link->border) {
        return 1;
    } else {
        /* else move left */
        link->x -= 512;
        return 0;
    }
}

int link_right(struct Link* link) {
    /* face right */
    sprite_set_horizontal_flip(link->sprite, 0);
    link->move = 1;

    /* if we are at the right end, just scroll the screen */
    if ((link->x >> 8) > (SCREEN_WIDTH - 16 - link->border)) {
        return 1;
    } else {
        /* else move right */
        link->x += 512;
        return 0;
    }
}

/* stop the link from walking left/right */
void link_stop(struct Link* link) {
    link->move = 0;
    link->frame = 0;
    link->counter = 15;
    sprite_set_offset(link->sprite, link->frame);
}

/* start the link jumping, unless already fgalling */
void link_jump(struct Link* link) {
    if (!link->falling) {
        link->yvel = -1350;
        link->falling = 1;
    }
}

/* start the link jumping, unless already fgalling */
void dlink_jump(struct DarkLink* dlink) {
    if (!dlink->falling) {
        dlink->yvel = -1350;
        dlink->falling = 1;
    }
}


/* move dark link left or right returns if it is at edge of the screen */
int dlink_left(struct DarkLink* dlink, struct Link* link) {
    /* face left */
    sprite_set_horizontal_flip(dlink->sprite, 1);
    dlink->move = 1;

    /* if we are at the left end, just scroll the screen */
    if ((link->x >> 8) < link->border && link->move) {
        return 1;
    } else {
        /* else move left */
        dlink->x -= 256;
        return 0;
    }
}

int dlink_right(struct DarkLink* dlink, struct Link* link) {
    /* face right */
    sprite_set_horizontal_flip(dlink->sprite, 0);
    dlink->move = 1;

    /* if we are at the right end, just scroll the screen */
    if ((link->x >> 8) > (SCREEN_WIDTH - 16 - link->border) && link->move) {
        return 1;
    } else {
        /* else move right */
        dlink->x += 256;
        return 0;
    }
}

int wraparound(int n, int tilemap);

/* finds which tile a screen coordinate maps to, taking scroll into account */
unsigned short tile_lookup(int x, int y, int xscroll, int yscroll,
        const unsigned short* tilemap, int tilemap_w, int tilemap_h) {

    /* adjust for the scroll */
    x += xscroll;
    y += yscroll;

    /* convert from screen coordinates to tile coordinates */
    x >>= 3;
    y >>= 3;

    /* account for wraparound */
	x = wraparound(x, tilemap_w);
	y = wraparound(y, tilemap_h);
    while (x < 0) {
        x += tilemap_w;
    }
    while (y < 0) {
        y += tilemap_h;
    }

    /* lookup this tile from the map */
    int index = y * tilemap_w + x;

    /* return the tile */
    return tilemap[index];
}


/* update the link */
void link_update(struct Link* link, int xscroll) {
    /* update y position and speed if falling */
    if (link->falling) {
        link->y += link->yvel;
        link->yvel += link->gravity;
    }

    /* check which tile the link's feet are over */
    unsigned short tile = tile_lookup((link->x >> 8) + 16, (link->y >> 8) + 32, xscroll,
            0, map, map_width, map_height);

    /* if it's block tile
     * these numbers refer to the tile indices of the blocks the link can walk on */ 
   if ( (tile == 108) || (tile == 109) || (tile >= 146 && tile <= 147) || (tile >= 178 && tile <= 179) || (tile >= 210 && tile <= 211) || (tile >= 242 && tile <= 243)) {
        /* stop the fall! */
        link->falling = 0;
        link->yvel = 0;

        /* make him line up with the top of a block
         * works by clearing out the lower bits to 0 */
        link->y &= ~0x7ff;

        /* move him down one because there is a one pixel gap in the image */
        link->y++;

    } else {
        /* he is falling now */
        link->falling = 1;
    }

    /* update animation if moving */
    if (link->move) {
        link->counter++;
        if (link->counter >= link->animation_delay) {
            link->frame = link->frame + 32;
            if (link->frame > 96) {
                link->frame = 32;
            }
            sprite_set_offset(link->sprite, link->frame);
            link->counter = 0;
        }
    }
    // update animation if sword attack
    if (link->sword) {
	link->counter++;
	if (link->counter >= link->animation_delay) {
	    link->frame = link->frame + 32;
	    if (link->frame > 223) { 
	        link->frame = 160; 
	    }  
	    sprite_set_offset(link->sprite, link->frame);
            link->counter = 0;
	}
    }
    /* set on screen position */
    sprite_position(link->sprite, link->x >> 8, link->y >> 8);
}

/* update the dark link */
void dlink_update(struct DarkLink* dlink, struct Link* link, int xscroll) {
    /* update y position and speed if falling */
    if (dlink->falling) {
        dlink->y += dlink->yvel;
        dlink->yvel += dlink->gravity;
    }	
    /* check which tile the link's feet are over */
    unsigned short tile = tile_lookup((dlink->x >> 8) + 16, (dlink->y >> 8) + 32, xscroll,
            0, map, map_width, map_height);
    /* if it's block tile
     * these numbers refer to the tile indices of the blocks the link can walk on */
    if ( (tile == 108) || (tile == 109) || (tile >= 146 && tile <= 147) || (tile >= 178 && tile <= 179) || (tile >= 210 && tile <= 211) || (tile >= 242 && tile <= 243)) {
        /* stop the fall! */
        dlink->falling = 0;
        dlink->yvel = 0;
		
        /* make him line up with the top of a block
         * works by clearing out the lower bits to 0 */
        dlink->y &= ~0x7ff;

        /* move him down one because there is a one pixel gap in the image */
        dlink->y++;

    } else {
        /* he is falling now */
        dlink->falling = 1;
    }
	
    if(((link->x)) < dlink->x){
	dlink_left(dlink, link);
    }
    else if((link->x) > dlink->x){
	dlink_right(dlink, link);
    }
    if (link->y < dlink->y){
	dlink_jump(dlink);
    }
			
    /* update animation if moving */
    if (dlink->move) {
        dlink->counter++;
        if (dlink->counter >= dlink->animation_delay) {
            dlink->frame = dlink->frame + 32;
            if (dlink->frame > 351) {
                dlink->frame = 224;
            }
            sprite_set_offset(dlink->sprite, dlink->frame);
            dlink->counter = 0;
        }
    }	
    /* set on screen position */
    sprite_position(dlink->sprite, dlink->x >> 8, dlink->y >> 8);
}

//Get link to have the sword attack set to 1
void link_sword(struct Link* link) {
    link->sword = 1;
    if (link->frame < 160) {
        link->frame = 128;
        sprite_set_offset(link->sprite, link->frame);
    }
    if (link->frame = 160) {
        sprite_set_offset(link->sprite, link->frame);
	link->frame = 0;
    }
}

//Stop sword attack
void link_stop_sword(struct Link* link) {
     link->sword = 0;
     //link_stop(link);
}



int link_dead(struct Link* link, struct DarkLink* dlink){
    //****This needs more statements to account for dlink's right side and bottom****
    if ((dlink->x >= link->x && dlink->x <= (link->x + 32) && 
	((dlink->y >= link->y && dlink->y <= link->y+32) || (dlink->y+16 >= link->y && dlink->y+16 <= link->y+32) || (dlink->y+32 >= link->y && dlink->y+32 <= link->y+32))) ||
	(dlink->x+32 >= link->x && dlink->x+32 <= (link->x + 32) &&
        ((dlink->y >= link->y && dlink->y <= link->y+32) || (dlink->y+16 >= link->y && dlink->y+16 <= link->y+32) || (dlink->y+32 >= link->y && dlink->y+32 <= link->y+32))) ||
	(dlink->x+16 >= link->x && dlink->x+16 <= (link->x + 32) &&
        ((dlink->y >= link->y && dlink->y <= link->y+32) || (dlink->y+16 >= link->y && dlink->y+16 <= link->y+32) || (dlink->y+32 >= link->y && dlink->y+32 <= link->y+32))) ||
	(dlink->x+8 >= link->x && dlink->x+8 <= (link->x + 32) &&
        ((dlink->y >= link->y && dlink->y <= link->y+32) || (dlink->y+16 >= link->y && dlink->y+16 <= link->y+32) || (dlink->y+32 >= link->y && dlink->y+32 <= link->y+32))) ||
	(dlink->x+24 >= link->x && dlink->x+24 <= (link->x + 32) &&
        ((dlink->y >= link->y && dlink->y <= link->y+32) || (dlink->y+16 >= link->y && dlink->y+16 <= link->y+32) || (dlink->y+32 >= link->y && dlink->y+32 <= link->y+32))) ||
	((dlink->x-10 <= link->x+32+10 && dlink->x+32+10 >= link->x-10) && 
		((dlink->y-10 <= link->y+32+10 && dlink->y+32+10 >= link->y-10)))) {
	link->frame = 192;
	dlink->frame = 384;
	sprite_set_offset(link->sprite, link->frame);
	sprite_set_offset(dlink->sprite, dlink->frame);
	sprite_update_all();
	return 1;
    }  
    return 0;
}

int link_score(struct Link* link, struct DarkLink* dlink) {
	int is_link_left = is_sprite_facing_left(link->sprite);
	if (is_link_left){
		if ((link->x <= dlink->x+32+10 && link->x+74 >= dlink->x) && 
		(dlink->y-10 <= link->y+32+10 && dlink->y+32+10 >= link->y-10)){
			dlink->frame = 416;
			sprite_set_offset(dlink->sprite, dlink->frame);
			return 1;
		}
	} else {
		if ((link->x <= dlink->x+32+10 && link->x+74 >= dlink->x) && 
		(dlink->y-10 <= link->y+32+10 && dlink->y+32+10 >= link->y-10)){
			dlink->frame = 416;
			sprite_set_offset(dlink->sprite, dlink->frame);
			return 1;
		}
	}
	return 0;
}

/* just kill time */
void delay(unsigned int amount);

/* the main function */
int main() {
    /* we set the mode to mode 0 with bg0 on */
    *display_control = MODE0 | BG1_ENABLE | BG0_ENABLE | BG2_ENABLE | SPRITE_ENABLE | SPRITE_MAP_1D;

    /* setup the background 0 */
    setup_background();

    /* setup the sprite image data */
    setup_sprite_image();

    /* clear all the sprites on screen now */
    sprite_clear();

    /* create the link */
    struct Link link;
    link_init(&link);
	
    /* creates dark link */
    struct DarkLink dlink;
    dlink_init(&dlink, 200);
	
	struct DarkLink dlink_dup_1;
	int duplicate_one_alive = 0;
	struct DarkLink dlink_dup_2;
	int duplicate_two_alive = 0;
	
	int which_dark_link_killed_link;
	
    /* creates ghost*/
    //struct Ghost ghost;
    //ghost_init(&ghost);

    //Ensures that user can't just hold A down to jump continuously and avoids double jumps
    int is_BUTTONA_released = 1;
	//Ensures you can't just hold your sword out forever
	int is_BUTTONB_released = 1;
	int counter = 0;
	
    /* set initial scroll to 0 */
    int xscroll = 0;
    int yscroll = 0;
	
	int score = 0;
	int lives = 3;
	
	char msg[22];
	sprintf(msg, "Score = %d | Lives = %d", score, lives);
	set_text(msg,0,5);

				
	/* loop forever */
	while (1) {
		/* update the link */
		link_update(&link, xscroll);
		
		/* update the dark link */
		dlink_update(&dlink, &link, xscroll);
		
		//if dup 1 alive then update him
		if (duplicate_one_alive) {
			dlink_update(&dlink_dup_1, &link, xscroll);
		}
		
		//if dup2 alive then update him
		if (duplicate_two_alive) {
			dlink_update(&dlink_dup_2, &link, xscroll);
		}
		
		/* now the arrow keys move the link */
		if (button_pressed(BUTTON_RIGHT) && !button_pressed(BUTTON_B)) {
			if (link_right(&link)) {
				xscroll++;
			}
		} else if (button_pressed(BUTTON_LEFT) && !button_pressed(BUTTON_B)) {
			if (link_left(&link)) {
				xscroll--;
			}
		} else {
			link_stop(&link);
		}

		/* check for jumping */
		if (button_pressed(BUTTON_A) && !button_pressed(BUTTON_B) && is_BUTTONA_released == 1) {
			link_jump(&link);
		is_BUTTONA_released = 0;
		} 
		if  (!button_pressed(BUTTON_A)) {
			//This is so that link doesn't double jump cause that is bull shit and so that he doesnt continuously jump 
			is_BUTTONA_released = 1;
		}
		
		int is_dlink_hit;
		
		/* check for sword attack */
		if (button_pressed(BUTTON_B) && is_BUTTONB_released == 1 && counter < 10) {
			is_BUTTONB_released = 0;
			link_sword(&link);
			is_dlink_hit = link_score(&link, &dlink);
			if (is_dlink_hit){
				score++;
				dlink.x = 200 << 8;
				dlink.y = 13 << 8;
			}
			if (duplicate_one_alive) { 
				is_dlink_hit = link_score(&link, &dlink_dup_1);
				if (is_dlink_hit){
					score++;
					dlink_dup_1.x = 10 << 8;
					dlink_dup_1.y = 13 << 8;
				}
			}
			if (duplicate_two_alive) { 
				is_dlink_hit = link_score(&link, &dlink_dup_2);
				if (is_dlink_hit){
					score++;
					dlink_dup_2.x = 100 << 8;
					dlink_dup_2.y = 13 << 8;
				}
			}
			counter++;
		} else if (button_pressed(BUTTON_B) && is_BUTTONB_released == 0 && counter < 10) {
			is_BUTTONB_released = 0;
			link_sword(&link);
			is_dlink_hit = link_score(&link, &dlink);
			if (is_dlink_hit){
				score++;
				dlink.x = 200 << 8;
				dlink.y = 13 << 8;
			}
			if (duplicate_one_alive) { 
				is_dlink_hit = link_score(&link, &dlink_dup_1);
				if (is_dlink_hit){
					score++;
					dlink_dup_1.x = 10 << 8;
					dlink_dup_1.y = 13 << 8;
				}
			}
			if (duplicate_two_alive) { 
				is_dlink_hit = link_score(&link, &dlink_dup_2);
				if (is_dlink_hit){
					score++;
					dlink_dup_2.x = 100 << 8;
					dlink_dup_2.y = 13 << 8;
				}
			}
			counter++;
		} if (counter >= 10 && button_pressed(BUTTON_B)) {
			link_stop_sword(&link);
		} if (!button_pressed(BUTTON_B)) {
			counter = 0;
			is_BUTTONB_released = 1;
			link_stop_sword(&link);
		}
		
		int is_link_dead = link_dead(&link, &dlink);
		if (is_link_dead) {
			which_dark_link_killed_link = 0;
		}
		if (!is_link_dead && duplicate_one_alive) {
			is_link_dead = link_dead(&link, &dlink_dup_1);
			if (is_link_dead) {
				which_dark_link_killed_link = 1;
			}
		} 
		if (!is_link_dead && duplicate_two_alive) {
			is_link_dead = link_dead(&link, &dlink_dup_2);
			if (is_link_dead) {
				which_dark_link_killed_link = 2;
			}
		}

		//If link is dead reinitialize	
		if (is_link_dead && lives <= 0 && !is_dlink_hit){
			score = 0;
			lives = 3;
			
			duplicate_one_alive = 0;
			duplicate_two_alive = 0;
			
			sprintf(msg, "Score = %d | Lives = %d", score, lives);
			set_text(msg,0,0);
			
			/* we set the mode to mode 0 with bg0 on */
			*display_control = MODE0 | BG1_ENABLE | BG0_ENABLE | BG2_ENABLE | SPRITE_ENABLE | SPRITE_MAP_1D;;

			/* setetup the background 0 */
			setup_background();

			/* setup the sprite image data */
			setup_sprite_image();

			/* clear all the sprites on screen now */
			sprite_clear();

			/* create the link */
			link_init(&link);

			/* creates dark link */
			dlink_init(&dlink, 200);

			//Ensures that user can't just hold A down to jump continuously and avoids double jumps
			int is_BUTTONA_released = 1;

			/* set initial scroll to 0 */
			int xscroll = 0;
			int yscroll = 0;

			//Reset the scores
			

			delay(40000);
		} 
		//If you get hit but still have lives
		else if (is_link_dead && lives > 0 && !is_dlink_hit){
			int dlink_left;
			if (which_dark_link_killed_link == 0) {
				//Then initial dlink killed link]
				dlink_left = is_sprite_facing_left(dlink.sprite);
			} else if (which_dark_link_killed_link == 1) {
				//Then initial dlink killed link]
				dlink_left = is_sprite_facing_left(dlink_dup_1.sprite);
			} else if (which_dark_link_killed_link == 2) {
				//Then initial dlink killed link]
				dlink_left = is_sprite_facing_left(dlink_dup_2.sprite);
			}
			lives--; 
			delay(10000);
			if (dlink_left){
				link.x -= 12800;
				if (link.x < 0){
					link.x = 10*256;
				}
			}
			else {
				link.x += 12800;
				if (link.x+16 > (SCREEN_WIDTH*256)){
					link.x = (SCREEN_WIDTH*256)-(42*256);
				}
			}
		}
		
		//If score is >5 add another dark link
		if (score > 10 && !duplicate_one_alive){
			/* creates dark link */
			dlink_init(&dlink_dup_1, 10);
			duplicate_one_alive = 1;
		} 
		
		if (score > 15 && !duplicate_two_alive){
			dlink_init(&dlink_dup_2, 100);
			duplicate_two_alive = 1;
		}
		
		/* wait for vblank before scrolling */
		wait_vblank();
		*bg0_x_scroll = xscroll;
		*bg1_x_scroll = xscroll * 2;
		sprite_update_all();
		sprintf(msg, "Score = %d | Lives = %d", score, lives);
		set_text(msg,0,5);

		/* delay some */
		delay(230);
	}
}


/* the game boy advance uses "interrupts" to handle certain situations
 * for now we will ignore these */
void interrupt_ignore() {
    /* do nothing */
}


/* this table specifies which interrupts we handle which way
 * for now, we ignore all of them */
typedef void (*intrp)();
const intrp IntrTable[13] = {
    interrupt_ignore,   /* V Blank interrupt */
    interrupt_ignore,   /* H Blank interrupt */
    interrupt_ignore,   /* V Counter interrupt */
    interrupt_ignore,   /* Timer 0 interrupt */
    interrupt_ignore,   /* Timer 1 interrupt */
    interrupt_ignore,   /* Timer 2 interrupt */
    interrupt_ignore,   /* Timer 3 interrupt */
    interrupt_ignore,   /* Serial communication interrupt */
    interrupt_ignore,   /* DMA 0 interrupt */
    interrupt_ignore,   /* DMA 1 interrupt */
    interrupt_ignore,   /* DMA 2 interrupt */
    interrupt_ignore,   /* DMA 3 interrupt */
    interrupt_ignore,   /* Key interrupt */
};

