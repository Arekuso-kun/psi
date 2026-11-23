/* GNU/newlib hooks. The application uses UART queues, not stdio streams. */
#if defined(__GNUC__) && !defined(__ARMCC_VERSION)
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
extern char __heap_start, __heap_end;
void *_sbrk(ptrdiff_t amount) {
    static char *heap;
    uintptr_t next;
    char *previous;
    if (!heap) heap = &__heap_start;
    previous = heap;
    next = (uintptr_t)heap + amount;
    if (next < (uintptr_t)&__heap_start || next > (uintptr_t)&__heap_end) {
        errno = ENOMEM; return (void *)-1;
    }
    heap = (char *)next;
    return previous;
}
int _write(int fd, const void *buffer, size_t count) { (void)fd;(void)buffer;(void)count;errno=ENOSYS;return -1; }
int _read(int fd, void *buffer, size_t count) { (void)fd;(void)buffer;(void)count;errno=ENOSYS;return -1; }
int _close(int fd) { (void)fd;errno=EBADF;return -1; }
int _lseek(int fd,int position,int whence) { (void)fd;(void)position;(void)whence;errno=ENOSYS;return -1; }
int _fstat(int fd,struct stat *status) { (void)fd;status->st_mode=S_IFCHR;return 0; }
int _isatty(int fd) { (void)fd;return 1; }
int _getpid(void) { return 1; }
int _kill(int pid,int signal) { (void)pid;(void)signal;errno=EINVAL;return -1; }
void _exit(int status) { (void)status;for (;;) { } }
#endif
