#include <Arduino.h>
#include <Audio.h>

// --------------
// Audio Object
// --------------
// I2S 入力 -> RecordQueue(Left, Right)
AudioInputI2S      i2s;   // BCLK=21, MCLK=23, LRCLK=20, RX=8
AudioRecordQueue   rec_L; // L
AudioRecordQueue   rec_R; // R

// I2SQuad 出力 (4ch: L+, L-, R+, R-)
AudioOutputI2SQuad i2s_quad;
AudioPlayQueue     queue_L;  // L+
AudioPlayQueue     queue_LM; // L- (L の符号反転)
AudioPlayQueue     queue_R;  // R+
AudioPlayQueue     queue_RM; // R- (R の符号反転)

// --------------
// Audio Connection
// --------------
// I2S 入力 -> RecordQueue(Left, Right)
AudioConnection patchCord1(i2s, 0, rec_L, 0);
AudioConnection patchCord2(i2s, 1, rec_R, 0);

// PlayQueue -> I2SQuad 出力  (チャネル順: 0=L+, 1=L-, 2=R+, 3=R-)
AudioConnection patchCord3(queue_L,   0, i2s_quad, 0);
AudioConnection patchCord4(queue_LM,  0, i2s_quad, 1);
AudioConnection patchCord5(queue_R,   0, i2s_quad, 2);
AudioConnection patchCord6(queue_RM,  0, i2s_quad, 3);

// --------------
// Audio Buffer
// --------------
// 1ブロック(=128サンプル)単位で作業するための配列
static const int BLOCK_SIZE = AUDIO_BLOCK_SAMPLES; // 通常128
int16_t samples_L [BLOCK_SIZE];
int16_t samples_LM[BLOCK_SIZE];
int16_t samples_R [BLOCK_SIZE];
int16_t samples_RM[BLOCK_SIZE];

void setup() {
  // Audio処理用メモリの割り当て (必要な量を確保 100は適当)
  AudioMemory(100);

  // RecordQueue を開始 (オーディオ入力の取り込みスタート)
  rec_L.begin();
  rec_R.begin();
}

void loop() {
  // L, R それぞれのキューにオーディオデータが溜まっているか確認
  if ((rec_L.available() > 0) && (rec_R.available() > 0)) {
    // それぞれ1ブロック(128サンプル)を取り出す
    int16_t *blockL = rec_L.readBuffer();
    int16_t *blockR = rec_R.readBuffer();

    if (blockL != nullptr && blockR != nullptr) {
      // blockL, blockR に各128サンプルの音声データがある
      // 符号反転 (L-, R-) や必要な処理を行い、別バッファへコピー
      for (int i = 0; i < BLOCK_SIZE; i++) {
        samples_L [i] = blockL[i];       // L+
        samples_LM[i] = -blockL[i];      // L- (反転)
        samples_R [i] = blockR[i];       // R+
        samples_RM[i] = -blockR[i];      // R- (反転)
      }

      // PlayQueue へ書き込み → Quad 出力へ
      queue_L.play(samples_L,   BLOCK_SIZE);
      queue_LM.play(samples_LM, BLOCK_SIZE);
      queue_R.play(samples_R,   BLOCK_SIZE);
      queue_RM.play(samples_RM, BLOCK_SIZE);
    }

    // 使い終わったらバッファを解放 (重要)
    rec_L.freeBuffer();
    rec_R.freeBuffer();
  }
}
