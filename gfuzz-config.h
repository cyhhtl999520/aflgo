/*
   GFuzz - Configuration parameters for Variable State Diversity-Guided Fuzzing
   ----------------------------------------------------------------------------
   
   Based on the research paper:
   "Variable State Diversity-Guided Fuzzing Method"
   
   This file contains all tunable parameters for GFuzz.
*/

#ifndef _GFUZZ_CONFIG_H
#define _GFUZZ_CONFIG_H

/* Enable GFuzz variable state monitoring */
#define GFUZZ_ENABLED 1

/* Algorithm 1: Key Variable Identification Parameters */

/* Distance threshold for function call distance (h in paper) */
#define GFUZZ_DISTANCE_THRESHOLD 3

/* Maximum number of key variables to track */
#define GFUZZ_MAX_KEY_VARS 1024

/* Algorithm 3: State Diversity Evaluation Parameters */

/* Weights for different variable types in similarity calculation */
#define GFUZZ_WEIGHT_NUMERIC    1.0
#define GFUZZ_WEIGHT_CHAR       0.8
#define GFUZZ_WEIGHT_STRING     1.2

/* String similarity calculation parameters */
#define GFUZZ_STRING_LEV_ALPHA  0.6    /* α in paper */
#define GFUZZ_STRING_LEN_BETA   0.4    /* β in paper */

/* Diversity score combination weights */
#define GFUZZ_WEIGHT_SIMILARITY 0.6    /* w_sim in paper */
#define GFUZZ_WEIGHT_COVERAGE   0.4    /* w_cov in paper */

/* Algorithm 4: Adaptive Seed Scheduling Parameters */

/* Learning rate for weight adaptation (λ in paper) */
#define GFUZZ_LEARNING_RATE     0.1

/* Energy coefficient for state score (γ in paper) */
#define GFUZZ_ENERGY_COEFF      0.5

/* Initial weights for traditional vs state score */
#define GFUZZ_INIT_WEIGHT_TRAD  0.5
#define GFUZZ_INIT_WEIGHT_STATE 0.5

/* Variable State Storage Parameters */

/* Size of variable state map (must be power of 2) */
#define GFUZZ_VAR_MAP_SIZE_POW2  14
#define GFUZZ_VAR_MAP_SIZE       (1 << GFUZZ_VAR_MAP_SIZE_POW2)

/* Maximum string length to track */
#define GFUZZ_MAX_STRING_LEN     256

/* Variable type definitions */
#define GFUZZ_VAR_TYPE_NUMERIC   0
#define GFUZZ_VAR_TYPE_CHAR      1
#define GFUZZ_VAR_TYPE_STRING    2
#define GFUZZ_VAR_TYPE_POINTER   3

/* State history size for diversity calculation */
#define GFUZZ_STATE_HISTORY_SIZE 100

/* Minimum coverage threshold for normalization */
#define GFUZZ_MIN_COVERAGE       0.0
#define GFUZZ_MAX_COVERAGE       1.0

/* Performance tuning */

/* Instrumentation sampling rate (0-100) */
#define GFUZZ_INST_RATIO         100

/* Enable state tracking only for critical paths */
#define GFUZZ_CRITICAL_PATH_ONLY 0

/* Debug and logging options */

/* Enable verbose logging */
#define GFUZZ_DEBUG              0

/* Enable state tracking statistics */
#define GFUZZ_STATS              1

/* Log state changes to file */
#define GFUZZ_LOG_STATES         0

#endif /* _GFUZZ_CONFIG_H */
