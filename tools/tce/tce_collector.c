#include <te_config.h>
#include <te_defs.h>
#include <logger_ta.h>

DEFINE_LGR_ENTITY("TCE collector");

extern int tce_init_tce_collector(int, char **);
extern int tce_collector(int, char **);
extern te_bool tce_standalone;

int main(int argc, char *argv[])
{
    tce_standalone = TRUE;
    tce_init_tce_collector(argc - 1, argv + 1);
    return tce_collector(0, NULL);
}
