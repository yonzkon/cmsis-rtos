#include <unistd.h>
#include <malloc.h>
#include <log.h>

void idle(void)
{
    LOG_INFO("start idle ...");

    while (1) {
        usleep(100 * 1000);
        struct mallinfo info = mallinfo();
        LOG_DEBUG("arena: %d, ordblks: %d, uordblks: %d, fordblks: %d, keepcost: %d",
                  info.arena, info.ordblks, info.uordblks, info.fordblks, info.keepcost);
    }

    LOG_INFO("exit idle ...");
}
