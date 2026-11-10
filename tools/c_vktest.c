#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define VK_CREATE_INSTANCE      1
void create_props(void **outbuf, uint32_t total);

void *g_base = 0;

int main(void) {
    void *h = dlopen("/data/data/com.termux/files/usr/tmp/opencode/panvk/libvulkan_panfrost.so", RTLD_NOW);
    if (!h) { printf("dlopen falhou: %s\n", dlerror()); return 1; }
    g_base = (void*)h;
    printf("[1] dlopen ok\n");

    void *fmt = dlsym(h, "vk_icdNegotiateLoaderICDInterfaceVersion");
    void *gipa = dlsym(h, "vk_icdGetInstanceProcAddr");
    printf("[2] symbols: negotiate=0x%llx gipa=0x%llx\n", (unsigned long long)fmt, (unsigned long long)gipa);

    int version = 0;
    ((int (*)(int *))fmt)(&version);
    printf("[3] interface = %d\n", version);

    void *(*getProc)(void *, const char *) = (void *(*)(void *, const char *))gipa;
    void *vkCreateInstance = getProc(NULL, "vkCreateInstance");
    printf("[4] vkCreateInstance = 0x%llx\n", (unsigned long long)(uintptr_t)vkCreateInstance);

    /* VkApplicationInfo (sType=3, apiVersion 1.3.0) */
    uint64_t appinfo[6];
    memset(appinfo, 0, sizeof(appinfo));
    *(uint32_t *)appinfo = 3;
    *(uint32_t *)((char *)appinfo + 44) = (1u << 22) | (3u << 12);
    char apname[] = "panvk-c-test";
    appinfo[2] = (uint64_t)(uintptr_t)apname;

    /* VkInstanceCreateInfo (sType=1) */
    uint64_t ci[8];
    memset(ci, 0, sizeof(ci));
    *(uint32_t *)ci = VK_CREATE_INSTANCE;
    ci[3] = (uint64_t)(uintptr_t)appinfo;

    void *inst = NULL;
    int r = ((int (*)(void *, void *, void *))vkCreateInstance)(ci, NULL, &inst);
    printf("[5] vkCreateInstance = %d inst=0x%llx\n", r, (unsigned long long)(uintptr_t)inst);
    if (r != 0) return 2;

    void *enumFn = getProc(inst, "vkEnumeratePhysicalDevices");
    uint32_t n = 0;
    ((int (*)(void *, uint32_t *, void *))enumFn)(inst, &n, NULL);
    void **pds = calloc(n, sizeof(void *));
    ((int (*)(void *, uint32_t *, void *))enumFn)(inst, &n, pds);
    printf("[6] gpus=%d\n", n);

    void *qp = getProc(inst, "vkGetPhysicalDeviceProperties");
    uint32_t props[256];  /* 1024 bytes >= sizeof(VkPhysicalDeviceProperties)=832 */
    memset(props, 0, sizeof(props));
    ((void (*)(void *, void *))qp)(pds[0], props);
    printf("[7] name=%s api=0x%x driver=0x%x vendor=0x%x\n",
           (char *)props + 20, props[0], props[1], props[2]);

    /* device */
    void *cdev = getProc(inst, "vkCreateDevice");
    float prio = 1.0f;
    uint32_t qci[6];
    memset(qci, 0, sizeof(qci));
    *(uint32_t *)qci = 2;      /* DEVICE_QUEUE_CREATE_INFO */
    *(uint32_t *)((char *)qci + 8) = 0;  /* family 0 */
    *(uint32_t *)((char *)qci + 12) = 1;
    *(uint64_t *)((char *)qci + 16) = (uint64_t)(uintptr_t)&prio;
    uint64_t dci[9];
    memset(dci, 0, sizeof(dci));
    *(uint32_t *)dci = 3;
    dci[3] = (uint64_t)(uintptr_t)qci;
    void *dev = NULL;
    r = ((int (*)(void *, void *, void *, void *))cdev)(pds[0], dci, NULL, &dev);
    printf("[8] vkCreateDevice = %d dev=0x%llx\n", r, (unsigned long long)(uintptr_t)dev);

    printf("[9] fim limpo (sem unload) - exit\n");
    return 0;
}
