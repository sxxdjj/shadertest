LOCAL_PATH := $(call my-dir)

#include $(CLEAR_VARS)

include $(CLEAR_VARS)

$(call import-add-path,$(LOCAL_PATH)/../../../cocos2d)
$(call import-add-path,$(LOCAL_PATH)/../../../cocos2d/external)
$(call import-add-path,$(LOCAL_PATH)/../../../cocos2d/cocos)

LOCAL_MODULE := cocos2dcpp_shared

LOCAL_MODULE_FILENAME := libcocos2dcpp

SRC_SUFFIX := *.cpp *.c *.cc
SRC_ROOT := $(LOCAL_PATH)/../../../Classes

ALL_FILES := $(shell find $(SRC_ROOT) -type f)
SRC_FILES := $(filter $(subst *,%,$(SRC_SUFFIX)),$(ALL_FILES))
LOCAL_SRC_FILES := hellocpp/main.cpp
LOCAL_SRC_FILES += $(SRC_FILES:$(LOCAL_PATH)/%=%)

SRC_DIRS := $(shell find $(SRC_ROOT))
LOCAL_C_INCLUDES := $(SRC_DIRS)

# _COCOS_HEADER_ANDROID_BEGIN
# _COCOS_HEADER_ANDROID_END

LOCAL_STATIC_LIBRARIES := cocos2dx_static
# _COCOS_LIB_ANDROID_BEGIN
# _COCOS_LIB_ANDROID_END

include $(BUILD_SHARED_LIBRARY)

$(call import-module,.)

# _COCOS_LIB_IMPORT_ANDROID_BEGIN
# _COCOS_LIB_IMPORT_ANDROID_END

