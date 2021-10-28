#ifndef __PORTENTA_SYSTEM_H
#define __PORTENTA_SYSTEM_H

#ifdef __cplusplus
 extern "C" {
#endif

int boot_set_pending(int permanent);
int boot_set_confirmed(void);
int confirmRunningSketch(void) {
    return boot_set_confirmed();
}

#ifdef __cplusplus
}
#endif

#endif /* __PORTENTA_SYSTEM_H */