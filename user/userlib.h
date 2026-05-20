#define U_UNUSED __attribute__((unused))

typedef unsigned int uintptr_t;
typedef unsigned int size_t;

#define SYS_WRITE 1u
#define SYS_EXIT 2u
#define SYS_READ 3u
#define SYS_OPEN 4u
#define SYS_CLOSE 5u
#define SYS_LIST 9u

static U_UNUSED long syscall3(uintptr_t number, uintptr_t a0, uintptr_t a1, uintptr_t a2)
{
    register uintptr_t r_a0 __asm__("a0") = a0;
    register uintptr_t r_a1 __asm__("a1") = a1;
    register uintptr_t r_a2 __asm__("a2") = a2;
    register uintptr_t r_a7 __asm__("a7") = number;

    __asm__ volatile("ecall"
                     : "+r"(r_a0)
                     : "r"(r_a1), "r"(r_a2), "r"(r_a7)
                     : "memory");
    return (long)r_a0;
}

static U_UNUSED size_t u_strlen(const char *s)
{
    size_t n = 0;
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

static U_UNUSED int u_streq(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

static U_UNUSED void u_write(const char *s)
{
    (void)syscall3(SYS_WRITE, 1, (uintptr_t)s, u_strlen(s));
}

static U_UNUSED void u_write_buf(const char *s, size_t len)
{
    (void)syscall3(SYS_WRITE, 1, (uintptr_t)s, len);
}

static U_UNUSED void u_exit(int status)
{
    (void)syscall3(SYS_EXIT, (uintptr_t)status, 0, 0);
    for (;;) {
    }
}

static U_UNUSED int u_open(const char *path)
{
    return (int)syscall3(SYS_OPEN, (uintptr_t)path, 0, 0);
}

static U_UNUSED long u_read(int fd, char *buf, size_t len)
{
    return syscall3(SYS_READ, (uintptr_t)fd, (uintptr_t)buf, len);
}

static U_UNUSED void u_close(int fd)
{
    (void)syscall3(SYS_CLOSE, (uintptr_t)fd, 0, 0);
}

static U_UNUSED long u_list(const char *path, char *buf, size_t len)
{
    return syscall3(SYS_LIST, (uintptr_t)buf, len, (uintptr_t)path);
}
