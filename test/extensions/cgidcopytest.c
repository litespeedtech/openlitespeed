/* Run as root in a disposable container; fixtures stay in a private /tmp dir.
 * Build from the repository root:
 * cc -ffunction-sections -fdata-sections -Isrc/extensions/cgi
 *    test/extensions/cgidcopytest.c src/extensions/cgi/rootcheck.c
 *    src/extensions/cgi/use_bwrap.c src/extensions/cgi/nsopts.c
 *           -Wl,--gc-sections -Wl,--wrap=malloc -Wl,--wrap=getpwuid -Wl,--wrap=stat
 *           -o /tmp/test_copy_source
 * No bubblewrap process or CGI is executed by this test.
 */
#define _GNU_SOURCE
#include "rootcheck.h"
#include "use_bwrap.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Exercise the actual static namespace copy preparation without mounting. */
#include "../../src/extensions/cgi/ns.c"

static int failures;
static int checks;
static int advisory_warnings;
static int skipped_copies;
static int stderr_lines;
static int error_callbacks;
static const char contents[] = "host=smtp.example.invalid\n";
static off_t expected_size = sizeof(contents) - 1;
static int inject_malloc_call;
static int inject_passwd_failure;
static int malloc_fail_after;
static int passwd_fail;
static int injected_failures;
static uid_t request_uid = 12345;

/* Compile-only discovery fixture; bubblewrap itself is never executed. */
int __real_stat(const char *path, struct stat *st);
int __wrap_stat(const char *path, struct stat *st)
{
    if (!strcmp(path, BWRAP_DEFAULT_BIN) || !strcmp(path, BWRAP_DEFAULT_BIN2))
    {
        memset(st, 0, sizeof(*st));
        st->st_mode = S_IFREG | 0755;
        return 0;
    }
    return __real_stat(path, st);
}

void *__real_malloc(size_t size);
void *__wrap_malloc(size_t size)
{
    if (malloc_fail_after && --malloc_fail_after == 0)
    {
        ++injected_failures;
        errno = ENOMEM;
        return NULL;
    }
    return __real_malloc(size);
}

struct passwd *__real_getpwuid(uid_t uid);
struct passwd *__wrap_getpwuid(uid_t uid)
{
    if (passwd_fail)
    {
        ++injected_failures;
        errno = ENOMEM;
        return NULL;
    }
    return __real_getpwuid(uid);
}

/* use_bwrap.c dependencies; the test only builds its argv and reads copy fds. */
int apply_rlimits_uid_chroot_stderr(lscgid_t *cgi) { (void)cgi; abort(); }
void ls_stderr(const char *fmt, ...)
{
    ++stderr_lines;
    if (strstr(fmt, "continuing copy"))
        ++advisory_warnings;
    if (strstr(fmt, "continuing CGI startup"))
        ++skipped_copies;
}
/* Stands in for lscgid's set_cgi_error(), which records the client error. */
static void error_callback(char *operation, char *path)
{ (void)operation; (void)path; ++error_callbacks; }

static void must(int ok, const char *operation)
{
    if (!ok)
    {
        perror(operation);
        exit(2);
    }
}

