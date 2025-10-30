/*
   GFuzz - AFL-Fuzz Integration
   -----------------------------
   
   This header provides integration points for GFuzz state diversity
   tracking in afl-fuzz.c. It extends the fuzzer with:
   - State snapshot storage in queue entries
   - State diversity evaluation during seed selection
   - Adaptive weight management
   - State-based energy boosting
   
   To integrate: Include this file in afl-fuzz.c and call the
   provided functions at appropriate points.
*/

#ifndef GFUZZ_AFL_INTEGRATION_H
#define GFUZZ_AFL_INTEGRATION_H

#include "../gfuzz-config.h"
#include "../gfuzz-diversity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>

/* ==================== Data Structures ==================== */

/* GFuzz additions to queue_entry (add to struct in afl-fuzz.c) */
typedef struct gfuzz_queue_data {
  gfuzz_state_snapshot_t state_snapshot;  /* Variable states for this input */
  double state_diversity_score;           /* Computed diversity score */
  uint8_t state_captured;                 /* Has state been captured? */
} gfuzz_queue_data_t;

/* Global GFuzz state */
typedef struct gfuzz_context {
  /* State history for diversity calculation */
  gfuzz_state_history_t history;
  
  /* Adaptive weights */
  gfuzz_adaptive_weights_t weights;
  
  /* Shared memory for variable states */
  uint32_t *var_state_shm;
  int var_state_shm_id;
  
  /* Statistics */
  uint64_t total_state_evaluations;
  uint64_t total_state_updates;
  double last_coverage_gain;
  double last_state_gain;
  
  /* Configuration */
  uint8_t enabled;
  uint32_t num_key_vars;
  
} gfuzz_context_t;

/* ==================== Global Instance ==================== */

static gfuzz_context_t *gfuzz_ctx = NULL;

/* ==================== Initialization ==================== */

/* Initialize GFuzz context */
static void gfuzz_init(uint32_t num_key_vars) {
  char *gfuzz_env = getenv("GFUZZ_ENABLED");
  
  if (!gfuzz_env || atoi(gfuzz_env) != 1) {
    return; /* GFuzz disabled */
  }
  
  gfuzz_ctx = (gfuzz_context_t*)calloc(1, sizeof(gfuzz_context_t));
  if (!gfuzz_ctx) {
    FATAL("Unable to allocate memory for GFuzz context");
  }
  
  gfuzz_ctx->enabled = 1;
  gfuzz_ctx->num_key_vars = num_key_vars;
  
  /* Initialize history */
  gfuzz_init_state_history(&gfuzz_ctx->history, num_key_vars);
  
  /* Initialize adaptive weights */
  gfuzz_init_weights(&gfuzz_ctx->weights);
  
  /* Attach to variable state shared memory */
  char *shm_str = getenv("GFUZZ_SHM_ID");
  if (shm_str) {
    gfuzz_ctx->var_state_shm_id = atoi(shm_str);
    gfuzz_ctx->var_state_shm = (uint32_t*)shmat(gfuzz_ctx->var_state_shm_id, NULL, 0);
    
    if (gfuzz_ctx->var_state_shm == (void*)-1) {
      WARNF("Could not attach to GFuzz state shared memory");
      gfuzz_ctx->var_state_shm = NULL;
    }
  } else {
    gfuzz_ctx->var_state_shm = NULL;
  }
  
  OKF("GFuzz initialized: tracking %u key variables", num_key_vars);
}

/* Cleanup GFuzz resources */
static void gfuzz_cleanup(void) {
  if (!gfuzz_ctx) return;
  
  if (gfuzz_ctx->var_state_shm) {
    shmdt(gfuzz_ctx->var_state_shm);
  }
  
  free(gfuzz_ctx);
  gfuzz_ctx = NULL;
}

/* ==================== State Capture ==================== */

/* Capture variable states after test case execution */
static void gfuzz_capture_state(gfuzz_queue_data_t *qdata) {
  if (!gfuzz_ctx || !gfuzz_ctx->enabled) return;
  if (!gfuzz_ctx->var_state_shm) return;
  if (!qdata) return;
  
  /* Create snapshot from shared memory */
  gfuzz_create_snapshot(&qdata->state_snapshot, gfuzz_ctx->var_state_shm);
  qdata->state_captured = 1;
  
  gfuzz_ctx->total_state_updates++;
}

/* ==================== Diversity Evaluation ==================== */

/* Evaluate state diversity for a test case */
static double gfuzz_evaluate_diversity(gfuzz_queue_data_t *qdata) {
  if (!gfuzz_ctx || !gfuzz_ctx->enabled) return 0.0;
  if (!qdata || !qdata->state_captured) return 0.0;
  
  /* Compute diversity score */
  double diversity = gfuzz_evaluate_state_diversity(
      &qdata->state_snapshot,
      &gfuzz_ctx->history);
  
  qdata->state_diversity_score = diversity;
  gfuzz_ctx->total_state_evaluations++;
  
  /* Add to history if interesting */
  if (diversity > 0.3) { /* Threshold for "interesting" states */
    gfuzz_add_to_history(&gfuzz_ctx->history, &qdata->state_snapshot);
  }
  
  return diversity;
}

/* ==================== Seed Scheduling ==================== */

