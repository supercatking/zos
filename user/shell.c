typedef unsigned int uintptr_t;
typedef unsigned int size_t;

#define SYS_WRITE 1u
#define SYS_EXIT 2u
#define SYS_READ 3u
#define SYS_OPEN 4u
#define SYS_CLOSE 5u
#define SYS_SLEEP 6u
#define SYS_KILL 7u
#define SYS_CREATE 8u
#define SYS_LIST 9u
#define SYS_UNLINK 10u
#define SYS_STAT 11u
#define SYS_MKDIR 12u
#define SYS_RENAME 13u
#define SYS_UPTIME 14u
#define SYS_MEMINFO 15u

#define MAX_LINE 96
#define MAX_ARGS 8

static long syscall3(uintptr_t number, uintptr_t a0, uintptr_t a1, uintptr_t a2)
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

static long sys_write(int fd, const char *buf, size_t len)
{
    return syscall3(SYS_WRITE, (uintptr_t)fd, (uintptr_t)buf, len);
}

static long sys_read(int fd, char *buf, size_t len)
{
    return syscall3(SYS_READ, (uintptr_t)fd, (uintptr_t)buf, len);
}

static long sys_open(const char *path)
{
    return syscall3(SYS_OPEN, (uintptr_t)path, 0, 0);
}

static long sys_close(int fd)
{
    return syscall3(SYS_CLOSE, (uintptr_t)fd, 0, 0);
}

static long sys_create(const char *path)
{
    return syscall3(SYS_CREATE, (uintptr_t)path, 0, 0);
}

static long sys_list(char *buf, size_t len)
{
    return syscall3(SYS_LIST, (uintptr_t)buf, len, 0);
}

static long sys_unlink(const char *path)
{
    return syscall3(SYS_UNLINK, (uintptr_t)path, 0, 0);
}

static long sys_stat(const char *path, char *buf, size_t len)
{
    return syscall3(SYS_STAT, (uintptr_t)path, (uintptr_t)buf, len);
}

static long sys_mkdir(const char *path)
{
    return syscall3(SYS_MKDIR, (uintptr_t)path, 0, 0);
}

static long sys_rename(const char *old_path, const char *new_path)
{
    return syscall3(SYS_RENAME, (uintptr_t)old_path, (uintptr_t)new_path, 0);
}

static long sys_uptime(void)
{
    return syscall3(SYS_UPTIME, 0, 0, 0);
}

static long sys_meminfo(char *buf, size_t len)
{
    return syscall3(SYS_MEMINFO, (uintptr_t)buf, len, 0);
}

static long sys_sleep(uintptr_t ticks)
{
    return syscall3(SYS_SLEEP, ticks, 0, 0);
}

static long sys_kill(uintptr_t pid)
{
    return syscall3(SYS_KILL, pid, 0, 0);
}

static void sys_exit(int status)
{
    (void)syscall3(SYS_EXIT, (uintptr_t)status, 0, 0);
    for (;;) {
    }
}

static size_t strlen(const char *s)
{
    size_t n = 0;
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

static int streq(const char *a, const char *b)
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

static void puts(const char *s)
{
    (void)sys_write(1, s, strlen(s));
}

static void put_uint(uintptr_t value)
{
    char tmp[10];
    uintptr_t n = 0;

    if (value == 0) {
        puts("0");
        return;
    }

    while (value != 0 && n < sizeof(tmp)) {
        tmp[n++] = (char)('0' + (value % 10u));
        value /= 10u;
    }
    while (n != 0) {
        char ch = tmp[--n];
        (void)sys_write(1, &ch, 1);
    }
}

static void parse_line(char *line, int *argc, char **argv)
{
    *argc = 0;

    for (char *p = line; *p != '\0';) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
            *p++ = '\0';
        }
        if (*p == '\0') {
            break;
        }
        if (*argc >= MAX_ARGS) {
            break;
        }
        argv[(*argc)++] = p;
        while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') {
            p++;
        }
    }
}

static int cmd_help(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    puts("commands: help echo ls cat ps sleep kill pwd clear reboot\n");
    return 0;
}

static int cmd_echo(int argc, char **argv)
{
    int redirect = -1;

    for (int i = 1; i < argc; i++) {
        if (streq(argv[i], ">")) {
            redirect = i;
            break;
        }
    }

    if (redirect >= 0) {
        if (redirect + 1 >= argc) {
            puts("echo: missing redirect target\n");
            return -1;
        }

        if (sys_create(argv[redirect + 1]) < 0) {
            puts("echo: create failed\n");
            return -1;
        }

        long fd = sys_open(argv[redirect + 1]);
        if (fd < 0) {
            puts("echo: open failed\n");
            return -1;
        }

        for (int i = 1; i < redirect; i++) {
            if (i > 1) {
                (void)sys_write((int)fd, " ", 1);
            }
            (void)sys_write((int)fd, argv[i], strlen(argv[i]));
        }
        (void)sys_write((int)fd, "\n", 1);
        (void)sys_close((int)fd);
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        if (i > 1) {
            puts(" ");
        }
        puts(argv[i]);
    }
    puts("\n");
    return 0;
}

static int cmd_ls(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    char buf[128];
    long n = sys_list(buf, sizeof(buf));
    if (n > 0) {
        (void)sys_write(1, buf, (size_t)n);
    }
    return 0;
}

