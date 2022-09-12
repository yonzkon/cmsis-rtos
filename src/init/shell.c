#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <log.h>

static int fd_shell;

static void cmd_handler(char *cmd)
{
    if (memcmp(cmd, "set", 3) == 0) {
        char level[12] = {0};
        sscanf(cmd, "set log=%s", level);
        if (strcasecmp(level, "debug") == 0)
            log_set_level(LOG_LV_DEBUG);
        else if (strcasecmp(level, "info") == 0)
            log_set_level(LOG_LV_INFO);
    }
}

int shell_init(void)
{
    fd_shell = open("/dev/ttyS1", 0);
    return 0;
}

int shell_fini(void)
{
    return 0;
}

void shell_loop(void)
{
    char cmd[64] = {0};

    int nread = read(fd_shell, (uint8_t *)cmd, sizeof(cmd) - 1);
    assert(nread >= 0);
    if (nread == 0) return;
    cmd_handler(cmd);
}

void shell_main(void)
{
    shell_init();
    LOG_INFO("start shell ...");

    while (1) {
        shell_loop();
        usleep(1000 * 1000);

        if (time(0) % 5 == 0) {
             struct mallinfo info = mallinfo();
             LOG_DEBUG("arena: %d, ordblks: %d, uordblks: %d, fordblks: %d, keepcost: %d",
                       info.arena, info.ordblks, info.uordblks, info.fordblks, info.keepcost);
        }
    }

    LOG_INFO("exit shell ...");
    shell_fini();
}
