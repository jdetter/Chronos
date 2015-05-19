#ifndef _ATA_READ_H_
#define _ATA_READ_H

void ata_wait();
int ata_readsect(uint sect, void* dst);

#endif
