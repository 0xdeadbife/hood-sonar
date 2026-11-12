#pragma once
#include "scanner_common.h"

// Look up the OUI (first 3 MAC bytes) in the curated vendor table.
// Returns true if found; sets *vendor and *cat (either may be NULL).
// On miss: *vendor = "", *cat = CAT_UNKNOWN, returns false.
bool scanner_oui_lookup(const uint8_t *mac, const char **vendor, cat_t *cat);

// Number of OUI entries in the table.
unsigned int scanner_oui_count(void);
