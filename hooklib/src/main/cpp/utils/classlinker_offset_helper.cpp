//
// Created by windysha on 2022/8/26.
//

#include "../includes/classlinker_offset_helper.h"
#include "../includes/elf_util.h"
#include <jni.h>
#include <unistd.h>
#include <fcntl.h>

static const size_t kPointerSize = sizeof(void *);
static constexpr size_t kVTablePosition = 2;

static __always_inline void *SymbolToVTable(void *symbol) {
    return (char *) symbol + kPointerSize * kVTablePosition;
}

static bool IsValidAddress(const void *p) {
    if (!p) {
        return false;
    }

    bool ret = true;
    int fd = open("/dev/random", O_WRONLY);
    size_t len = sizeof(int32_t);
    if (fd != -1) {
        if (write(fd, p, len) < 0) {
            ret = false;
        }
        close(fd);
    } else {
        ret = false;
    }
    return ret;
}

static int
findOffset(void *start, size_t max_count, size_t step, void *value, int start_search_offset = 0) {
    if (nullptr == start) {
        return -1;
    }
    if (start_search_offset > max_count) {
        return -1;
    }

    for (int i = start_search_offset; i <= max_count; i += step) {
        void *current_value = *reinterpret_cast<void **>((size_t) start + i);
        if (value == current_value) {
            return i;
        }
    }
    return -1;
}

int32_t SearchClassLinkerOffset(JavaVM *vm, void *runtime_instance, const char *art_lib_path) {
    SandHook::ElfImg libart{art_lib_path};
    void *class_linker_vtable = reinterpret_cast<void *>(libart.getSymbAddress(
            "_ZTVN3art11ClassLinkerE"));

    if (class_linker_vtable != nullptr) {
        // Before Android 9, class_liner have no virtual table, so class_linker_vtable is null.
        class_linker_vtable = SymbolToVTable(class_linker_vtable);
    }

    //need to search jvm offset from 200, if not, on xiaomi android7.1.2 we would get jvm_offset: 184, but in fact it is 440
    int jvm_offset_in_runtime = findOffset(runtime_instance, 1000, 4, (void *) vm, 200);

    int step = 4;
    int class_linker_offset_value = -1;
    for (int i = jvm_offset_in_runtime - step; i > 0; i -= step) {
        void *class_linker_addr = *reinterpret_cast<void **>((uintptr_t) runtime_instance + i);

        if (class_linker_addr == nullptr || !IsValidAddress(class_linker_addr)) {
            continue;
        }
        if (class_linker_vtable != nullptr) {
            if (*(void **) class_linker_addr == class_linker_vtable) {
                class_linker_offset_value = i;
                break;
            }
        } else {
            // in runtime.h,
            //    ThreadList* thread_list_;
            //    InternTable* intern_table_;
            //    ClassLinker* class_linker_;
            // these objects list as this kind of order.
            // And intern_table_ pointer is also saved in struct ClassLinker:
            // So, we can search ClassLinker struct to verify intern_table_ address.
            void *intern_table_addr = *reinterpret_cast<void **>(
                    (uintptr_t) runtime_instance +
                    i -
                    kPointerSize);
            if (IsValidAddress(intern_table_addr)) {
                for (int j = 200; j < 500; j += step) {
                    void *intern_table = *reinterpret_cast<void **>(
                            (uintptr_t) class_linker_addr + j);
                    if (intern_table_addr == intern_table) {
                        class_linker_offset_value = i;
                        break;
                    }
                }
            }
        }
        if (class_linker_offset_value > 0) {
            break;
        }
    }
    return class_linker_offset_value;
}