/* Calculate combined score for seed selection (Algorithm 4) */
static double gfuzz_calculate_seed_score(double traditional_score,
                                         gfuzz_queue_data_t *qdata) {
  if (!gfuzz_ctx || !gfuzz_ctx->enabled) {
    return traditional_score;
  }
  
  double state_score = 0.0;
  if (qdata && qdata->state_captured) {
    state_score = qdata->state_diversity_score;
  }
  
  return gfuzz_calculate_combined_score(
      traditional_score,
      state_score,
      &gfuzz_ctx->weights);
}

/* Calculate energy multiplier based on state diversity */
static double gfuzz_get_energy_multiplier(gfuzz_queue_data_t *qdata) {
  if (!gfuzz_ctx || !gfuzz_ctx->enabled) {
    return 1.0;
  }
  
  if (!qdata || !qdata->state_captured) {
    return 1.0;
  }
  
  return gfuzz_calculate_energy_multiplier(qdata->state_diversity_score);
}

/* ==================== Adaptive Weight Update ==================== */

/* Update adaptive weights based on fuzzing progress */
static void gfuzz_update_weights(double coverage_gain, double state_gain) {
  if (!gfuzz_ctx || !gfuzz_ctx->enabled) return;
  
  gfuzz_ctx->last_coverage_gain = coverage_gain;
  gfuzz_ctx->last_state_gain = state_gain;
  
  gfuzz_update_adaptive_weights(&gfuzz_ctx->weights, coverage_gain, state_gain);
  
#if GFUZZ_DEBUG
  fprintf(stderr, "[GFuzz] Weights updated: trad=%.3f, state=%.3f (cov_gain=%.3f, state_gain=%.3f)\n",
          gfuzz_ctx->weights.weight_traditional,
          gfuzz_ctx->weights.weight_state,
          coverage_gain, state_gain);
#endif
}

/* ==================== Statistics ==================== */

/* Print GFuzz statistics */
static void gfuzz_print_stats(void) {
  if (!gfuzz_ctx || !gfuzz_ctx->enabled) return;
  
  SAYF("\n");
  SAYF(cGRA "    GFuzz statistics " cRST "\n");
  SAYF(cGRA "   ----------------------\n" cRST);
  SAYF("   State evaluations : " cRST "%llu\n", 
       (unsigned long long)gfuzz_ctx->total_state_evaluations);
  SAYF("   State updates     : " cRST "%llu\n",
       (unsigned long long)gfuzz_ctx->total_state_updates);
  SAYF("   History size      : " cRST "%u\n",
       gfuzz_ctx->history.count);
  SAYF("   Active variables  : " cRST "%u / %u\n",
       gfuzz_ctx->history.count > 0 ? 
       gfuzz_ctx->history.snapshots[0].valid_count : 0,
       gfuzz_ctx->num_key_vars);
  SAYF("   Weight (trad)     : " cRST "%.3f\n",
       gfuzz_ctx->weights.weight_traditional);
  SAYF("   Weight (state)    : " cRST "%.3f\n",
       gfuzz_ctx->weights.weight_state);
  SAYF("\n");
}

/* ==================== Helper Macros ==================== */

/* Check if GFuzz is enabled */
#define GFUZZ_IS_ENABLED() (gfuzz_ctx && gfuzz_ctx->enabled)

/* Get GFuzz data pointer for a queue entry */
#define GFUZZ_QUEUE_DATA(q) ((gfuzz_queue_data_t*)((q)->gfuzz_data))

/* ==================== Integration Instructions ==================== */

/*
 * To integrate GFuzz into afl-fuzz.c:
 *
 * 1. Add to struct queue_entry:
 *    void *gfuzz_data;
 *
 * 2. In setup_shm():
 *    Add after AFL shared memory setup:
 *    gfuzz_init(GFUZZ_MAX_KEY_VARS);
 *
 * 3. In add_to_queue() or save_if_interesting():
 *    After creating new queue entry:
 *    if (GFUZZ_IS_ENABLED()) {
 *      q->gfuzz_data = calloc(1, sizeof(gfuzz_queue_data_t));
 *    }
 *
 * 4. In run_target() after execution:
 *    if (GFUZZ_IS_ENABLED() && q->gfuzz_data) {
 *      gfuzz_capture_state(GFUZZ_QUEUE_DATA(q));
 *      gfuzz_evaluate_diversity(GFUZZ_QUEUE_DATA(q));
 *    }
 *
 * 5. In calculate_score():
 *    Before returning perf_score:
 *    if (GFUZZ_IS_ENABLED() && q->gfuzz_data) {
 *      double multiplier = gfuzz_get_energy_multiplier(GFUZZ_QUEUE_DATA(q));
 *      perf_score *= multiplier;
 *    }
 *
 * 6. In main fuzzing loop (periodically):
 *    Every N cycles, call:
 *    if (GFUZZ_IS_ENABLED()) {
 *      double cov_gain = (new_coverage - old_coverage) / old_coverage;
 *      double state_gain = (new_paths - old_paths) / old_paths;
 *      gfuzz_update_weights(cov_gain, state_gain);
 *    }
 *
 * 7. In show_stats() or similar:
 *    Add:
 *    gfuzz_print_stats();
 *
 * 8. In cleanup:
 *    Add:
 *    gfuzz_cleanup();
 *
 * 9. Free gfuzz_data in queue entries:
 *    In destroy_queue():
 *    if (q->gfuzz_data) free(q->gfuzz_data);
 */

#endif /* GFUZZ_AFL_INTEGRATION_H */
