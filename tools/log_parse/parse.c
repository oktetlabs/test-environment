/**
 * Test Environment.
 *
 * Parsers parses text log file and in all lines like
 * 
 * WARN  Tester  Parser  16:34:33 958 ms
 *
 * adds the time from the beginning of the log in the end of the line.
 *
 * WARN  Tester  Parser  16:34:33 958 ms                           5 s 715 ms
 *
 * Author: Konstantin Ushakov <Konstantin.Ushakov@oktetlabs.ru>
 */
#include    <stdio.h>
#include    <stdlib.h>

#define LINE_LENGTH 1024

int
gettime(char *s, int *time)
{
    char *c;
    char *tmp = strdup((const char *)s);

    if (tmp == NULL)
        return -1;
    
    *time = 0;
    c = strchr(tmp, ':');
    *c = 0;
    *(c + 3) = 0;
    *(c + 6) = 0;
    *(c + 10) = 0;
    c -= 2;

    /* Now c is at the beginning of the xx:xx:xx xxx ms */
    *time += 1000 * 3600 * atoi(c);
    *time += 60 * 1000 * atoi(c + 3);
    *time += 1000 * atoi(c + 6);
    *time += atoi(c + 9);

    free(tmp);
    return 0;   
}

int
main(int argc, char *argv[])
{
    FILE *parse_file;
    FILE *output_file;
    char  line[LINE_LENGTH];
    char *c = NULL;
    int   i = 1;
    int   first_ts;
    int   curr_ts;
    int   rc;
    
    memset(line, 0, sizeof(line));
    
    if (argc < 3)
    {
        printf("To few arguments\n");
        return -1;
    }
    
    parse_file = fopen(argv[1], "r");
    if (parse_file == NULL)
    {
        printf("Wrong input file name");
        return -1;
    }
    output_file = fopen(argv[2], "w");
    if (output_file == NULL)
    {
        printf("Wrong output file name");
        fclose(parse_file);
        return -1;
    }

    /* Get the first timestamp */
    c = fgets(line, LINE_LENGTH, parse_file);
    if (c == NULL)
        return -1;
    fprintf(output_file, "%s", line);
    
    c = fgets(line, LINE_LENGTH, parse_file);
    if (c == NULL)
        return -1;
    
    if (gettime(line, &first_ts) != 0)
        return -1;    
    fprintf(output_file, "%s", line);

    c = fgets(line, LINE_LENGTH, parse_file);
    if (c == NULL)
        return -1;
    fprintf(output_file, "%s", line);
    
    do
    {
        c = fgets(line, LINE_LENGTH, parse_file);
        if (c == NULL)
            break;

        if (strstr(line, " ms") == NULL)
        {
            fprintf(output_file, "%s", line);
        }
        else
        {
            if (gettime(line, &curr_ts) != 0)
                return -1;
            c = strchr(line, '\n');
            *c = 0;
            snprintf(c, 60 - strlen(line), "%s",
                     "                                                     ");
            fprintf(output_file, "%s%s%i%s%i%s", line, "     ", 
                    (curr_ts - first_ts) / 1000, ".",
                    (curr_ts - first_ts) % 1000, "\n");
        }
    } while(1);
        
    fclose(parse_file);
    fclose(output_file);
}
