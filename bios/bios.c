#include "bios.h"
#include "sd.h"
#include "io.h"
#include "serial.h"
#include "keyboard.h"

void *memset(void *s, int c, unsigned short n)
{
    unsigned char *p = s;
    unsigned short m;

    for (m = 0; m < n; ++m)
        p[m] = c;

    return s;
}

void panic(const char *p)
{
    putstr(p);
    putstr("\n\r\n\rHalted.\n\r");

    for (;;)
        continue;
}

void int10_function(struct callregs *regs)
{
    if (regs->ax.h == 0xe) {
        putchar(regs->ax.l);
        regs->flags &= ~CF;
    } else {
        regs->flags |= CF;
    }
}

#define INT11_DISKS_PRESENT (1 << 0)
#define INT11_MDA_ADAPTOR (3 << 4)

void int11_function(struct callregs *regs)
{
    regs->ax.x = INT11_DISKS_PRESENT | INT11_MDA_ADAPTOR;
}

// Serial services
void int14_function(struct callregs *regs)
{
    regs->flags |= CF;
}

static const unsigned char bios_params_rec[] = {
    8, 0, 0xff, 0, 0, 0, 0, 0
};

static void system_configuration_parameters(struct callregs *regs)
{
    regs->flags &= ~CF;
    regs->ax.h = 0x80;
    set_es(get_cs());
    regs->bx.x = (unsigned short)bios_params_rec;
}

static void wait_event(struct callregs *regs)
{
    regs->flags &= ~CF;
}

static void system_extended_memory_size(struct callregs *regs)
{
    regs->flags &= ~CF;
    regs->ax.x = 0;
}

static void a20_gate(struct callregs *regs)
{
    regs->flags |= CF;
}

void int15_function(struct callregs *regs)
{
    regs->flags |= CF;

    switch (regs->ax.h) {
    case 0xc0:
        system_configuration_parameters(regs);
        break;
    case 0x41:
        wait_event(regs);
        break;
    case 0x88:
        system_extended_memory_size(regs);
        break;
    case 0x24:
        a20_gate(regs);
        break;
    default:
        break;
    }
}

// Printer services
void int17_function(struct callregs *regs)
{
    regs->flags |= CF;
}

void int18_function(struct callregs *regs)
{
    (void)regs;
    panic("No basic services\n\r");
}

void int19_function(struct callregs *regs)
{
    (void)regs;
    asm volatile("jmp $0xffff, $0x0");
}

void int1a_function(struct callregs *regs)
{
    regs->flags |= CF;
}

void int1b_function(struct callregs *regs)
{
    (void)regs;
    panic("No break handler\n\r");
}

void int1c_function(struct callregs *regs)
{
    (void)regs;
    panic("No tick handler\n\r");
}

void set_vector(int vector, void (*handler)(void))
{
    writew(0, vector * 4, (unsigned short)handler);
    writew(0, vector * 4 + 2, get_cs());
}

extern void int10_handler(void);
extern void int11_handler(void);
extern void int12_handler(void);
extern void int13_handler(void);
extern void int14_handler(void);
extern void int15_handler(void);
extern void int16_handler(void);
extern void int17_handler(void);
extern void int18_handler(void);
extern void int19_handler(void);
extern void int1a_handler(void);
extern void int1b_handler(void);
extern void int1c_handler(void);

static void install_vectors(void)
{
    set_vector(0x10, int10_handler);
    set_vector(0x11, int11_handler);
    set_vector(0x12, int12_handler);
    set_vector(0x13, int13_handler);
    set_vector(0x14, int14_handler);
    set_vector(0x15, int15_handler);
    set_vector(0x16, int16_handler);
    set_vector(0x17, int17_handler);
    set_vector(0x18, int18_handler);
    set_vector(0x19, int19_handler);
    set_vector(0x1a, int1a_handler);
    set_vector(0x1b, int1b_handler);
    set_vector(0x1c, int1c_handler);
}

void root(void)
{
    putstr("s80x86 DE0-CV BIOS, (C) Jamie Iles 2017, " __DATE__ " " __TIME__ "\r\n");
    putstr("Platform: " __PLATFORM__ "\r\n");
    putstr("Build: " __BUILD__ "\r\n");
    putstr("\r\n");

    keyboard_init();
    install_vectors();
    sd_init();
    sd_boot();
}