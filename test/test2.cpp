#include <xlog/xlog.h>

int test2()
{
    xlog::Logger log(u8"hi");
    log.Info("teapot)");


    return 0;
}