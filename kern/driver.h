#ifndef DRIVER_H
#define DRIVER_H

int handler_install(void (*tickback)(unsigned int),
                    void (*keybacksync)(int, int),
                    void (*keybackasync)(int, int));

#endif
