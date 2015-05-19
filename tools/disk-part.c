/* Partition a disk */

/**
 * Options:
 * -p<n> <l> <s>
 *     n: The partition number
 *     l: The logical location of the partition
 *     s: How many sectors are in the partition
 *
 *
 * example:
 *  disk-part -p1 64 4096 -p2 6000 1024
 *
 *  ^ This will create 2 partitions: one partition starting at sector 64 that
 *         contains 4096 sectors and one partition starting at sector 6000
 *         that contains 1024 sectors.   
 */

int main(int argc, char** argv)
{
	return 0;
}
