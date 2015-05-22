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

#define ATA_ERR 0x0
#define ATA_DRQ 0x1 << 3
#define ATA_SRV 0x1 << 4
#define ATA_DF 0x1 << 5
#define ATA_RDY 0x1 << 6
#define ATA_BSY 0x1 << 7

int ata_init()
{
  outb(PRIMARY_ATA_COMMAND, 0x4); 

}


int ata_wait()
{
  uchar status;

  do {
    status = inb(PRIMARY_ATA_COMMAND);
  } while(status & ATA_BSY);

  do {
    status = inb(PRIMARY_ATA_COMMAND);
  } while(!((status & ATA_DRQ) || (status & ATA_ERR) || (status & ATA_DF)));

  if(status & ATA_ERR){
    while(1);
  }
  else if(status & ATA_DF){
    while(1);
  }
  else{
    return 0;
  }

}

int ata_readsect(uint sect, char* dst)
{
  outb(PRIMARY_ATA_DRIVE, 0xE0 | ((sect >> 24) & 0x0F));
  outb(PRIMARY_ATA_FEATURES_ERROR, 0x00);
  outb(PRIMARY_ATA_SECTOR_COUNT, 0x1);
  outb(PRIMARY_ATA_SECTOR_NUMBER, sect);
  outb(PRIMARY_ATA_CYLINDER_LOW, sect >> 8);
  outb(PRIMARY_ATA_CLYDINER_HIGH, sect >> 16);
  outb(PRIMARY_ATA_COMMAND, 0x20);

  ata_wait();

  dst = (ushort *) dst;
  int i;
  int word;
  for(i = 0; i < 256; i++){
    word = inw(PRIMARY_ATA_DATA);
    dst[i] = word;  
  }  

  return 0;
}

int ata_writesect(uint sect, char* src)
{
  outb(PRIMARY_ATA_DRIVE, 0xE0 | ((sect >> 24) & 0x0F));
  outb(PRIMARY_ATA_FEATURES_ERROR, 0x00);
  outb(PRIMARY_ATA_SECTOR_COUNT, 0x1);
  outb(PRIMARY_ATA_SECTOR_NUMBER, sect);
  outb(PRIMARY_ATA_CYLINDER_LOW, sect >> 8);
  outb(PRIMARY_ATA_CLYDINER_HIGH, sect >> 16);
  outb(PRIMARY_ATA_COMMAND, 0x30);

  ata_wait();

  src = (ushort *) src;

  int i;
  
  for(i = 0; i < 256; i++){
    outw(PRIMARY_ATA_DATA, src[i]);
  }  

  return 0;
}
