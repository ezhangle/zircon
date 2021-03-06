// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "device-internal.h"
#include "devcoordinator.h"

#include <ddk/binding.h>
#include <ddk/device.h>
#include <ddk/driver.h>

#include <fdio/remoteio.h>

#include <zircon/types.h>

#include <threads.h>
#include <stdint.h>


// Handle IDs for USER0 handles
#define ID_HJOBROOT 4

// Nothing outside of devmgr/{devmgr,devhost,rpc-device}.c
// should be calling devhost_*() APIs, as this could
// violate the internal locking design.

// Safe external APIs are in device.h and device_internal.h

typedef struct zx_driver {
    const char* name;
    const zx_driver_ops_t* ops;
    void* ctx;
    const char* libname;
    list_node_t node;
    zx_status_t status;
} zx_driver_t;

extern zx_protocol_device_t device_default_ops;

zx_status_t devhost_device_unbind(zx_device_t* dev);

zx_status_t devhost_device_add(zx_device_t* dev, zx_device_t* parent,
                               const zx_device_prop_t* props, uint32_t prop_count,
                               const char* businfo, zx_handle_t resource);
zx_status_t devhost_device_install(zx_device_t* dev);
zx_status_t devhost_device_remove(zx_device_t* dev);
zx_status_t devhost_device_bind(zx_device_t* dev, const char* drv_libname);
zx_status_t devhost_device_rebind(zx_device_t* dev);
zx_status_t devhost_device_create(zx_driver_t* drv, zx_device_t* parent,
                                  const char* name, void* ctx,
                                  zx_protocol_device_t* ops, zx_device_t** out);
zx_status_t devhost_device_open_at(zx_device_t* dev, zx_device_t** out,
                                 const char* path, uint32_t flags);
zx_status_t devhost_device_close(zx_device_t* dev, uint32_t flags);
void devhost_device_destroy(zx_device_t* dev);

zx_status_t devhost_load_firmware(zx_device_t* dev, const char* path,
                                  zx_handle_t* fw, size_t* size);

zx_status_t devhost_get_topo_path(zx_device_t* dev, char* path, size_t max, size_t* actual);

// shared between devhost.c and rpc-device.c
typedef struct devhost_iostate {
    zx_device_t* dev;
    size_t io_off;
    uint32_t flags;
    bool dead;
    port_handler_t ph;
} devhost_iostate_t;

devhost_iostate_t* create_devhost_iostate(zx_device_t* dev);
zx_status_t devhost_rio_handler(zxrio_msg_t* msg, void* cookie);

zx_status_t devhost_start_iostate(devhost_iostate_t* ios, zx_handle_t h);

// routines devhost uses to talk to dev coordinator
zx_status_t devhost_add(zx_device_t* dev, zx_device_t* child,
                        const char* businfo, zx_handle_t resource,
                        const zx_device_prop_t* props, uint32_t prop_count);
zx_status_t devhost_remove(zx_device_t* dev);


// device refcounts
void dev_ref_release(zx_device_t* dev);
static inline void dev_ref_acquire(zx_device_t* dev) {
    dev->refcount++;
}

zx_handle_t get_root_resource(void);

zx_device_t* device_create_setup(zx_device_t* parent);

// locking and lock debugging
extern mtx_t __devhost_api_lock;
extern bool __dm_locked;

#if 0
static inline void __DM_DIE(const char* fn, int ln) {
    cprintf("OOPS: %s: %d\n", fn, ln);
    *((int*) 0x3333) = 1;
}
static inline void __DM_LOCK(const char* fn, int ln) {
    //cprintf(devhost_is_remote ? "X" : "+");
    if (__dm_locked) __DM_DIE(fn, ln);
    mtx_lock(&__devhost_api_lock);
    cprintf("LOCK: %s: %d\n", fn, ln);
    __dm_locked = true;
}

static inline void __DM_UNLOCK(const char* fn, int ln) {
    cprintf("UNLK: %s: %d\n", fn, ln);
    //cprintf(devhost_is_remote ? "x" : "-");
    if (!__dm_locked) __DM_DIE(fn, ln);
    __dm_locked = false;
    mtx_unlock(&__devhost_api_lock);
}

#define DM_LOCK() __DM_LOCK(__FILE__, __LINE__)
#define DM_UNLOCK() __DM_UNLOCK(__FILE__, __LINE__)
#else
static inline void DM_LOCK(void) {
    mtx_lock(&__devhost_api_lock);
}

static inline void DM_UNLOCK(void) {
    mtx_unlock(&__devhost_api_lock);
}
#endif
