extern void sys_init(void);
extern void init(void);

void fenix_main(void)
{
    sys_init();
    init();
}
