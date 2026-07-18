#ifndef MOCK_XDC_RUNTIME_ERROR_H
#define MOCK_XDC_RUNTIME_ERROR_H
/* Minimal stand-in for SYS/BIOS xdctools' xdc/runtime/Error.h -- only the
 * two symbols esm_iwr68xx.c actually uses (Error_Block, Error_init()).
 * There's no real xdctools install on a host test build; this exists so
 * esm_iwr68xx.c (which is inherently RTOS-coupled -- see its own file
 * header) can be linked against fakes instead of the real SYS/BIOS Hwi
 * dispatcher. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    int dummy;
} Error_Block;

void Error_init(Error_Block* eb);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* MOCK_XDC_RUNTIME_ERROR_H */
