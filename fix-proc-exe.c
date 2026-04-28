#define _GNU_SOURCE
#define _LARGEFILE64_SOURCE

#include <dlfcn.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef open64
#undef open64
#endif
#ifdef openat64
#undef openat64
#endif
#ifdef fopen64
#undef fopen64
#endif
#ifdef freopen64
#undef freopen64
#endif
#ifdef stat64
#undef stat64
#endif
#ifdef lstat64
#undef lstat64
#endif

/* Set this per app to the target under $APPDIR for /proc/self/exe redirects */
#ifndef FIX_PROC_EXE_TARGET_PATH
#define FIX_PROC_EXE_TARGET_PATH "shared/bin/asphyxia-core"
#endif

static const char proc_exe_path[] = "/proc/self/exe";

static const char *resolve_target_path(void) {
	static char target[PATH_MAX];
	const char *appdir;

	appdir = getenv("APPDIR");
	if (appdir == NULL || appdir[0] == '\0') {
		return NULL;
	}

	if (snprintf(target, sizeof(target), "%s/%s", appdir, FIX_PROC_EXE_TARGET_PATH) >= (int)sizeof(target)) {
		return NULL;
	}

	return target;
}

static const char *redirect_path(const char *path) {
	const char *target;

	if (path == NULL || strcmp(path, proc_exe_path) != 0) {
		return path;
	}

	target = resolve_target_path();
	return target != NULL ? target : path;
}

static ssize_t redirect_readlink(const char *path, char *buf, size_t bufsiz) {
	const char *target;
	size_t len;

	if (path == NULL || strcmp(path, proc_exe_path) != 0) {
		return (ssize_t)-1;
	}

	target = resolve_target_path();
	if (target == NULL || bufsiz == 0) {
		return (ssize_t)-1;
	}

	len = strlen(target);
	if (len >= bufsiz) {
		memcpy(buf, target, bufsiz);
		return (ssize_t)bufsiz;
	}

	memcpy(buf, target, len);
	return (ssize_t)len;
}

static int should_supply_mode(int flags) {
#ifdef O_TMPFILE
	return (flags & O_CREAT) != 0 || (flags & O_TMPFILE) == O_TMPFILE;
#else
	return (flags & O_CREAT) != 0;
#endif
}

static void *resolve_symbol(const char *name) {
	return dlsym(RTLD_NEXT, name);
}

static void *resolve_symbol_fallback(const char *name, const char *fallback) {
	void *symbol = resolve_symbol(name);

	if (symbol == NULL && fallback != NULL) {
		symbol = resolve_symbol(fallback);
	}

	return symbol;
}

static int call_open(int (*fn)(const char *, int, ...), const char *path, int flags, mode_t mode, int supply_mode) {
	return supply_mode ? fn(path, flags, mode) : fn(path, flags);
}

static int call_openat(int (*fn)(int, const char *, int, ...), int dirfd, const char *path, int flags, mode_t mode, int supply_mode) {
	return supply_mode ? fn(dirfd, path, flags, mode) : fn(dirfd, path, flags);
}

int open(const char *pathname, int flags, ...) {
	static int (*real_open)(const char *, int, ...);
	va_list ap;
	mode_t mode = 0;
	int supply_mode = should_supply_mode(flags);

	if (real_open == NULL) {
		real_open = resolve_symbol("open");
	}

	if (supply_mode) {
		va_start(ap, flags);
		mode = (mode_t)va_arg(ap, int);
		va_end(ap);
	}

	return call_open(real_open, redirect_path(pathname), flags, mode, supply_mode);
}

int open64(const char *pathname, int flags, ...) {
	static int (*real_open64)(const char *, int, ...);
	va_list ap;
	mode_t mode = 0;
	int supply_mode = should_supply_mode(flags);

	if (real_open64 == NULL) {
		real_open64 = resolve_symbol_fallback("open64", "open");
	}

	if (supply_mode) {
		va_start(ap, flags);
		mode = (mode_t)va_arg(ap, int);
		va_end(ap);
	}

	return call_open(real_open64, redirect_path(pathname), flags, mode, supply_mode);
}

int openat(int dirfd, const char *pathname, int flags, ...) {
	static int (*real_openat)(int, const char *, int, ...);
	va_list ap;
	mode_t mode = 0;
	int supply_mode = should_supply_mode(flags);

	if (real_openat == NULL) {
		real_openat = resolve_symbol("openat");
	}

	if (supply_mode) {
		va_start(ap, flags);
		mode = (mode_t)va_arg(ap, int);
		va_end(ap);
	}

	return call_openat(real_openat, dirfd, redirect_path(pathname), flags, mode, supply_mode);
}

int openat64(int dirfd, const char *pathname, int flags, ...) {
	static int (*real_openat64)(int, const char *, int, ...);
	va_list ap;
	mode_t mode = 0;
	int supply_mode = should_supply_mode(flags);

	if (real_openat64 == NULL) {
		real_openat64 = resolve_symbol_fallback("openat64", "openat");
	}

	if (supply_mode) {
		va_start(ap, flags);
		mode = (mode_t)va_arg(ap, int);
		va_end(ap);
	}

	return call_openat(real_openat64, dirfd, redirect_path(pathname), flags, mode, supply_mode);
}

FILE *fopen(const char *pathname, const char *mode) {
	static FILE *(*real_fopen)(const char *, const char *);

	if (real_fopen == NULL) {
		real_fopen = resolve_symbol("fopen");
	}

	return real_fopen(redirect_path(pathname), mode);
}

