
#include "global.h"
#include "ov.h"

FS_archive sdmcArchive = { 0x9, (FS_path){ PATH_EMPTY, 1, (u8*)"" } };
Handle fsUserHandle = 0;

// Monster part status
typedef struct {
    u16 broken; // 0 = not-broken, 1 = broken
    u16 stagger; // when this number reaches zero, the monster will stagger
    s16 cut; // when this number reaches zero the part will be cut off
} Part;


// Active monster information
typedef struct
{
  u8 unknown1[0x1318];
  u32 hp;
  u32 hp_max;
  u8 unknown2[0x3e];
  Part parts[8];
} Monster;

typedef struct
{
  Monster* monster;
  s16 max[8];
  u8 remove;
} PartMax;

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

#ifdef GAME_REGION_JPN
#define MONSTER_ARRAY_ADDR 0x08325244
#endif

// This is where the game stores the active monsters on the current zone.
static Monster** monster_array = (Monster**)MONSTER_ARRAY_ADDR;

// Max HP of individual monster parts.
static PartMax part_max[16];
static s32 part_max_count = 0;

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
    for(u32 i = 0; i < part_max_count;i++)
    {
      part_max[i].remove = 1;
    }

    // Initialize max monster part HP.
    for(u32 i = 0; i < count;i++)
    {
      u8 add = 1;
      for(u32 j = 0; j < part_max_count; j++)
      {
        if(monsters[i] == part_max[j].monster)
        {
          // Monster already on list.
          add = 0;
          part_max[j].remove = 0;
          break;
        }
      }

      // Add new monster to the list.
      if(add)
      {
        part_max[part_max_count].monster = monsters[i];
        part_max[part_max_count].remove = 0;
        for(u32 j = 0; j < 8; j++)
        {
          part_max[part_max_count].max[j] = monsters[i]->parts[j].cut;
        }
        part_max_count++;
      }
    }

    // Cleanup part HP of dead monsters.
    if(part_max_count > 0)
    {
      for(s32 i = part_max_count - 1; i >= 0 ;i--)
      {
        if(part_max[i].remove)
        {
          part_max[i] = part_max[--part_max_count];
        }
      }
    }

    u32 width = 320 / count - 8;

    for(u32 i = 0; i < count; i++)
    {
      Monster* m = monsters[i];
      u32 x = (width + 4) * i + 4;
      u32 w = m->hp * width / m->hp_max;
      ovDrawRect(addr, stride, format, 2, x - 1, 10, width + 2, 255, 255, 255);
      ovDrawRect(addr, stride, format, 3, x, 8, width, 0, 0, 0);

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

      ovDrawRect(addr, stride, format, 3, x, 8, w, r, g, 0);

      for(u32 j = 0; j < part_max_count; j++)
      {
        if(part_max[j].monster == m)
        {
          u8 parts[8];
          u32 part_count = 0;
          for(u32 k = 0; k < 8; k++)
          {
            if(m->parts[k].cut > 0 && part_max[j].max[k] > 0)
            {
               parts[part_count++] = k;
            }
          }
          if( part_count > 0 )
          {
            u32 pwidth = width / part_count - 8;

            for(u32 k = 0; k < part_count; k++)
            {
              s16 hp = m->parts[parts[k]].cut;
              s16 hp_max = part_max[j].max[parts[k]];
              u32 px = (pwidth + 4) * k + x + 4;
              u32 pw = hp * pwidth / hp_max;
              ovDrawRect(addr, stride, format, 11, px - 1, 6, pwidth + 2, 255, 255, 255);
              ovDrawRect(addr, stride, format, 12, px, 4, pwidth, 0, 0, 0);

              r = 0;
              g = 0;

              if( hp * 5 > hp_max) // 20% part HP
              {
                g = 255;
              }
              if( hp * 10 < hp_max * 3) // 30% part HP
              {
                r = 255;
              }

              ovDrawRect(addr, stride, format, 12, px, 4, pw, r, g, 0);
            }
          }
          break;
        }
      }
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

