#if defined(TEST)
struct child_descriptor {
	int foo;
};
struct child_descriptor child_descriptor;
#endif


main (int argc, char **argv)
{
    --argc;
    if (argc) {
	printf("argc = %d\n", argc);
    } else {
	printf("argc = 0\n");
    }
    return 0;
}
