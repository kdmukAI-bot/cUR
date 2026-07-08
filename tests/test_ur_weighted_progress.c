/*
 * test_ur_weighted_progress.c
 *
 * Exercises ur_decoder_estimated_percent_complete_weighted() against real
 * multi-fragment streams and asserts its invariants after every frame:
 * the value stays in [0, 0.99] while the decode is incomplete, never
 * decreases, and reads exactly 1.0 once the decoder completes.
 */

#include "../src/ur_decoder.h"
#include "test_harness.h"
#include "test_utils.h"
#include <stdio.h>
#include <stdlib.h>

#define TEST_CASES_DIR "tests/test_cases/bytes"

static bool test_file(const char *filepath) {
  printf("\n=== Testing file: %s ===\n", filepath);

  int fragment_count = 0;
  char **fragments = read_fragments_from_file(filepath, &fragment_count);
  if (!fragments || fragment_count == 0) {
    fprintf(stderr, "❌ No fragments found in file: %s\n", filepath);
    return false;
  }

  ur_decoder_t *decoder = ur_decoder_new();
  if (!decoder) {
    fprintf(stderr, "❌ Failed to create UR decoder\n");
    free_fragments(fragments, fragment_count);
    return false;
  }

  bool ok = true;
  if (ur_decoder_estimated_percent_complete_weighted(decoder) != 0.0f) {
    fprintf(stderr, "❌ Fresh decoder should report 0.0\n");
    ok = false;
  }

  float prev = 0.0f;
  int parts_used = 0;
  for (int i = 0; i < fragment_count && ok; i++) {
    ur_decoder_state_t state = ur_decoder_receive_part(decoder, fragments[i]);
    if (ur_decoder_state_is_error(state))
      continue;
    parts_used++;

    float weighted = ur_decoder_estimated_percent_complete_weighted(decoder);
    bool complete = ur_decoder_state_is_terminal(state);

    if (weighted < 0.0f || weighted > 1.0f) {
      fprintf(stderr, "❌ Frame %d: %.6f outside [0, 1]\n", i, weighted);
      ok = false;
    }
    // Must compare against 0.99f: the clamp returns the float literal, and
    // (double)0.99f > 0.99, so a double literal here would false-fail.
    if (!complete && weighted > 0.99f) {
      fprintf(stderr, "❌ Frame %d: %.6f > 0.99 while incomplete\n", i,
              weighted);
      ok = false;
    }
    if (complete && weighted != 1.0f) {
      fprintf(stderr, "❌ Frame %d: complete but reports %.6f\n", i, weighted);
      ok = false;
    }
    // Epsilon sized for float: worst-case rounding jitter of the ~150-term
    // score sums is ~1e-5, while a genuine regression shifts the estimate by
    // at least one score quantum (1/seq_len)/seq_len ≈ 4e-5 on the largest
    // test vector.
    if (weighted + 1e-5f < prev) {
      fprintf(stderr, "❌ Frame %d: decreased from %.6f to %.6f\n", i, prev,
              weighted);
      ok = false;
    }
    prev = weighted;

    if (complete)
      break;
  }

  if (ok && ur_decoder_get_state(decoder) != UR_DECODER_OK) {
    fprintf(stderr, "❌ Decode did not complete after %d parts\n", parts_used);
    ok = false;
  }
  if (ok) {
    printf("✅ PASS - invariants held across %d parts (final %.4f)\n",
           parts_used, prev);
  }

  ur_decoder_free(decoder);
  free_fragments(fragments, fragment_count);
  return ok;
}

int main(int argc, char *argv[]) {
  if (ur_decoder_estimated_percent_complete_weighted(NULL) != 0.0f) {
    fprintf(stderr, "❌ NULL decoder should report 0.0\n");
    return 1;
  }
  return run_test_suite(argc, argv, "UR Weighted Progress Test", TEST_CASES_DIR,
                        ".UR_fragments.txt", test_file);
}
