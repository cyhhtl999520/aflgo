/*
   GFuzz - State Diversity Evaluation and Adaptive Scheduling
   -----------------------------------------------------------
   
   Implements Algorithm 3 (State Diversity Evaluation) and 
   Algorithm 4 (Adaptive Seed Scheduling) from the paper.
*/

#ifndef GFUZZ_DIVERSITY_H
#define GFUZZ_DIVERSITY_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "gfuzz-config.h"

/* State snapshot for a test case */
typedef struct {
  uint32_t states[GFUZZ_VAR_MAP_SIZE];
  uint32_t valid_count;      /* Number of non-zero states */
  double similarity_score;
  double coverage_score;
  double diversity_score;
} gfuzz_state_snapshot_t;

/* State history for diversity calculation */
typedef struct {
  gfuzz_state_snapshot_t snapshots[GFUZZ_STATE_HISTORY_SIZE];
  uint32_t count;
  uint32_t head;
  uint32_t total_vars;        /* Total number of tracked variables */
  double min_coverage;
  double max_coverage;
} gfuzz_state_history_t;

/* Adaptive weights for scheduling */
typedef struct {
  double weight_traditional;
  double weight_state;
  double learning_rate;
  double coverage_gain;
  double state_gain;
} gfuzz_adaptive_weights_t;

/* Initialize state history */
static inline void gfuzz_init_state_history(gfuzz_state_history_t *history, 
                                           uint32_t total_vars) {
  memset(history, 0, sizeof(gfuzz_state_history_t));
  history->total_vars = total_vars;
  history->min_coverage = GFUZZ_MIN_COVERAGE;
  history->max_coverage = GFUZZ_MAX_COVERAGE;
}

/* Initialize adaptive weights */
static inline void gfuzz_init_weights(gfuzz_adaptive_weights_t *weights) {
  weights->weight_traditional = GFUZZ_INIT_WEIGHT_TRAD;
  weights->weight_state = GFUZZ_INIT_WEIGHT_STATE;
  weights->learning_rate = GFUZZ_LEARNING_RATE;
  weights->coverage_gain = 0.0;
  weights->state_gain = 0.0;
}

/* Calculate Levenshtein distance for strings (simplified version) */
static inline uint32_t gfuzz_levenshtein_distance(uint32_t hash1, uint32_t hash2) {
  /* Since we store hashes, we use Hamming distance as approximation */
  uint32_t xor_val = hash1 ^ hash2;
  uint32_t dist = 0;
  
  while (xor_val) {
    dist += xor_val & 1;
    xor_val >>= 1;
  }
  
  return dist;
}

/* Calculate similarity between two variable states based on type */
static inline double gfuzz_calculate_var_similarity(uint32_t current, uint32_t historical) {
  if (current == 0 || historical == 0) {
    return 0.0; /* No valid comparison */
  }
  
  /* Check if it's a char type (bit 24 set) */
  if ((current & 0x01000000) && (historical & 0x01000000)) {
    /* Character type - use ASCII distance */
    uint8_t c1 = current & 0xFF;
    uint8_t c2 = historical & 0xFF;
    double diff = fabs((double)c1 - (double)c2) / 127.0;
    return diff * GFUZZ_WEIGHT_CHAR;
  }
  
  /* Check if it's a string (upper byte is length) */
  uint32_t len1 = (current >> 24) & 0xFF;
  uint32_t len2 = (historical >> 24) & 0xFF;
  
  if (len1 > 0 && len2 > 0 && len1 < 255 && len2 < 255) {
    /* String type - use hash distance and length difference */
    uint32_t hash1 = current & 0x00FFFFFF;
    uint32_t hash2 = historical & 0x00FFFFFF;
    
    uint32_t lev_dist = gfuzz_levenshtein_distance(hash1, hash2);
    uint32_t min_len = (len1 < len2) ? len1 : len2;
    uint32_t len_diff = (len1 > len2) ? (len1 - len2) : (len2 - len1);
    
    double lev_sim = (min_len > 0) ? ((double)lev_dist / min_len) : 0.0;
    double len_sim = ((double)len_diff / 255.0);
    
    double str_sim = lev_sim * GFUZZ_STRING_LEV_ALPHA + 
                     len_sim * GFUZZ_STRING_LEN_BETA;
    
    return str_sim * GFUZZ_WEIGHT_STRING;
  }
  
  /* Numeric type - simple equality check */
  double diff = (current == historical) ? 0.0 : 1.0;
  return diff * GFUZZ_WEIGHT_NUMERIC;
}

