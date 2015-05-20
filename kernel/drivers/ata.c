#include "types.h"
#include "ata.h"
#include "x86.h"

#define PRIMARY_ATA_DATA 0x1F0
#define PRIMARY_ATA_FEATURES_ERROR 0x1F1
#define PRIMARY_ATA_SECTOR_COUNT 0x1F2
#define PRIMARY_ATA_SECTOR_NUMBER 0x1F3
#define PRIMARY_ATA_CYLINDER_LOW 0x1F4
#define PRIMARY_ATA_CYLINDER_HIGH 0x1F5
#define PRIMARY_ATA_DRIVE 0x1F6
#define PRIMARY_ATA_COMMAND 0x1F7

#define SECTSIZE 512
#define ATA_ERR 0x0
#define ATA_DRQ (0x1 << 3)
#define ATA_SRV (0x1 << 4)
#define ATA_DF (0x1 << 5)
#define ATA_RDY (0x1 << 6)
#define ATA_BSY (0x1 << 7)

void ata_wait()
{
        while((inb(PRIMARY_ATA_COMMAND) & (ATA_RDY | ATA_BSY)) != ATA_RDY);
}

int ata_readsect(uint sect, void* dst)
{
        ata_wait();
        outb(PRIMARY_ATA_SECTOR_COUNT, 0x1);
        outb(PRIMARY_ATA_SECTOR_NUMBER, sect);
        outb(PRIMARY_ATA_CYLINDER_LOW, sect >> 8);
        outb(PRIMARY_ATA_CYLINDER_HIGH, sect >> 16);
        outb(PRIMARY_ATA_CYLINDER_HIGH, (sect >> 24) & 0xE0);
        outb(PRIMARY_ATA_COMMAND, 0x20);

        ata_wait();
        insl(PRIMARY_ATA_DATA, dst, SECTSIZE/4);

        return 0;
}

int ata_writesect(uint sect, void* src)
{
  outb(PRIMARY_ATA_SECTOR_COUNT, 0x1);
  outb(PRIMARY_ATA_SECTOR_NUMBER, sect);
  outb(PRIMARY_ATA_CYLINDER_LOW, sect >> 8);
  outb(PRIMARY_ATA_CYLINDER_HIGH, sect >> 16);
  outb(PRIMARY_ATA_CYLINDER_HIGH, (sect >> 24) & 0xE0);
  outb(PRIMARY_ATA_COMMAND, 0x30);

  ata_wait();

  uint* srcw = (uint*)src;

  outsl(PRIMARY_ATA_DATA, srcw, 512/4);


  outb(PRIMARY_ATA_COMMAND, 0xE7);
  ata_wait();

  return 0;
}
