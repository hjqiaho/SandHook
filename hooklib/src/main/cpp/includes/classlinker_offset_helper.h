//
// Created by windysha on 2022/8/26.
//

#ifndef SANDHOOK_CLASSLINKER_OFFSET_HELPER_H
#define SANDHOOK_CLASSLINKER_OFFSET_HELPER_H

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif
int32_t SearchClassLinkerOffset(JavaVM *vm, void *runtime_instance, const char *art_lib_path);
#ifdef __cplusplus
}
#endif

#endif //SANDHOOK_CLASSLINKER_OFFSET_HELPER_H
