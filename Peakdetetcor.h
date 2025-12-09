// Peak detection for 1024-point spectrum on Arduino R4
// Returns up to 10 significant peaks with smoothing

#define FFT_SIZE 1024
#define MAX_PEAKS 10
#define SMOOTH_WINDOW 5        // Odd number recommended (3-9 typical)
#define MIN_PEAK_DISTANCE 15    // Minimum bins between peaks (adjust to taste)
#define PEAK_THRESHOLD_FACTOR 0.05  // Peaks must be at least this * avg above baseline

typedef struct {
  uint16_t bin;   // x: frequency bin (0 to 1023)
  uint16_t value; // y: magnitude (0 to 1023 typical)
} SpectrumPoint;

SpectrumPoint smoothed[FFT_SIZE];
SpectrumPoint peaks[MAX_PEAKS];
uint8_t peakCount = 0;

// Temporary buffer for smoothing (optional, can reuse input if memory tight)
uint16_t tempSmooth[FFT_SIZE];

// -------------------------------------------------------------------
// Main peak detection function - call this with fresh 1024-point data
// -------------------------------------------------------------------
uint8_t findSpectrumPeaks(const SpectrumPoint* input, SpectrumPoint* outputPeaks) {
  peakCount = 0;

  // ---------------------------------------------------------------
  // 1. Apply moving average smoothing (reduces grass/noise)
  // ---------------------------------------------------------------
  for (int i = 0; i < FFT_SIZE; i++) {
    uint32_t sum = 0;
    int count = 0;
    for (int j = -SMOOTH_WINDOW/2; j <= SMOOTH_WINDOW/2; j++) {
      int idx = i + j;
      if (idx >= 0 && idx < FFT_SIZE) {
        sum += input[idx].value;
        count++;
      }
    }
    tempSmooth[i] = sum / count;
  }

  // Copy smoothed data (optional - for debugging/plotting)
  for (int i = 0; i < FFT_SIZE; i++) {
    smoothed[i].bin = i;
    smoothed[i].value = tempSmooth[i];
  }

  // ---------------------------------------------------------------
  // 2. Estimate noise floor (median of lower 40% of trace)
  // ---------------------------------------------------------------
  uint16_t sorted[FFT_SIZE];
  for (int i = 0; i < FFT_SIZE; i++) sorted[i] = tempSmooth[i];
  
  // Simple partial sort to find ~40th percentile
  int lowerCount = FFT_SIZE * 2 / 5;
  for (int i = 0; i < lowerCount; i++) {
    int minIdx = i;
    for (int j = i + 1; j < FFT_SIZE; j++) {
      if (sorted[j] < sorted[minIdx]) minIdx = j;
    }
    uint16_t temp = sorted[i];
    sorted[i] = sorted[minIdx];
    sorted[minIdx] = temp;
  }
  uint16_t noiseFloor = sorted[lowerCount - 1];  // Approx 40th percentile

  // Dynamic threshold: peaks must rise significantly above local noise
  uint16_t threshold = noiseFloor + (uint16_t)((float)(1023 - noiseFloor) * PEAK_THRESHOLD_FACTOR);

  // ---------------------------------------------------------------
  // 3. Find local maxima with hysteresis and spacing
  // ---------------------------------------------------------------
  struct Candidate {
    uint16_t bin;
    uint16_t value;
    float prominence;  // How much it stands out
  } candidates[MAX_PEAKS * 2];
  int candCount = 0;

  for (int i = SMOOTH_WINDOW; i < FFT_SIZE - SMOOTH_WINDOW; i++) {
    uint16_t val = tempSmooth[i];
    if (val < threshold) continue;

    bool isPeak = true;
    for (int j = -SMOOTH_WINDOW; j <= SMOOTH_WINDOW; j++) {
      if (j == 0) continue;
      if (tempSmooth[i + j] >= val) {
        isPeak = false;
        break;
      }
    }

    if (isPeak) {
      // Measure prominence (height above highest valley on sides)
      uint16_t leftMin = val;
      for (int j = i - 1; j >= 0 && j >= i - 80; j--) {
        if (tempSmooth[j] < leftMin) leftMin = tempSmooth[j];
      }
      uint16_t rightMin = val;
      for (int j = i + 1; j < FFT_SIZE && j <= i + 80; j++) {
        if (tempSmooth[j] < rightMin) rightMin = tempSmooth[j];
      }
      uint16_t base = max(leftMin, rightMin);
      float prominence = val - base;

      // Insert candidate if good enough
      if (prominence > (1023 * 0.08)) {  // At least ~8% of full scale prominence
        if (candCount < MAX_PEAKS * 2) {
          candidates[candCount++] = { (uint16_t)i, val, prominence };
        }
      }
    }
  }

  // ---------------------------------------------------------------
  // 4. Sort candidates by prominence (best first)
  // ---------------------------------------------------------------
  for (int i = 0; i < candCount - 1; i++) {
    for (int j = i + 1; j < candCount; j++) {
      if (candidates[j].prominence > candidates[i].prominence) {
        Candidate temp = candidates[i];
        candidates[i] = candidates[j];
        candidates[j] = temp;
      }
    }
  }

  // ---------------------------------------------------------------
  // 5. Select top peaks with minimum distance enforcement
  // ---------------------------------------------------------------
  bool used[FFT_SIZE] = {0};

  for (int c = 0; c < candCount && peakCount < MAX_PEAKS; c++) {
    uint16_t bin = candidates[c].bin;
    if (used[bin]) continue;

    bool tooClose = false;
    for (int p = 0; p < peakCount; p++) {
      if (abs((int)peaks[p].bin - (int)bin) < MIN_PEAK_DISTANCE) {
        tooClose = true;
        break;
      }
    }
    if (!tooClose) {
      peaks[peakCount].bin = bin;
      peaks[peakCount].value = candidates[c].value;
      outputPeaks[peakCount] = peaks[peakCount];
      peakCount++;

      // Mark nearby bins as used to prevent close duplicates
      for (int d = -MIN_PEAK_DISTANCE/2; d <= MIN_PEAK_DISTANCE/2; d++) {
        int idx = bin + d;
        if (idx >= 0 && idx < FFT_SIZE) used[idx] = true;
      }
    }
  }

  // Sort final peaks by bin (frequency) order - nice for display
  for (int i = 0; i < peakCount - 1; i++) {
    for (int j = i + 1; j < peakCount; j++) {
      if (outputPeaks[j].bin < outputPeaks[i].bin) {
        SpectrumPoint temp = outputPeaks[i];
        outputPeaks[i] = outputPeaks[j];
        outputPeaks[j] = temp;
      }
    }
  }

  return peakCount;  // Number of valid peaks found (0 to 10)
}