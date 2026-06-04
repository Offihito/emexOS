#pragma once

void cli(void);
void sti(void);
__attribute__((noreturn)) void chalt(void); // Fixed typo, should be chalt not halt
__attribute__((noreturn)) void idle(void);
void wfi(void);
void nop(void);