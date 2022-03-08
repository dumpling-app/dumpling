typedef struct ThreadContext {
    uint32_t cspr;
    uint32_t gpr[14];
    uint32_t lr;
    uint32_t pc;
    struct ThreadContext *threadQueueNext;
    uint32_t maxPriority;
    uint32_t priority;
    uint32_t state;
    uint32_t pid;
    uint32_t id;
    uint32_t flags;
    uint32_t exitValue;
    struct ThreadContext **joinQueue;
    struct ThreadContext **threadQueue;
    uint8_t unk1[56];
    void *stackPointer;
    uint8_t unk2[8];
    void *sysStackAddr;
    void *userStackAddr;
    uint32_t userStackSize;
    void *threadLocalStorage;
    uint32_t profileCount;
    uint32_t profileTime;
} ThreadContext_t;

typedef struct {
    uint32_t paddr;
    uint32_t vaddr;
    uint32_t size;
    uint32_t domain;
    uint32_t type;
    uint32_t cached;
} ios_map_shared_info_t;

extern ThreadContext_t **currentThreadContext;

void installPatches();