FILE *fopen64(const char *pathname, const char *mode) {
	static FILE *(*real_fopen64)(const char *, const char *);

	if (real_fopen64 == NULL) {
		real_fopen64 = resolve_symbol_fallback("fopen64", "fopen");
	}

	return real_fopen64(redirect_path(pathname), mode);
}

FILE *freopen(const char *pathname, const char *mode, FILE *stream) {
	static FILE *(*real_freopen)(const char *, const char *, FILE *);

	if (real_freopen == NULL) {
		real_freopen = resolve_symbol("freopen");
	}

	return real_freopen(redirect_path(pathname), mode, stream);
}

FILE *freopen64(const char *pathname, const char *mode, FILE *stream) {
	static FILE *(*real_freopen64)(const char *, const char *, FILE *);

	if (real_freopen64 == NULL) {
		real_freopen64 = resolve_symbol_fallback("freopen64", "freopen");
	}

	return real_freopen64(redirect_path(pathname), mode, stream);
}

int access(const char *pathname, int amode) {
	static int (*real_access)(const char *, int);

	if (real_access == NULL) {
		real_access = resolve_symbol("access");
	}

	return real_access(redirect_path(pathname), amode);
}

int faccessat(int dirfd, const char *pathname, int amode, int flags) {
	static int (*real_faccessat)(int, const char *, int, int);

	if (real_faccessat == NULL) {
		real_faccessat = resolve_symbol("faccessat");
	}

	return real_faccessat(dirfd, redirect_path(pathname), amode, flags);
}

int stat(const char *pathname, struct stat *buf) {
	static int (*real_stat)(const char *, struct stat *);

	if (real_stat == NULL) {
		real_stat = resolve_symbol("stat");
	}

	return real_stat(redirect_path(pathname), buf);
}

int lstat(const char *pathname, struct stat *buf) {
	static int (*real_lstat)(const char *, struct stat *);

	if (real_lstat == NULL) {
		real_lstat = resolve_symbol("lstat");
	}

	return real_lstat(redirect_path(pathname), buf);
}

int fstatat(int dirfd, const char *pathname, struct stat *buf, int flags) {
	static int (*real_fstatat)(int, const char *, struct stat *, int);

	if (real_fstatat == NULL) {
		real_fstatat = resolve_symbol("fstatat");
	}

	return real_fstatat(dirfd, redirect_path(pathname), buf, flags);
}

ssize_t readlink(const char *pathname, char *buf, size_t bufsiz) {
	static ssize_t (*real_readlink)(const char *, char *, size_t);
	ssize_t redirected;

	redirected = redirect_readlink(pathname, buf, bufsiz);
	if (redirected >= 0) {
		return redirected;
	}

	if (real_readlink == NULL) {
		real_readlink = resolve_symbol("readlink");
	}

	return real_readlink(pathname, buf, bufsiz);
}

ssize_t readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz) {
	static ssize_t (*real_readlinkat)(int, const char *, char *, size_t);
	ssize_t redirected;

	redirected = redirect_readlink(pathname, buf, bufsiz);
	if (redirected >= 0) {
		return redirected;
	}

	if (real_readlinkat == NULL) {
		real_readlinkat = resolve_symbol("readlinkat");
	}

	return real_readlinkat(dirfd, pathname, buf, bufsiz);
}

#if defined(__GLIBC__)
int stat64(const char *pathname, struct stat64 *buf) {
	static int (*real_stat64)(const char *, struct stat64 *);

	if (real_stat64 == NULL) {
		real_stat64 = resolve_symbol_fallback("stat64", "stat");
	}

	return real_stat64(redirect_path(pathname), buf);
}

int lstat64(const char *pathname, struct stat64 *buf) {
	static int (*real_lstat64)(const char *, struct stat64 *);

	if (real_lstat64 == NULL) {
		real_lstat64 = resolve_symbol_fallback("lstat64", "lstat");
	}

	return real_lstat64(redirect_path(pathname), buf);
}

int __xstat(int ver, const char *pathname, struct stat *buf) {
	static int (*real___xstat)(int, const char *, struct stat *);

	if (real___xstat == NULL) {
		real___xstat = resolve_symbol("__xstat");
	}

	return real___xstat(ver, redirect_path(pathname), buf);
}

int __lxstat(int ver, const char *pathname, struct stat *buf) {
	static int (*real___lxstat)(int, const char *, struct stat *);

	if (real___lxstat == NULL) {
		real___lxstat = resolve_symbol("__lxstat");
	}

	return real___lxstat(ver, redirect_path(pathname), buf);
}

int __xstat64(int ver, const char *pathname, struct stat64 *buf) {
	static int (*real___xstat64)(int, const char *, struct stat64 *);

	if (real___xstat64 == NULL) {
		real___xstat64 = resolve_symbol_fallback("__xstat64", "__xstat");
	}

	return real___xstat64(ver, redirect_path(pathname), buf);
}

int __lxstat64(int ver, const char *pathname, struct stat64 *buf) {
	static int (*real___lxstat64)(int, const char *, struct stat64 *);

	if (real___lxstat64 == NULL) {
		real___lxstat64 = resolve_symbol_fallback("__lxstat64", "__lxstat");
	}

	return real___lxstat64(ver, redirect_path(pathname), buf);
}
#endif
