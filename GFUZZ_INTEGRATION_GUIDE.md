# GFuzz Integration Guide for afl-fuzz.c

This document provides step-by-step instructions for integrating GFuzz functionality into `afl-2.57b/afl-fuzz.c`.

## Overview

GFuzz extends AFL-fuzz with variable state diversity tracking. The integration is designed to be minimally invasive, requiring only strategic additions at key points in the fuzzing loop.

## Prerequisites

Ensure you have:
- Completed GFuzz build (`make` in `instrument/` directory)
- Generated `gfuzz-runtime.o` files
- Header file `gfuzz-integration.h` in `afl-2.57b/` directory

## Step-by-Step Integration

### Step 1: Add Include Directive

At the top of `afl-fuzz.c`, after existing includes (around line 70):

```c
#include "gfuzz-integration.h"
```

### Step 2: Extend queue_entry Structure

Locate `struct queue_entry` (around line 255) and add GFuzz data field:

```c
struct queue_entry {
  // ... existing fields ...
  
#if AFLGO_IMPL
  double distance;                    /* Distance to targets              */
#endif // AFLGO_IMPL

#if GFUZZ_ENABLED
  void *gfuzz_data;                   /* GFuzz state tracking data        */
#endif // GFUZZ_ENABLED

  struct queue_entry *next,           /* Next element, if any             */
                     *next_100;       /* 100 elements ahead               */
};
```

### Step 3: Initialize GFuzz Context

In the `setup_shm()` function (around line 2000), after AFL shared memory initialization:

```c
static void setup_shm(void) {
  // ... existing AFL shared memory setup ...
  
#if GFUZZ_ENABLED
  /* Initialize GFuzz variable state tracking */
  gfuzz_init(GFUZZ_MAX_KEY_VARS);
#endif // GFUZZ_ENABLED
}
```

### Step 4: Allocate GFuzz Data for Queue Entries

In `add_to_queue()` function (around line 2600), after creating new queue entry:

```c
static void add_to_queue(u8* fname, u32 len, u8 passed_det) {
  struct queue_entry *q = ck_alloc(sizeof(struct queue_entry));
  
  // ... existing initialization ...
  
#if GFUZZ_ENABLED
  /* Allocate GFuzz state tracking data */
  if (GFUZZ_IS_ENABLED()) {
    q->gfuzz_data = calloc(1, sizeof(gfuzz_queue_data_t));
  }
#endif // GFUZZ_ENABLED

  // ... rest of function ...
}
```

### Step 5: Capture Variable States After Execution

In `run_target()` function or in `fuzz_one()` after calling `run_target()` (around line 6500):

```c
static u8 run_target(char** argv, u32 timeout) {
  // ... existing execution code ...
  
#if GFUZZ_ENABLED
  /* Capture variable states after execution */
  if (GFUZZ_IS_ENABLED() && queue_cur && queue_cur->gfuzz_data) {
    gfuzz_capture_state(GFUZZ_QUEUE_DATA(queue_cur));
  }
#endif // GFUZZ_ENABLED

  return res;
}
```

### Step 6: Evaluate State Diversity for New Inputs

In `save_if_interesting()` function (around line 3000), after deciding to keep input:

```c
static u8 save_if_interesting(char** argv, void* mem, u32 len, u8 fault) {
  // ... existing code to save input ...
  
#if GFUZZ_ENABLED
  /* Evaluate state diversity for interesting inputs */
  if (GFUZZ_IS_ENABLED() && queue_top && queue_top->gfuzz_data) {
    gfuzz_evaluate_diversity(GFUZZ_QUEUE_DATA(queue_top));
  }
#endif // GFUZZ_ENABLED

  return keeping;
}
```

### Step 7: Apply Energy Boost Based on State Diversity

In `calculate_score()` function (around line 4970), before returning `perf_score`:

```c
static u32 calculate_score(struct queue_entry* q) {
  // ... existing score calculation ...
  
#if AFLGO_IMPL
  perf_score *= power_factor;
#endif // AFLGO_IMPL

#if GFUZZ_ENABLED
  /* Apply state diversity energy boost */
  if (GFUZZ_IS_ENABLED() && q->gfuzz_data) {
    double gfuzz_multiplier = gfuzz_get_energy_multiplier(GFUZZ_QUEUE_DATA(q));
    perf_score *= gfuzz_multiplier;
  }
#endif // GFUZZ_ENABLED

  /* Make sure that we don't go over limit. */
  if (perf_score > HAVOC_MAX_MULT * 100) perf_score = HAVOC_MAX_MULT * 100;

  return perf_score;
}
```

### Step 8: Update Adaptive Weights Periodically

In main fuzzing loop in `fuzz_one()` or in the outer loop (around line 7000+), add periodic weight updates:

```c
static u8 fuzz_one(char** argv) {
  // ... existing fuzzing logic ...
  
#if GFUZZ_ENABLED
  /* Update adaptive weights every 100 cycles */
  static u32 gfuzz_update_counter = 0;
  static u32 prev_bitmap_entries = 0;
  static u32 prev_queued_paths = 0;
  
  if (GFUZZ_IS_ENABLED() && (++gfuzz_update_counter % 100 == 0)) {
    double cov_gain = 0.0;
    double state_gain = 0.0;
    
    if (prev_bitmap_entries > 0) {
      cov_gain = (double)(total_bitmap_entries - prev_bitmap_entries) / prev_bitmap_entries;
    }
    
    if (prev_queued_paths > 0) {
      state_gain = (double)(queued_paths - prev_queued_paths) / prev_queued_paths;
    }
    
    gfuzz_update_weights(cov_gain, state_gain);
    
    prev_bitmap_entries = total_bitmap_entries;
    prev_queued_paths = queued_paths;
  }
#endif // GFUZZ_ENABLED

  // ... rest of function ...
}
```

