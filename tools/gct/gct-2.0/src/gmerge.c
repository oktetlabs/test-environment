/*
 * Based on code Copyright (C) 1990 by Thomas Hoch and Wolfram Schafer.
 * Modified for GCT by Brian Marick.
 * You may distribute under the terms of the GNU General Public License.
 * See the file named COPYING.
 */


#include <stdio.h>
#include <string.h>

#define MAX_LOGFILES 100
#define PATH_BUF_LEN	1025  /* Because there's no portable system define. */
#define LINESIZE	1000	/* EASILY adequate. */

main(argc,argv)
     int argc;
     char *argv[];
{
  /* Notice, all arrays start at index 1 not 0 ! */

  char logfile_name[MAX_LOGFILES][PATH_BUF_LEN];	/* The logfiles to sum. */
  FILE *logfile[MAX_LOGFILES];			/* Open file descriptors. */
  int i;
  int read_num;					/* retval for scanf. */
  int log_num;					/* Number of logfiles. */
  unsigned long total_count, count;
  unsigned long number_eofs = 0;		/* Stop merging when hit
						   the end of all logfiles. */
  char timestamp[LINESIZE];	/* Expected timestamp for all files. */
  char buffer[LINESIZE];	/* fgets buffer. */
  

  if (argc > MAX_LOGFILES+1)
    {
      fprintf(stderr,"gmerge: Only %d logfiles can be merged at a time.\n",
	      MAX_LOGFILES);
      exit(1);
    }

  if (argc == 1)
    {
      fprintf(stderr,"Usage: gmerge logfile1 logfile2...\n");
      exit(1);
    }
  else
    for (i= 1; i < argc; i++) {
      if (argv[i][0] != '-') 
	{
	  strcpy(logfile_name[i],argv[i]);
	    logfile[i]= fopen(logfile_name[i],"r");
	  if (NULL== logfile[i])
	    {
	      fprintf(stderr,"gmerge: Can't open file %s\n",
		      logfile_name[i]);
	      exit(1);
	    }    
	  fgets(buffer, LINESIZE, logfile[i]);	/* Suck up header line. */
	  /* Error checks later - errors are sticky. */
	}
      else
	{
	  fprintf(stderr,"gmerge: Unknown argument %s\n",argv[i]);
	  exit(1);	 
	}
    }
      
  log_num = argc -1;

  /* Check consistency of all files */

  /* Get the timestamp for the first file */
  fgets(timestamp, LINESIZE, logfile[1]);
  if (ferror(logfile[1]) || feof(logfile[1]))
    {
      fprintf(stderr, "gmerge:  Can't read timestamp from %s.\n", logfile_name[1]);
      exit(1);
    }
      
  /* Check timestamps from remaining files. */
  for (i=2; i<=log_num; i++)
    {
      fgets(buffer, LINESIZE, logfile[i]);
      if (ferror(logfile[i]) || feof(logfile[i]))
	{
	  fprintf(stderr, "gmerge:  Can't read timestamp from %s.\n", logfile_name[i]);
	  exit(1);
	}
      if (0 != strcmp(buffer, timestamp))
	{
	  fprintf(stderr, "%s and %s come from two different instrumentations.\n",
		  logfile_name[1], logfile_name[i]);
	  exit(1);
	}
    }

  printf("GCT Log File (from gmerge)\n");
  printf("%s", timestamp);

  /*
   * Loop continues until an EOF has been seen on all streams.  As each
   * EOF is seen, the stream is set to NULL to mark it as finished.
   * No point in closing the streams.
   */
  for(number_eofs=0;;)
    {
      total_count = 0;

      for (i=1;i<=log_num;i++)
	{
	  if (NULL == logfile[i])	/* EOF already seen. */
	    continue;
	  
	  read_num = fscanf(logfile[i],"%lu\n", &count);
	  if (read_num != 1)
	    {
	      if (feof(logfile[i]))
		{
		  number_eofs++;
		  logfile[i] = NULL;
		  continue;
		}
	      else 
		{
		  fprintf(stderr, "gmerge:  Failed to read log line from %s.\n",
			  logfile_name[i]);
		  perror("Error");
		  exit(1);
		}
	    }
	  total_count += count;
	}
      if (number_eofs == log_num)
	break;
      fprintf(stdout, "%lu\n", total_count);
    }
  exit(0);
}


