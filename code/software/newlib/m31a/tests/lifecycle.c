/* Native-only driver of the production C lifecycle. Not a 68010 emulator.
 * Fake Newlib array/exit functions expose ordering; no physical addresses used. */
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
static jmp_buf done;
static char events[512];
static int mode, status, callbacks_count;
static void (*callbacks[8])(void);
static void event(const char *s) {
    if (events[0]) strcat(events, ",");
    strcat(events,s);
}
#define main m31_test_main
#define atexit m31_test_atexit
#define exit m31_test_exit
#define _exit m31_test_low_exit
#define _init m31_test_init
#define _fini m31_test_fini
#define m31_console_init m31_test_console
#define m31_heap_init m31_test_heap
#define __libc_init_array m31_test_arrays_init
#define __libc_fini_array m31_test_arrays_fini
#include "../runtime.c"
#undef main
#undef atexit
#undef exit
#undef _exit
#undef _init
#undef _fini
#undef m31_console_init
#undef m31_heap_init
#undef __libc_init_array
#undef __libc_fini_array
static void ctor_low(void) { event("ctor-low"); }
static void ctor_high(void) { event("ctor-high"); }
static void dtor_low(void) { event("dtor-low"); }
static void dtor_high(void) { event("dtor-high"); }
void (*_ctors_start[])(void) = {ctor_low,ctor_high};
void (*_dtors_start[])(void) = {dtor_low,dtor_high};
#define STRING0(x) #x
#define STRING(x) STRING0(x)
__asm__(".global _ctors_end\n.set _ctors_end,_ctors_start+2*" STRING(__SIZEOF_POINTER__) "\n"
        ".global _dtors_end\n.set _dtors_end,_dtors_start+2*" STRING(__SIZEOF_POINTER__) "\n");
unsigned char _image_size[1]; /* metadata entry is linked but never called */
void m31_test_heap(const struct m31_layout *x) { (void)x; event("heap"); }
int m31_test_console(void) { event("console"); return mode==1 ? -1 : 0; }
int m31_test_atexit(void (*f)(void)) {
    event("register");
    if (mode==2) return -1;
    callbacks[callbacks_count++]=f;
    return 0;
}
void m31_test_arrays_init(void) { event("arrays-init"); }
void m31_test_arrays_fini(void) { event("arrays-fini"); }
static void app_callback(void) { event("app-atexit"); }
int m31_test_main(void) {
    event("main");
    m31_test_atexit(app_callback);
    event("main-return");
    return 17;
}
void m31_test_low_exit(int n) {
    status=n;event("low-exit");longjmp(done,1);
}
void m31_test_exit(int n) {
    event("exit");
    while (callbacks_count) callbacks[--callbacks_count]();
    m31_test_low_exit(n);
}
int main(int argc, char **argv) {
    mode=argc>1 ? argv[1][0]-'0' : 0;
    if (!setjmp(done)) m31_run_application();
    printf("{\"status\":%d,\"events\":\"%s\"}\n",status,events);
    return 0;
}

#ifdef M31_DIAGNOSTIC
void m31_diag_mark(unsigned code) {
    char text[]="diag-X";text[5]=(char)code;event(text);
}
void m31_diag_error(unsigned code) {
    char text[24];snprintf(text,sizeof(text),"error-%u",code);event(text);
}
#endif
