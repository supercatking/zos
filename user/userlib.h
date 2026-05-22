#define U_UNUSED __attribute__((unused))

typedef unsigned int uintptr_t;
typedef unsigned int size_t;

#define SYS_WRITE 1u
#define SYS_EXIT 2u
#define SYS_READ 3u
#define SYS_OPEN 4u
#define SYS_CLOSE 5u
#define SYS_CREATE 8u
#define SYS_LIST 9u
#define SYS_STAT 11u
#define SYS_EXEC 16u
#define SYS_FORK 17u
#define SYS_WAIT 18u
#define SYS_GETPID 19u
#define SYS_PROCINFO 20u

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

static U_UNUSED int u_starts_with(const char *s, const char *prefix)
{
    while (*prefix != '\0') {
        if (*s != *prefix) {
            return 0;
        }
        s++;
        prefix++;
    }
    return 1;
}

static U_UNUSED void u_copy_string(char *dst, const char *src, size_t max_len)
{
    size_t i = 0;

    if (max_len == 0) {
        return;
    }

    while (i + 1u < max_len && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static U_UNUSED void u_memset(void *ptr, int value, size_t len)
{
    unsigned char *p = (unsigned char *)ptr;

    for (size_t i = 0; i < len; i++) {
        p[i] = (unsigned char)value;
    }
}

static U_UNUSED void u_write(const char *s)
{
    (void)syscall3(SYS_WRITE, 1, (uintptr_t)s, u_strlen(s));
}

static U_UNUSED long u_write_fd(int fd, const char *s, size_t len)
{
    return syscall3(SYS_WRITE, (uintptr_t)fd, (uintptr_t)s, len);
}

static U_UNUSED void u_write_buf(const char *s, size_t len)
{
    (void)syscall3(SYS_WRITE, 1, (uintptr_t)s, len);
}

static U_UNUSED void u_put_uint(uintptr_t value)
{
    char tmp[10];
    uintptr_t n = 0;

    if (value == 0) {
        u_write("0");
        return;
    }

    while (value != 0 && n < sizeof(tmp)) {
        tmp[n++] = (char)('0' + value % 10u);
        value /= 10u;
    }
    while (n != 0) {
        char ch = tmp[--n];
        u_write_buf(&ch, 1);
    }
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

static U_UNUSED int u_create(const char *path)
{
    return (int)syscall3(SYS_CREATE, (uintptr_t)path, 0, 0);
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

static U_UNUSED long u_stat(const char *path, char *buf, size_t len)
{
    return syscall3(SYS_STAT, (uintptr_t)path, (uintptr_t)buf, len);
}

static U_UNUSED int u_copy_file_to_stdout(const char *path)
{
    char buf[128];
    int fd = u_open(path);

    if (fd < 0) {
        return -1;
    }
    for (;;) {
        long n = u_read(fd, buf, sizeof(buf));
        if (n <= 0) {
            break;
        }
        u_write_buf(buf, (size_t)n);
    }
    u_close(fd);
    return 0;
}

static U_UNUSED long u_fork(void)
{
    return syscall3(SYS_FORK, 0, 0, 0);
}

static U_UNUSED long u_wait(void)
{
    return syscall3(SYS_WAIT, 0, 0, 0);
}

static U_UNUSED long u_getpid(void)
{
    return syscall3(SYS_GETPID, 0, 0, 0);
}

static U_UNUSED void u_spin(unsigned int count)
{
    for (volatile unsigned int i = 0; i < count; i++) {
    }
}
