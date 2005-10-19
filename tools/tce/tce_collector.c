#include <te_config.h>
#include <te_defs.h>
#include <logger_ta.h>
#include <tce_collector.h>
#include "logger_defs.h"

DEFINE_LGR_ENTITY("TCE collector");

int main(int argc, char *argv[])
{
    tce_standalone = TRUE;
    tce_init_collector(argc - 1, argv + 1);
    return tce_collector();
}
