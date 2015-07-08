/* Partition a disk */

/**
 * Options:
 * -p <n>,<l>,<s>,<t>
 *     n: The partition number (integer)
 *     l: The logical start of the partition (integer)
 *     s: How many sectors are in the partition (integer)
 *     t: The type of the partition (integer)
 *
 *
 * example:
 *  disk-part -p 1,64,4096,7 -p 2,6000,1024,6 disk.img
 *
 *  ^ This will create 2 partitions: one partition starting at sector 64 that
 *         contains 4096 sectors with type 7 (NTFS) and one partition starting 
 * 	   at sector 6000 that contains 1024 sectors. With type 6 (FAT16)
 *	   and it will use disk.img as the disk to configure.
 *
 * NOTE: use classical generic MBR formatting:
 * - https://en.wikipedia.org/wiki/Master_boot_record#Sector_layout
 *
 * NOTE: if a partition is not specified, its entry  should be 
 * cleared (with all 0s).
 *
 */

int main(int argc, char** argv)
{
	return 0;
}
