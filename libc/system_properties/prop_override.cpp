/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * See system_properties/prop_override.h.
 */

#include "system_properties/prop_override.h"

#include <stdatomic.h>
#include <string.h>

#include <async_safe/log.h>

// Process-local, fixed-capacity so population needs no allocation and lookups
// are lock-free. 32 entries is far more than the handful of build-version
// properties the only caller spoofs.
#define PROP_OVERRIDE_MAX_ENTRIES 32
#define PROP_OVERRIDE_NAME_MAX 92
#define PROP_OVERRIDE_VALUE_MAX 92

struct prop_override_entry {
  char name[PROP_OVERRIDE_NAME_MAX];
  char value[PROP_OVERRIDE_VALUE_MAX];
};

static prop_override_entry g_entries[PROP_OVERRIDE_MAX_ENTRIES];
// Published count. Entries [0, count) are fully written before count is stored
// with release ordering, so a reader that acquire-loads count sees their
// contents. Single writer (app bind time); readers are concurrent.
static _Atomic(uint32_t) g_count = 0;

const char* prop_override_lookup(const char* name) {
  uint32_t count = atomic_load_explicit(&g_count, memory_order_acquire);
  if (count == 0 || name == nullptr) {
    return nullptr;
  }
  for (uint32_t i = 0; i < count; i++) {
    if (strcmp(g_entries[i].name, name) == 0) {
      return g_entries[i].value;
    }
  }
  return nullptr;
}

void prop_override_add(const char* name, const char* value) {
  if (name == nullptr || value == nullptr) {
    return;
  }
  uint32_t count = atomic_load_explicit(&g_count, memory_order_relaxed);
  if (count >= PROP_OVERRIDE_MAX_ENTRIES) {
    async_safe_format_log(ANDROID_LOG_WARN, "libc",
                          "prop_override: table full, dropping \"%s\"", name);
    return;
  }
  if (strlen(name) >= PROP_OVERRIDE_NAME_MAX || strlen(value) >= PROP_OVERRIDE_VALUE_MAX) {
    async_safe_format_log(ANDROID_LOG_WARN, "libc",
                          "prop_override: \"%s\" name/value too long, dropped", name);
    return;
  }
  strlcpy(g_entries[count].name, name, PROP_OVERRIDE_NAME_MAX);
  strlcpy(g_entries[count].value, value, PROP_OVERRIDE_VALUE_MAX);
  atomic_store_explicit(&g_count, count + 1, memory_order_release);
}

void prop_override_clear() {
  atomic_store_explicit(&g_count, 0, memory_order_release);
}