static void expect(int ok, const char *name)
{
    ++checks;
    printf("%s %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok)
        ++failures;
}

static void policy(const char *name, int (*check)(char *), const char *path,
                   int expected)
{
    char *copy = strdup(path);
    must(copy != NULL, "strdup");
    int rc = check(copy);
    expect(rc == expected && strcmp(copy, path) == 0, name);
    free(copy);
}

static int fd_count(void)
{
    int count = 0;
    for (int fd = 0; fd < 1024; ++fd)
        if (fcntl(fd, F_GETFD) != -1)
            ++count;
    return count;
}

/* Reads a copy fd to EOF and compares it with the fixture: the contents
 * prefix followed by zero fill up to expected_size. */
static int fd_has_contents(int fd)
{
    char buffer[4096];
    off_t total = 0;
    ssize_t n;
    while ((n = read(fd, buffer, sizeof(buffer))) > 0)
    {
        if (total == 0 && (n < (ssize_t)sizeof(contents) - 1
                           || memcmp(buffer, contents, sizeof(contents) - 1)))
            return 0;
        total += n;
    }
    return n == 0 && total == expected_size;
}

static int build(const char *copy, int *argc, char ***argv, int *done,
                 bwrap_mem_t **mem)
{
    static char *cgi_argv[] = {"/usr/bin/true", NULL};
    char command[1024];
    int len = snprintf(command, sizeof(command),
                       "bwrap --ro-bind /usr /usr '%s' --dir /after", copy);
    must(len > 0 && len < (int)sizeof(command), "command length");
    char command_env[1100];
    snprintf(command_env, sizeof(command_env), "LS_BWRAP_CMDLINE=%s", command);
    char *cgi_env[] = {command_env, NULL};
    lscgid_t cgi = {0};
    cgi.m_argv = cgi_argv;
    cgi.m_env = cgi_env;
    cgi.m_data.m_uid = request_uid;
    cgi.m_data.m_gid = 12345;
    malloc_fail_after = inject_malloc_call;
    passwd_fail = inject_passwd_failure;
    int rc = build_bwrap_exec(&cgi, error_callback, argc, argv, done, mem);
    malloc_fail_after = passwd_fail = 0;
    return rc;
}

/* expected: 0 = copied, 1 = skipped; neither aborts CGI preparation nor
 * records a request error. */
static void bwrap_copy(const char *path, int optional, int expected,
                       const char *name)
{
    int before = fd_count();
    int errors = error_callbacks;
    char copy[512];
    int len = snprintf(copy, sizeof(copy), "$COPY%s %s /config",
                       optional ? "-TRY" : "", path);
    must(len > 0 && len < (int)sizeof(copy), "copy length");
    int argc = 0, done = 0;
    char **argv = NULL;
    bwrap_mem_t *mem = NULL;
    int rc = build(copy, &argc, &argv, &done, &mem);
    int ok;
    {
        ok = rc == 0 && done == 0 && argc == (expected ? 7 : 10);
        if (ok)
        {
            ok = strcmp(argv[1], "--ro-bind") == 0
                 && strcmp(argv[2], "/usr") == 0
                 && strcmp(argv[3], "/usr") == 0
                 && strcmp(argv[argc - 3], "--dir") == 0
                 && strcmp(argv[argc - 2], "/after") == 0
                 && strcmp(argv[argc - 1], "/usr/bin/true") == 0
                 && argv[argc] == NULL;
        }
        if (ok && !expected)
        {
            ok = strcmp(argv[4], "--file") == 0
                 && strcmp(argv[6], "/config") == 0
                 && fd_has_contents(atoi(argv[5]));
        }
    }
    bwrap_free(&mem);
    expect(ok && fd_count() == before && error_callbacks == errors, name);
}

/* Fail allocations only during preparation, after command initialization. */
static void bwrap_copy_allocation_failure(const char *copy, const char *name)
{
    int before = fd_count();
    int errors = error_callbacks;
    int warnings = skipped_copies;
    int injected = injected_failures;
    int argc = 0, done = 0;
    char **argv = NULL;
    bwrap_mem_t *mem = NULL;
    int rc = build(copy, &argc, &argv, &done, &mem);
    int ok = rc == 0 && done == 0 && argc == 7
             && strcmp(argv[1], "--ro-bind") == 0
             && strcmp(argv[2], "/usr") == 0
             && strcmp(argv[3], "/usr") == 0
             && strcmp(argv[4], "--dir") == 0
             && strcmp(argv[5], "/after") == 0
             && strcmp(argv[6], "/usr/bin/true") == 0 && argv[7] == NULL;
    bwrap_free(&mem);
    expect(ok && fd_count() == before && error_callbacks == errors
           && skipped_copies == warnings + 1 && injected_failures == injected + 1,
           name);
}

/* Configuration and expansion errors still fail bwrap preparation. */
static void bwrap_config_error(const char *copy, const char *name)
{
    int before = fd_count();
    int errors = error_callbacks;
    int argc = 0, done = 0;
    char **argv = NULL;
    bwrap_mem_t *mem = NULL;
    int rc = build(copy, &argc, &argv, &done, &mem);
    int ok = rc != 0 && done == 1 && argv == NULL && mem == NULL;
    bwrap_free(&mem);
    expect(ok && fd_count() == before && error_callbacks > errors, name);
}

static void namespace_copy(const char *path, int flags, int expected,
                           const char *name)
{
    int before = fd_count();
    int errors = error_callbacks;
    char *source = strdup(path);
    must(source != NULL, "strdup namespace source");
    SetupOp op = {0};
    op.type = SETUP_COPY;
    op.source = source;
    op.dest = "/config";
    op.flags = flags;
    op.fd = -1;
    int rc = setup_copy(NULL, &op, 0);
    int ok = expected == 1 ? rc == 0 && op.type == SETUP_NOOP && op.fd == -1
                           : rc == 0 && op.type == SETUP_COPY && op.fd >= 0;
    if (ok && expected == 0)
        ok = fd_has_contents(op.fd);
    if (op.fd >= 0)
        close(op.fd);
    free(source);
    expect(ok && fd_count() == before && error_callbacks == errors, name);
}

static void copy_failure_cases(const char *path, const char *reason)
{
    char name[128];
    int warnings_before = skipped_copies;
    snprintf(name, sizeof(name), "COPY-TRY skips %s and preserves remaining argv", reason);
    bwrap_copy(path, 1, 1, name);
    snprintf(name, sizeof(name), "COPY skips %s and preserves remaining argv", reason);
    bwrap_copy(path, 0, 1, name);
    snprintf(name, sizeof(name), "optional namespace copy skips %s", reason);
    namespace_copy(path, OP_FLAG_ALLOW_NOTEXIST, 1, name);
    snprintf(name, sizeof(name), "namespace copy skips %s", reason);
    namespace_copy(path, 0, 1, name);
    expect(skipped_copies > warnings_before, "copy failures explain that startup continues");
}

static void copy_success_cases(const char *path, const char *reason)
{
    char name[128];
    snprintf(name, sizeof(name), "COPY copies %s", reason);
    bwrap_copy(path, 0, 0, name);
    snprintf(name, sizeof(name), "COPY-TRY copies %s", reason);
    bwrap_copy(path, 1, 0, name);
    snprintf(name, sizeof(name), "namespace copies %s", reason);
    namespace_copy(path, 0, 0, name);
    snprintf(name, sizeof(name), "optional namespace copies %s", reason);
    namespace_copy(path, OP_FLAG_ALLOW_NOTEXIST, 0, name);
}

static void copy_warning_cases(const char *path, const char *reason)
{
    int before = advisory_warnings;
    char name[128];
    snprintf(name, sizeof(name), "COPY still copies despite %s", reason);
    bwrap_copy(path, 0, 0, name);
    snprintf(name, sizeof(name), "COPY-TRY still copies despite %s", reason);
    bwrap_copy(path, 1, 0, name);
    snprintf(name, sizeof(name), "namespace still copies despite %s", reason);
    namespace_copy(path, 0, 0, name);
    snprintf(name, sizeof(name), "optional namespace still copies despite %s", reason);
    namespace_copy(path, OP_FLAG_ALLOW_NOTEXIST, 0, name);
    expect(advisory_warnings == before + 4, "policy warnings explain that copy continues");
}

int main(void)
{
    if (geteuid() != 0)
    {
        fprintf(stderr, "Run in a disposable container as root.\n");
        return 77;
    }
    char directory[] = "/tmp/lscgid-copy-source.XXXXXX";
    must(mkdtemp(directory) != NULL, "mkdtemp");
    char file[256], link[256], missing[256], fifo[256];
    snprintf(file, sizeof(file), "%s/site.conf", directory);
    snprintf(link, sizeof(link), "%s/config-link", directory);
    snprintf(missing, sizeof(missing), "%s/absent", directory);
    snprintf(fifo, sizeof(fifo), "%s/fifo", directory);
    int fd = open(file, O_WRONLY | O_CREAT | O_EXCL, 0640);
    must(fd >= 0, "open");
    must(write(fd, contents, sizeof(contents) - 1) == sizeof(contents) - 1,
         "write");
    must(close(fd) == 0, "close");
    rootcheck_set_error_callback(error_callback);

    policy("root-owned copy source", check_root_copy_source, file, 0);
    policy("root-owned config", check_root_protected_file, file, 0);
    policy("root-owned executable policy", check_root_executable, file, 0);
    must(chown(file, 8, 12345) == 0, "chown service file");
    must(chmod(file, 0640) == 0, "chmod service file");
    policy("service-owned 0640 copy source", check_root_copy_source, file, 0);
    policy("config still requires root ownership", check_root_protected_file,
           file, 403);
    policy("executable still requires root ownership", check_root_executable,
           file, 403);
    must(chmod(directory, 0711) == 0, "chmod Exim-style directory");
    bwrap_copy(file, 0, 0, "COPY copies service-owned data in root 0711 directory");
    bwrap_copy(file, 1, 0, "COPY-TRY copies service-owned data");
    namespace_copy(file, 0, 0, "namespace copies service-owned data");
    namespace_copy(file, OP_FLAG_ALLOW_NOTEXIST, 0, "optional namespace copies data");
    copy_failure_cases(missing, "missing file");
    int lines = stderr_lines;
    bwrap_copy(missing, 1, 1, "COPY-TRY skips missing file quietly");
    namespace_copy(missing, OP_FLAG_ALLOW_NOTEXIST, 1,
                   "optional namespace copy skips missing file quietly");
    namespace_copy(missing, OP_FLAG_SOURCE_CREATE, 1,
                   "SOURCE_CREATE copy skips missing file quietly");
    expect(stderr_lines == lines, "optional missing copy sources log nothing");

    char copy[512];
    snprintf(copy, sizeof(copy), "$COPY %s", file);
    bwrap_config_error(copy, "COPY without a target fails preparation");
    snprintf(copy, sizeof(copy), "$COPY-TRY %s", file);
    bwrap_config_error(copy, "COPY-TRY without a target fails preparation");
    snprintf(copy, sizeof(copy), "$COPY %s /config/$BOGUS", file);
    bwrap_config_error(copy, "COPY with an unknown target variable fails preparation");
    snprintf(copy, sizeof(copy), "$COPY %s/$BOGUS /config", directory);
    bwrap_config_error(copy, "COPY with an unknown source variable fails preparation");
    {
        SetupOp op = {0};
        op.type = SETUP_COPY;
        op.source = file;
        op.fd = -1;
        int rc = setup_copy(NULL, &op, 0);
        expect(rc != 0 && op.type == SETUP_COPY && op.fd == -1,
               "namespace copy without a dest fails setup");
    }

    snprintf(copy, sizeof(copy), "$COPY %s /config/$(invalid)", file);
    bwrap_config_error(copy, "COPY command substitution in target stays fatal");
    snprintf(copy, sizeof(copy), "$COPY %s/$(invalid) /config", directory);
    bwrap_config_error(copy, "COPY command substitution in source stays fatal");

    char long_path[201];
    memset(long_path, 'a', sizeof(long_path) - 1);
    long_path[sizeof(long_path) - 1] = 0;
    inject_malloc_call = 2; /* Initial argv memory succeeds, expansion fails. */
    for (int optional = 0; optional <= 1; ++optional)
    {
        const char *mode = optional ? "$COPY-TRY" : "$COPY";
        snprintf(copy, sizeof(copy), "%s %s /$UID/%s", mode, file, long_path);
        bwrap_copy_allocation_failure(copy, "target expansion OOM skips copy without client error");
        snprintf(copy, sizeof(copy), "%s /$UID/%s /config", mode, long_path);
        bwrap_copy_allocation_failure(copy, "source expansion OOM skips copy without client error");
    }
    snprintf(copy, sizeof(copy), "/$UID/%s", long_path);
    bwrap_config_error(copy, "expansion OOM outside COPY still fails request");

    request_uid = 0;
    s_bwrap_extra_bytes = 0; /* Force username/home cache allocation. */
    for (int optional = 0; optional <= 1; ++optional)
    {
        const char *mode = optional ? "$COPY-TRY" : "$COPY";
        snprintf(copy, sizeof(copy), "%s %s /$USER/config", mode, file);
        bwrap_copy_allocation_failure(copy, "username cache OOM skips copy without client error");
        snprintf(copy, sizeof(copy), "%s %s $HOMEDIR/config", mode, file);
        bwrap_copy_allocation_failure(copy, "home cache OOM skips copy without client error");
    }
    inject_malloc_call = 0;
    s_bwrap_extra_bytes = BWRAP_ALLOCATE_EXTRA_DEFAULT;
    inject_passwd_failure = 1;
    for (int optional = 0; optional <= 1; ++optional)
    {
        snprintf(copy, sizeof(copy), "$COPY%s %s $HOMEDIR/config",
                 optional ? "-TRY" : "", file);
        bwrap_copy_allocation_failure(copy, "passwd lookup OOM skips copy without client error");
    }
    inject_passwd_failure = 0;
    request_uid = 12345;

    must(chmod(file, 0660) == 0, "chmod group-write");
    policy("report group-writable copy source", check_root_copy_source, file, 403);
    copy_warning_cases(file, "group-write policy warning");
    must(chmod(file, 0642) == 0, "chmod other-write");
    policy("report other-writable copy source", check_root_copy_source, file, 403);
    copy_warning_cases(file, "other-write policy warning");
    must(chmod(file, 0640) == 0, "chmod restore file");
    must(chmod(directory, 0777) == 0, "chmod directory");
    policy("report writable source directory", check_root_copy_source, file, 403);
    copy_warning_cases(file, "writable directory policy warning");
    must(chmod(directory, 0700) == 0, "chmod restore directory");
    must(chown(directory, 8, 0) == 0, "chown directory");
    policy("report non-root source directory", check_root_copy_source, file, 403);
    copy_warning_cases(file, "service-owned directory policy warning");
    must(chown(directory, 0, 0) == 0, "chown restore directory");

    must(symlink(file, link) == 0, "symlink");
    policy("root-owned link to service-owned data", check_root_copy_source, link, 0);
    policy("config link target still requires root", check_root_protected_file,
           link, 403);
    must(lchown(link, 8, 12345) == 0, "lchown");
    policy("report non-root symlink", check_root_copy_source, link, 403);
    copy_warning_cases(link, "service-owned symlink policy warning");
    policy("missing source remains 404", check_root_copy_source, missing, 404);
    policy("report directory as copy data", check_root_copy_source, directory, 403);
    must(mkfifo(fifo, 0600) == 0, "mkfifo");
    policy("report non-regular copy data", check_root_copy_source, fifo, 403);

    copy_failure_cases(fifo, "non-regular file");
    namespace_copy(fifo, OP_FLAG_SOURCE_CREATE, 1, "SOURCE_CREATE copy skips failure");

    /* A source open failure after validation was bypassed for non-root use. */
    must(chmod(directory, 0755) == 0, "chmod directory for open/read failures");
    must(chmod(file, 0000) == 0, "chmod unreadable source");
    must(seteuid(65534) == 0, "drop effective uid");
    copy_failure_cases(file, "open failure");
    copy_failure_cases(directory, "directory source");
    must(seteuid(0) == 0, "restore effective uid");
    must(chmod(file, 0640) == 0, "restore source mode");

    /* Sources past the default 64 KiB pipe capacity are copied after the pipe
     * is grown.  Sources over the limit are skipped without hanging or using
     * a partially copied file. */
    fd = open(file, O_WRONLY);
    must(fd >= 0, "open large source");
    must(ftruncate(fd, 256 * 1024) == 0, "extend source");
    expected_size = 256 * 1024;
    copy_success_cases(file, "256 KiB source past default pipe capacity");
    must(ftruncate(fd, BWRAP_COPY_MAX_SIZE) == 0, "extend source to limit");
    expected_size = BWRAP_COPY_MAX_SIZE;
    copy_success_cases(file, "source at the copy size limit");
    must(ftruncate(fd, BWRAP_COPY_MAX_SIZE + 1) == 0, "extend source past limit");
    must(close(fd) == 0, "close large source");
    copy_failure_cases(file, "oversized source");
    must(unlink(fifo) == 0, "unlink fifo");
    must(unlink(link) == 0, "unlink link");
    must(unlink(file) == 0, "unlink file");
    must(rmdir(directory) == 0, "rmdir");
    printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
