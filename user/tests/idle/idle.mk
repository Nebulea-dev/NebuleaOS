$(eval $(call clear-module-vars))
LOCAL_MODULE_PATH := $(call my-dir)

$(eval $(call clear-process-vars))
LOCAL_PROCESS_NAME := idle
LOCAL_PROCESS_SRC := idle.c
$(eval $(call build-test-process))
$(eval $(call build-test-module))
