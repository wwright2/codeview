#include <stdio.h>
#include <string.h>

int main()
{
    int count = 0;
    char buf[1024];
    while (fgets (buf, sizeof buf, stdin) != 0)
    {
	if (count == 0)
	    printf ("%c ", 1);

	char *cp = strchr (buf, '\n');
	if (*cp) *cp = 0;
	printf ("%s\\\\n", buf);

	if (++count == 8)
	{
	    printf ("\n");
	    count = 0;
	}
    }
}