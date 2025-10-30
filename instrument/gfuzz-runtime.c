/*
   GFuzz - Runtime Variable State Tracking
   ----------------------------------------
   
   Provides runtime functions for tracking variable states during execution.
   This is linked with the instrumented binary and communicates state information
   to the fuzzer through shared memory.
*/

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "../gfuzz-config.h"

/* Shared memory for variable states */
uint32_t __gfuzz_var_states[GFUZZ_VAR_MAP_SIZE];

/* State tracking enabled flag */
static uint8_t gfuzz_enabled = 0;

/* Initialize GFuzz runtime */
static void __attribute__((constructor)) __gfuzz_init(void) {
  /* Check if GFuzz is enabled */
  char *gfuzz_env = getenv("GFUZZ_ENABLED");
  if (gfuzz_env && atoi(gfuzz_env) == 1) {
    gfuzz_enabled = 1;
  }
  
  /* Initialize state map */
  memset(__gfuzz_var_states, 0, sizeof(__gfuzz_var_states));
  
#if GFUZZ_DEBUG
  if (gfuzz_enabled) {
    fprintf(stderr, "[GFuzz Runtime] Initialized with map size %d\n", 
            GFUZZ_VAR_MAP_SIZE);
  }
#endif
}

/* Record numeric variable state */
void __gfuzz_record_numeric(uint32_t var_id, uint32_t value) {
  if (!gfuzz_enabled) return;
  
  if (var_id < GFUZZ_VAR_MAP_SIZE) {
    __gfuzz_var_states[var_id] = value;
    
#if GFUZZ_LOG_STATES
    fprintf(stderr, "[GFuzz] Var %u = %u\n", var_id, value);
#endif
  }
}

/* Record char variable state */
void __gfuzz_record_char(uint32_t var_id, uint8_t value) {
  if (!gfuzz_enabled) return;
  
  if (var_id < GFUZZ_VAR_MAP_SIZE) {
    /* Store char in lower byte, use upper bits for type info */
    __gfuzz_var_states[var_id] = (uint32_t)value | 0x01000000;
    
#if GFUZZ_LOG_STATES
    fprintf(stderr, "[GFuzz] Var %u = '%c' (0x%02x)\n", var_id, value, value);
#endif
  }
}

/* Record string variable state */
void __gfuzz_record_string(uint32_t var_id, const char* value) {
  if (!gfuzz_enabled || !value) return;
  
  if (var_id < GFUZZ_VAR_MAP_SIZE) {
    /* Compute a hash of the string for compact representation */
    uint32_t hash = 5381;
    const char *str = value;
    int c;
    int len = 0;
    
    while ((c = *str++) && len < GFUZZ_MAX_STRING_LEN) {
      hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
      len++;
    }
    
    /* Store hash in lower 24 bits, length info in upper byte */
    uint32_t len_encoded = (len > 255) ? 255 : len;
    __gfuzz_var_states[var_id] = (hash & 0x00FFFFFF) | (len_encoded << 24);
    
#if GFUZZ_LOG_STATES
    fprintf(stderr, "[GFuzz] Var %u = \"%s\" (hash=0x%08x, len=%d)\n", 
            var_id, value, hash, len);
#endif
  }
}

/* Record pointer variable state */
void __gfuzz_record_pointer(uint32_t var_id, const void* value) {
  if (!gfuzz_enabled) return;
  
  if (var_id < GFUZZ_VAR_MAP_SIZE) {
    /* Store lower 32 bits of pointer address */
    uintptr_t ptr_val = (uintptr_t)value;
    __gfuzz_var_states[var_id] = (uint32_t)(ptr_val & 0xFFFFFFFF);
    
#if GFUZZ_LOG_STATES
    fprintf(stderr, "[GFuzz] Var %u = %p\n", var_id, value);
#endif
  }
}

/* Get current state snapshot (for fuzzer to read) */
void __gfuzz_get_state_snapshot(uint32_t *buffer, uint32_t size) {
  if (!gfuzz_enabled || !buffer) return;
  
  uint32_t copy_size = (size < GFUZZ_VAR_MAP_SIZE) ? size : GFUZZ_VAR_MAP_SIZE;
  memcpy(buffer, __gfuzz_var_states, copy_size * sizeof(uint32_t));
}

/* Reset state map (called between test cases) */
void __gfuzz_reset_states(void) {
  if (!gfuzz_enabled) return;
  
  memset(__gfuzz_var_states, 0, sizeof(__gfuzz_var_states));
}

/* Statistics collection (if enabled) */
#if GFUZZ_STATS

static uint64_t gfuzz_total_updates = 0;
static uint32_t gfuzz_active_vars = 0;

void __gfuzz_update_stats(void) {
  gfuzz_total_updates++;
  
  /* Count active variables */
  uint32_t active = 0;
  for (uint32_t i = 0; i < GFUZZ_VAR_MAP_SIZE; i++) {
    if (__gfuzz_var_states[i] != 0) {
      active++;
    }
  }
  gfuzz_active_vars = active;
}

void __gfuzz_print_stats(void) {
  fprintf(stderr, "[GFuzz Stats] Total updates: %llu, Active vars: %u\n",
          (unsigned long long)gfuzz_total_updates, gfuzz_active_vars);
}

static void __attribute__((destructor)) __gfuzz_cleanup(void) {
  if (gfuzz_enabled) {
    __gfuzz_print_stats();
  }
}

#endif // GFUZZ_STATS
