// Simple Eden log-collector wrapper
// Exports vkGetInstanceProcAddr and vkGetDeviceProcAddr and logs the requested symbol names.
// Writes logs to /storage/emulated/0/eden_wrapper_logs/vulkan_calls.txt

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
static int log_fd = -1;
static int logger_inited = 0;
static const char *log_dir = "/storage/emulated/0/eden_wrapper_logs";
static const char *log_file_path = "/storage/emulated/0/eden_wrapper_logs/vulkan_calls.txt";

static void ensure_log_dir_and_file(void) {
    if (logger_inited) return;
    pthread_mutex_lock(&log_lock);
    if (logger_inited) {
        pthread_mutex_unlock(&log_lock);
        return;
    }
    // create directory with 0755 if needed
    mkdir("/storage", 0755); // safe no-op if exists
    mkdir("/storage/emulated", 0755);
    mkdir("/storage/emulated/0", 0755);
    mkdir(log_dir, 0755);

    // open file for append
    log_fd = open(log_file_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd < 0) {
        // try fallback path in app data
        log_fd = open("/data/local/tmp/eden_wrapper_vulkan_calls.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    }
    logger_inited = 1;
    pthread_mutex_unlock(&log_lock);
}

static void safe_write_log(const char *fmt, ...) {
    ensure_log_dir_and_file();
    if (log_fd < 0) return;
    pthread_mutex_lock(&log_lock);
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n > 0) {
        // append newline
        if (n < (int)sizeof(buf) - 2) {
            buf[n++] = '\n';
            buf[n] = '\0';
        }
        write(log_fd, buf, n);
    }
    pthread_mutex_unlock(&log_lock);
}

// Typedefs for Vulkan entry points (we don't include vulkan headers)
typedef void* PFN_vkVoidFunction;
typedef PFN_vkVoidFunction (*PFN_vkGetInstanceProcAddr)(void* instance, const char* pName);
typedef PFN_vkVoidFunction (*PFN_vkGetDeviceProcAddr)(void* device, const char* pName);

// Forwarding using dlsym(RTLD_NEXT, ...)
static PFN_vkGetInstanceProcAddr real_vkGetInstanceProcAddr = NULL;
static PFN_vkGetDeviceProcAddr real_vkGetDeviceProcAddr = NULL;

__attribute__((constructor))
static void xeno_init(void) {
    // Called when library is loaded by Eden. initialize logger and record load.
    ensure_log_dir_and_file();
    safe_write_log("[xeno] libvulkan.so wrapper loaded");
    // Try to resolve next symbols if available
    real_vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(RTLD_NEXT, "vkGetInstanceProcAddr");
    real_vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)dlsym(RTLD_NEXT, "vkGetDeviceProcAddr");
    if (!real_vkGetInstanceProcAddr) {
        safe_write_log("[xeno] warning: real vkGetInstanceProcAddr not found via RTLD_NEXT");
    } else {
        safe_write_log("[xeno] resolved real vkGetInstanceProcAddr");
    }
    if (!real_vkGetDeviceProcAddr) {
        safe_write_log("[xeno] note: vkGetDeviceProcAddr not found via RTLD_NEXT");
    }
}

__attribute__((destructor))
static void xeno_fini(void) {
    safe_write_log("[xeno] libvulkan.so wrapper unloading");
    if (log_fd >= 0) close(log_fd);
}

// Exported function: vkGetInstanceProcAddr
void* vkGetInstanceProcAddr(void* instance, const char* pName) {
    // Log call
    safe_write_log("[vkGetInstanceProcAddr] name=%s instance=%p", pName ? pName : "(null)", instance);
    // Ensure we have the real symbol
    if (!real_vkGetInstanceProcAddr) {
        real_vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(RTLD_NEXT, "vkGetInstanceProcAddr");
        if (!real_vkGetInstanceProcAddr) {
            safe_write_log("[xeno] ERROR: cannot find real vkGetInstanceProcAddr (RTLD_NEXT)");
            return NULL;
        }
    }
    // Get the real function pointer
    PFN_vkVoidFunction f = real_vkGetInstanceProcAddr(instance, pName);
    // If it's a function we care about, optionally wrap it here in the future.
    return f;
}

// Exported function: vkGetDeviceProcAddr
void* vkGetDeviceProcAddr(void* device, const char* pName) {
    safe_write_log("[vkGetDeviceProcAddr] name=%s device=%p", pName ? pName : "(null)", device);
    if (!real_vkGetDeviceProcAddr) {
        real_vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)dlsym(RTLD_NEXT, "vkGetDeviceProcAddr");
        if (!real_vkGetDeviceProcAddr) {
            // Try to obtain device proc via instance proc
            if (real_vkGetInstanceProcAddr) {
                PFN_vkVoidFunction f = real_vkGetInstanceProcAddr(NULL, pName);
                return f;
            }
            safe_write_log("[xeno] WARNING: cannot find real vkGetDeviceProcAddr");
            return NULL;
        }
    }
    PFN_vkVoidFunction f = real_vkGetDeviceProcAddr(device, pName);
    return f;
}
