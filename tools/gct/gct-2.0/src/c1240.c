/* @(#)File: c1240.c    Version: 1.1    Date: 3/26/85 */

                           /*c1240.c*/
/*======================================================================
	=================== TESTPLAN SEGMENT ===================
>KEYS:  < ANSI 6.4.2  K&R 9.7
>WHAT:  < Switch statement
>HOW:   <
>BUGS:  <
======================================================================*/

#include "testhead.h"        

/*--------------------------------------------------------------------*/

extern int local_flag ;   		

extern FILE *temp ;

char progname[] = "c1240()";

/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
main()					/***** BEGINNING OF MAIN. *****/
{
	char st[8];

	int i;
	int j;

	setup();
/*--------------------------------------------------------------------*/
block0:	blenter();			/*<<<<<ENTER DATA HERE<<<<<<<<*/


	for (i = 0; i < 12; i++) {
		switch (i) default: ;
		switch (i) case 2: case 4: case 5: 
			if (i != 2 && i != 4 && i != 5)
			{
				local_flag = FAILED;
				fprintf(temp,"i: %d !=2 && != 4",i); 
				fprintf(temp," && != 5\n");
			}
		switch (i) goto l10;
		switch (i) case 0: 
			if (i != 0)
			{
				local_flag = FAILED;
				ipostcall(i,0,"");
			}
		switch (i) case 1: 
			if (i != 1)
			{
				local_flag = FAILED;
				ipostcall(i,1,"");
			}
		switch (i) case 3: break;
		switch (i) case 6: continue;
		j = i;
		switch (i) {
			int i = s();
		l10:
			local_flag = FAILED;
			fprintf(temp,"label l10\n");
		l11:
			local_flag = FAILED;
			fprintf(temp,"label l11\n");
			break;
		case 7:
			if (j != 7)
			{
				local_flag = FAILED;
				ipostcall(j,7,"");
			}
		case 8:
			if (j != 8 && j != 7)
			{
				local_flag = FAILED;
				fprintf(temp,"case 8: i: %d\n",j);
			}
			break;
		default:
			;
		case 9:
			;
			continue;
		}
		switch (i) case 2: {
		case 1:
			local_flag = FAILED;
			fprintf(temp,"case inside case\n");
			break;
		case 3:
			local_flag = FAILED;
			fprintf(temp,"case inside case\n");
			break;
		case 5:
			local_flag = FAILED;
			fprintf(temp,"case inside case\n");
		}
		switch (i) case 10: 
		{
			local_flag = FAILED;
			fprintf(temp,"i: %d == 10\n",i);
		}
		switch (i) case 11: 
		{
			local_flag = FAILED;
			fprintf(temp,"i: %d == 11\n",i);
		}
	}

	blexit();
/*--------------------------------------------------------------------*/
block1: blenter();			/*<<<<<ENTER DATA HERE<<<<<<<<*/

	for (i = 0; i < 6; i++)
		switch (i) default:
			if (i < 3)
				case 5:  st[i] = 'o';
			else
				case 0: st[i] = 'p';

	st[6] = 0;
	st[7] = 0;
	st[8] = 0;
	if (!(streq(st,"pooppo")))
	{
		local_flag = FAILED;
		spostcall(st,"pooppo","");
	}

	blexit();
/*--------------------------------------------------------------------*/
	anyfail();	
}					/******** END OF MAIN. ********/
/*--------------------------------------------------------------------*/
					/*<<<<<FUNCTIONS GO HERE<<<<<<*/

s()
{
	local_flag = FAILED;
	fprintf(temp,"s() was called\n");
}

streq(s, t)
char *s, *t;
{
	while (*s && *s == *t) {
		s++, t++;
	}
	return *s == *t;
}
/*--------------------------------------------------------------------*/
