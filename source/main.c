
#include "global.h"
#include "ov.h"

FS_archive sdmcArchive = { 0x9, (FS_path){ PATH_EMPTY, 1, (u8*)"" } };
Handle fsUserHandle = 0;

// Active monster information
typedef struct
{
  u8 unknown[0x1318];
  u32 hp;
  u32 hp_max;
} Monster;

static Handle ptmuHandle;

#define CALLBACK_OVERLAY (101)

extern u32 getCurrentProcessHandle();

extern void initSharedFunc();


#ifdef GAME_REGION_USA
#define MONSTER_ARRAY_ADDR 0x083343A4
#endif

#ifdef GAME_REGION_EUR
#define MONSTER_ARRAY_ADDR 0x08334984
#endif

#ifdef GAME_REGION_EUR
#define MONSTER_ARRAY_ADDR 0x08325244
#endif

// This is where the game stores the active monsters on the current zone.
static Monster** monster_array = (Monster**)MONSTER_ARRAY_ADDR;


Result ptmuInit(void)
{
  Result res = srv_getServiceHandle(NULL, &ptmuHandle, "ptm:u");
  return res;
}

void setMaxHP(u16* monster_hp_max,u16 monster_hp)
{
  if(monster_hp == 0 || monster_hp > *monster_hp_max)
  {
    *monster_hp_max = monster_hp;
  }
}

u32 drawMonsterHP(u32 addr, u32 stride, u32 format)
{
  Monster* monsters[8];
  u32 count = 0;

  // Pick large monsters  
  for(u32 i = 0; i < 8; i++)
  {
    Monster* m = monster_array[i];
    if(m && m->hp_max > 300 && m->hp > 0)
    {
       monsters[count++] = m;
    }
  }

  if(count > 0)
  {
    u32 width = 320 / count - 8;

    for(u32 i = 0; i < count; i++)
    {
      Monster* m = monsters[i];
      u32 x = (width + 4) * i + 4;
      u32 w = m->hp * width / m->hp_max;
      ovDrawRect(addr, stride, format, 3, x - 1, 12, width + 2, 255, 255, 255);
      ovDrawRect(addr, stride, format, 4, x, 10, width, 0, 0, 0);

      u8 r = 0;
      u8 g = 0;

      if( m->hp * 5 > m->hp_max) // 20% HP
      {
        g = 255;
      }
      if( m->hp * 10 < m->hp_max * 3) // 30% HP
      {
        r = 255;
      }


      ovDrawRect(addr, stride, format, 4, x, 10, w, r, g, 0);
    }
    return 0;
  }
  return 1;
}


/*
Overlay Callback.
isBottom: 1 for bottom screen, 0 for top screen.
addr: writable cached framebuffer virtual address, should flush data cache after modifying.
addrB: right-eye framebuffer for top screen, undefined for bottom screen.
stride: framebuffer stride(pitch) in bytes, at least 240*bytes_per_pixel.
format: framebuffer format, see https://www.3dbrew.org/wiki/GPU/External_Registers for details.

return 0 on success. return 1 when nothing in framebuffer was modified.
*/

u32 overlayCallback(u32 isBottom, u32 addr, u32 addrB, u32 stride, u32 format)
{
  if (isBottom == 1)
  {
     return drawMonsterHP( addr, stride, format);
  }
  return 1;
}

int main()
{
  initSharedFunc();
  initSrv();
  ptmuInit();
  plgRegisterCallback(CALLBACK_OVERLAY, (void*) overlayCallback, 0);
  return 0;
}

