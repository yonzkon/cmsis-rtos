#include <apix.h>
#include <apix-stm32.h>

static struct apix *bus;

void fenix_main(void (*init)(void))
{
    bus = apix_new();
    apix_enable_stm32(bus);

    init();
    apix_disable_stm32(bus);
    apix_destroy(bus);
}