/* Algorithm 3: State Diversity Evaluation */
static inline double gfuzz_evaluate_state_diversity(
    const gfuzz_state_snapshot_t *current,
    const gfuzz_state_history_t *history) {
  
  if (history->count == 0) {
    return 1.0; /* Maximum diversity for first input */
  }
  
  double similarity_score = 0.0;
  uint32_t comparison_count = 0;
  
  /* Calculate similarity with historical states (lines 2-21) */
  for (uint32_t h = 0; h < history->count; h++) {
    const gfuzz_state_snapshot_t *hist_snap = &history->snapshots[h];
    
    for (uint32_t v = 0; v < GFUZZ_VAR_MAP_SIZE; v++) {
      if (current->states[v] != 0) {
        double var_sim = gfuzz_calculate_var_similarity(
            current->states[v], 
            hist_snap->states[v]);
        
        similarity_score += var_sim;
        comparison_count++;
      }
    }
  }
  
  /* Normalize similarity score */
  if (comparison_count > 0) {
    similarity_score /= comparison_count;
  }
  
  /* Calculate coverage score (lines 23-27) */
  double coverage_ratio = (history->total_vars > 0) ? 
      ((double)current->valid_count / history->total_vars) : 0.0;
  
  double coverage_score = (coverage_ratio - history->min_coverage) / 
      (history->max_coverage - history->min_coverage + 0.0001);
  
  /* Clamp coverage score */
  if (coverage_score < 0.0) coverage_score = 0.0;
  if (coverage_score > 1.0) coverage_score = 1.0;
  
  /* Combine scores (line 30) */
  double diversity_score = 
      GFUZZ_WEIGHT_SIMILARITY * similarity_score + 
      GFUZZ_WEIGHT_COVERAGE * coverage_score;
  
  return diversity_score;
}

/* Add snapshot to history */
static inline void gfuzz_add_to_history(gfuzz_state_history_t *history,
                                       const gfuzz_state_snapshot_t *snapshot) {
  if (history->count < GFUZZ_STATE_HISTORY_SIZE) {
    memcpy(&history->snapshots[history->count], snapshot, 
           sizeof(gfuzz_state_snapshot_t));
    history->count++;
  } else {
    /* Ring buffer - overwrite oldest */
    memcpy(&history->snapshots[history->head], snapshot,
           sizeof(gfuzz_state_snapshot_t));
    history->head = (history->head + 1) % GFUZZ_STATE_HISTORY_SIZE;
  }
  
  /* Update coverage bounds */
  double cov = (history->total_vars > 0) ? 
      ((double)snapshot->valid_count / history->total_vars) : 0.0;
  
  if (cov < history->min_coverage) history->min_coverage = cov;
  if (cov > history->max_coverage) history->max_coverage = cov;
}

/* Create state snapshot from current state map */
static inline void gfuzz_create_snapshot(gfuzz_state_snapshot_t *snapshot,
                                        const uint32_t *state_map) {
  memcpy(snapshot->states, state_map, sizeof(snapshot->states));
  
  /* Count valid (non-zero) states */
  snapshot->valid_count = 0;
  for (uint32_t i = 0; i < GFUZZ_VAR_MAP_SIZE; i++) {
    if (state_map[i] != 0) {
      snapshot->valid_count++;
    }
  }
  
  snapshot->similarity_score = 0.0;
  snapshot->coverage_score = 0.0;
  snapshot->diversity_score = 0.0;
}

/* Algorithm 4: Adaptive Weight Update */
static inline void gfuzz_update_adaptive_weights(
    gfuzz_adaptive_weights_t *weights,
    double new_cov_gain,
    double new_state_gain) {
  
  weights->coverage_gain = new_cov_gain;
  weights->state_gain = new_state_gain;
  
  /* Update weights based on relative gains */
  double lambda = weights->learning_rate;
  double gain_diff = weights->coverage_gain - weights->state_gain;
  
  weights->weight_traditional += lambda * gain_diff;
  weights->weight_state += lambda * (weights->state_gain - weights->coverage_gain);
  
  /* Normalize weights to sum to 1.0 */
  double sum = weights->weight_traditional + weights->weight_state;
  if (sum > 0.0) {
    weights->weight_traditional /= sum;
    weights->weight_state /= sum;
  }
  
  /* Clamp weights to reasonable range */
  if (weights->weight_traditional < 0.1) weights->weight_traditional = 0.1;
  if (weights->weight_traditional > 0.9) weights->weight_traditional = 0.9;
  
  weights->weight_state = 1.0 - weights->weight_traditional;
}

/* Calculate combined score for seed selection (part of Algorithm 4) */
static inline double gfuzz_calculate_combined_score(
    double traditional_score,
    double state_diversity_score,
    const gfuzz_adaptive_weights_t *weights) {
  
  return weights->weight_traditional * traditional_score +
         weights->weight_state * state_diversity_score;
}

/* Calculate energy multiplier based on state score (Algorithm 4, line 18) */
static inline double gfuzz_calculate_energy_multiplier(double state_score) {
  return 1.0 + GFUZZ_ENERGY_COEFF * state_score;
}

#endif /* GFUZZ_DIVERSITY_H */