static int cmd_cat(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "/README";
    char buf[96];
    long fd = sys_open(path);

    if (fd < 0) {
        puts("cat: not found\n");
        return -1;
    }

    for (;;) {
        long n = sys_read((int)fd, buf, sizeof(buf));
        if (n <= 0) {
            break;
        }
        (void)sys_write(1, buf, (size_t)n);
    }
    (void)sys_close((int)fd);
    return 0;
}

static int cmd_ps(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    char *cat_argv[] = {"cat", "/proc/status"};
    return cmd_cat(2, cat_argv);
}

static int cmd_sleep(int argc, char **argv)
{
    (void)argv;
    (void)sys_sleep(argc > 1 ? 10u : 5u);
    puts("sleep done\n");
    return 0;
}

static int cmd_kill(int argc, char **argv)
{
    (void)argv;
    (void)sys_kill(argc > 1 ? 1u : 1u);
    puts("kill: pid 1 noted\n");
    return 0;
}

static int cmd_pwd(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    puts("/\n");
    return 0;
}

static int cmd_touch(int argc, char **argv)
{
    if (argc < 2) {
        puts("touch: missing file\n");
        return -1;
    }
    if (sys_create(argv[1]) < 0) {
        puts("touch: failed\n");
        return -1;
    }
    return 0;
}

static int cmd_rm(int argc, char **argv)
{
    if (argc < 2) {
        puts("rm: missing file\n");
        return -1;
    }
    if (sys_unlink(argv[1]) < 0) {
        puts("rm: failed\n");
        return -1;
    }
    return 0;
}

static int cmd_mkdir(int argc, char **argv)
{
    if (argc < 2) {
        puts("mkdir: missing dir\n");
        return -1;
    }
    if (sys_mkdir(argv[1]) < 0) {
        puts("mkdir: failed\n");
        return -1;
    }
    return 0;
}

static int cmd_rmdir(int argc, char **argv)
{
    return cmd_rm(argc, argv);
}

static int cmd_stat(int argc, char **argv)
{
    char buf[96];

    if (argc < 2) {
        puts("stat: missing file\n");
        return -1;
    }

    long n = sys_stat(argv[1], buf, sizeof(buf));
    if (n < 0) {
        puts("stat: not found\n");
        return -1;
    }
    (void)sys_write(1, buf, (size_t)n);
    return 0;
}

static int cmd_cp(int argc, char **argv)
{
    char buf[96];

    if (argc < 3) {
        puts("cp: missing operand\n");
        return -1;
    }

    long src = sys_open(argv[1]);
    if (src < 0 || sys_create(argv[2]) < 0) {
        puts("cp: failed\n");
        return -1;
    }

    long dst = sys_open(argv[2]);
    if (dst < 0) {
        (void)sys_close((int)src);
        puts("cp: failed\n");
        return -1;
    }

    for (;;) {
        long n = sys_read((int)src, buf, sizeof(buf));
        if (n <= 0) {
            break;
        }
        (void)sys_write((int)dst, buf, (size_t)n);
    }

    (void)sys_close((int)src);
    (void)sys_close((int)dst);
    return 0;
}

static int cmd_mv(int argc, char **argv)
{
    if (argc < 3) {
        puts("mv: missing operand\n");
        return -1;
    }
    if (sys_rename(argv[1], argv[2]) < 0) {
        puts("mv: failed\n");
        return -1;
    }
    return 0;
}

static int cmd_uptime(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    puts("uptime ");
    put_uint((uintptr_t)sys_uptime());
    puts("s\n");
    return 0;
}

static int cmd_mem(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    char buf[96];
    long n = sys_meminfo(buf, sizeof(buf));
    if (n > 0) {
        (void)sys_write(1, buf, (size_t)n);
    }
    return 0;
}

static int cmd_uname(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    puts("ZOS riscv32\n");
    return 0;
}

static int cmd_clear(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    puts("\033[2J\033[H");
    return 0;
}

static int cmd_reboot(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    sys_exit(0);
    return 0;
}

struct command {
    const char *name;
    int (*handler)(int argc, char **argv);
};

static const struct command commands[] = {
    {"help", cmd_help},
    {"echo", cmd_echo},
    {"ls", cmd_ls},
    {"cat", cmd_cat},
    {"ps", cmd_ps},
    {"sleep", cmd_sleep},
    {"kill", cmd_kill},
    {"pwd", cmd_pwd},
    {"touch", cmd_touch},
    {"rm", cmd_rm},
    {"mkdir", cmd_mkdir},
    {"rmdir", cmd_rmdir},
    {"stat", cmd_stat},
    {"cp", cmd_cp},
    {"mv", cmd_mv},
    {"uptime", cmd_uptime},
    {"mem", cmd_mem},
    {"uname", cmd_uname},
    {"clear", cmd_clear},
    {"reboot", cmd_reboot},
};

__attribute__((section(".text.start"), used))
void _start(void)
{
    char line[MAX_LINE];
    char *argv[MAX_ARGS];
    int argc;

    for (;;) {
        puts("zos$ ");
        long n = sys_read(0, line, sizeof(line) - 1u);
        if (n <= 0) {
            continue;
        }
        line[n] = '\0';
        parse_line(line, &argc, argv);
        if (argc == 0) {
            continue;
        }

        int handled = 0;
        for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
            if (streq(argv[0], commands[i].name)) {
                (void)commands[i].handler(argc, argv);
                handled = 1;
                break;
            }
        }
        if (!handled) {
            puts("unknown command\n");
        }
    }
}
