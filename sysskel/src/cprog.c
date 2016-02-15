int write(int, char*, int);

int _start(int argc, char** argv)
{
	char* message = "Hello, World!\n";
	write(1, message, 14);
	return 0;
}
