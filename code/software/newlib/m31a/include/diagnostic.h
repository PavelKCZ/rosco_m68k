#ifndef M31_DIAGNOSTIC_H
#define M31_DIAGNOSTIC_H
/* Defined only in the opt-in diagnostic profile; no normal-mode code/state. */
#ifdef M31_DIAGNOSTIC
void m31_diag_mark(unsigned code);
void m31_diag_error(unsigned code);
#define M31_MARK(code) m31_diag_mark((unsigned)(code))
#define M31_ERROR(code) m31_diag_error((unsigned)(code))
#else
#define M31_MARK(code) ((void)0)
#define M31_ERROR(code) ((void)0)
#endif
#endif
