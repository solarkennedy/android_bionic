/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * Process-local system-property value overrides.
 *
 * A read-only table, empty in every process until something populates it via
 * __system_property_override_add() after fork. When non-empty, SystemProperties
 * read paths (Read / ReadCallback, hence __system_property_get /
 * __system_property_read / __system_property_read_callback) return the override
 * value for a matching name. Nothing is written to the shared __properties__
 * area, so the override is confined to the calling process and invisible to any
 * other. Used by the framework Play Integrity hook to present legacy build
 * identity to the DroidGuard process only; see
 * frameworks/base android.security.pif.
 */

#pragma once

#include <sys/cdefs.h>

__BEGIN_DECLS

// Returns the override value for |name|, or nullptr if there is none. The
// returned pointer is owned by the table and stays valid for the process
// lifetime (the table is never freed or shrunk, only cleared to empty).
const char* prop_override_lookup(const char* name);

// Appends name -> value. Silently ignored once the table is full or if the
// name/value do not fit. Expected to be called single-threaded, at app bind
// time, before the overridden properties are read.
void prop_override_add(const char* name, const char* value);

// Empties the table (subsequent lookups return nullptr).
void prop_override_clear();

__END_DECLS