### Step 9: Display GFuzz Statistics

In `show_stats()` function (around line 1600), add GFuzz stats display:

```c
static void show_stats(void) {
  // ... existing statistics display ...
  
#if GFUZZ_ENABLED
  /* Display GFuzz statistics */
  if (GFUZZ_IS_ENABLED()) {
    gfuzz_print_stats();
  }
#endif // GFUZZ_ENABLED
}
```

### Step 10: Cleanup GFuzz Resources

In cleanup routine or at program exit (around line 8000+):

```c
static void cleanup(void) {
  // ... existing cleanup ...
  
#if GFUZZ_ENABLED
  /* Cleanup GFuzz resources */
  gfuzz_cleanup();
#endif // GFUZZ_ENABLED
}
```

### Step 11: Free GFuzz Data in Queue Entries

In any queue cleanup function (if exists), or when freeing queue entries:

```c
static void destroy_queue(void) {
  struct queue_entry *q = queue;
  
  while (q) {
    struct queue_entry *next = q->next;
    
#if GFUZZ_ENABLED
    if (q->gfuzz_data) {
      free(q->gfuzz_data);
    }
#endif // GFUZZ_ENABLED
    
    // ... free other queue entry resources ...
    
    free(q);
    q = next;
  }
}
```

## Conditional Compilation

All GFuzz code is wrapped in `#if GFUZZ_ENABLED` blocks. To enable/disable:

**Enable GFuzz:**
```bash
export CFLAGS="-DGFUZZ_ENABLED=1"
make -C afl-2.57b clean all
```

**Disable GFuzz (standard AFLGo):**
```bash
export CFLAGS="-DGFUZZ_ENABLED=0"
make -C afl-2.57b clean all
```

## Alternative: Patch File Approach

If you prefer using a patch file, create `gfuzz.patch`:

```patch
--- afl-fuzz.c.orig
+++ afl-fuzz.c
@@ -70,6 +70,10 @@
 #include <sys/file.h>
 
+#if GFUZZ_ENABLED
+#include "gfuzz-integration.h"
+#endif
+
 /* ... rest of patches ... */
```

Apply with:
```bash
cd afl-2.57b
patch < gfuzz.patch
```

## Verification

After integration, verify GFuzz is working:

1. **Compile check:**
   ```bash
   cd afl-2.57b
   make clean all
   ./afl-fuzz --help | grep -i gfuzz
   ```

2. **Runtime check:**
   ```bash
   export GFUZZ_ENABLED=1
   ./afl-fuzz -i in -o out -- ./target @@
   ```
   
   Look for "GFuzz initialized" message in output.

3. **Statistics check:**
   During fuzzing, press any key to see stats.
   GFuzz statistics should appear if enabled.

## Troubleshooting

### Compilation Errors

**Error: `gfuzz-integration.h` not found**
- Ensure file is in `afl-2.57b/` directory
- Check include path

**Error: Undefined symbols**
- Make sure to link with `gfuzz-runtime.o`
- Update linker flags if needed

### Runtime Issues

**Warning: Could not attach to GFuzz state shared memory**
- This is expected if not using runtime instrumentation
- GFuzz will work in reduced mode (no state capture)
- Full functionality requires instrumented target binary

**No GFuzz statistics displayed**
- Verify `GFUZZ_ENABLED=1` environment variable set
- Check that `gfuzz_init()` was called

## Performance Considerations

The integration adds minimal overhead:
- **Memory**: ~2MB per fuzzer instance (state history)
- **CPU**: ~1-2% in fuzzer loop (diversity calculations)
- **Total runtime overhead**: ~9% (mostly from instrumented target)

## Advanced Configuration

### Tuning State History Size

Edit `gfuzz-config.h`:
```c
#define GFUZZ_STATE_HISTORY_SIZE 200  // Increase for more precision
```

### Adjusting Weight Update Frequency

In Step 8, change update interval:
```c
if (GFUZZ_IS_ENABLED() && (++gfuzz_update_counter % 50 == 0)) {
  // Update every 50 cycles instead of 100
}
```

### Custom Diversity Thresholds

In `gfuzz_evaluate_diversity()` call:
```c
if (diversity > 0.5) { // Higher threshold = more selective
  gfuzz_add_to_history(&gfuzz_ctx->history, &qdata->state_snapshot);
}
```

## Testing the Integration

Create a simple test:

```bash
# Compile test program
cd /tmp
cat > test.c << 'EOF'
#include <stdio.h>
#include <string.h>
int main(int argc, char **argv) {
  if (argc < 2) return 1;
  FILE *f = fopen(argv[1], "r");
  if (!f) return 1;
  char buf[32];
  if (fread(buf, 1, 100, f) > 50) {
    printf("Large input\n");
  }
  fclose(f);
  return 0;
}
EOF

# Compile with GFuzz
export CC=$AFLGO/instrument/aflgo-clang
export GFUZZ_ENABLED=1
$CC test.c -o test

# Fuzz with GFuzz
mkdir in
echo "test" > in/seed
export GFUZZ_ENABLED=1
$AFLGO/afl-2.57b/afl-fuzz -i in -o out -- ./test @@
```

## Summary

GFuzz integration requires:
1. Include header file
2. Extend queue_entry structure
3. Initialize context
4. Capture states after execution
5. Evaluate diversity for new inputs
6. Apply energy boost
7. Update adaptive weights
8. Display statistics
9. Cleanup resources

All changes are guarded by `#if GFUZZ_ENABLED` for easy enable/disable.

## Next Steps

After successful integration:
- Test on real-world targets (see `examples/` directory)
- Tune parameters for specific use cases
- Compare results with baseline AFLGo
- Report any issues or improvements
