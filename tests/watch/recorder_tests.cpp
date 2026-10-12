#include "RecordingPlayback.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
using namespace dpwatch;
static void u32(std::vector<unsigned char> &b, unsigned n) {
  for (int i = 0; i < 4; ++i)
    b.push_back((n >> (i * 8)) & 255);
}
static FILE *wav(unsigned channels = 2, unsigned bits = 16,
                 unsigned bytes = 8) {
  std::vector<unsigned char> b = {'R', 'I', 'F', 'F'};
  u32(b, 36 + bytes);
  const char *h = "WAVEfmt ";
  b.insert(b.end(), h, h + 8);
  u32(b, 16);
  b.push_back(1);
  b.push_back(0);
  b.push_back(static_cast<unsigned char>(channels));
  b.push_back(0);
  u32(b, 24000);
  u32(b, 24000 * channels * bits / 8);
  b.push_back(static_cast<unsigned char>(channels * bits / 8));
  b.push_back(0);
  b.push_back(static_cast<unsigned char>(bits));
  b.push_back(0);
  const char *d = "data";
  b.insert(b.end(), d, d + 4);
  u32(b, bytes);
  b.resize(b.size() + bytes);
  FILE *f = nullptr;
  assert(tmpfile_s(&f) == 0);
  assert(fwrite(b.data(), 1, b.size(), f) == b.size());
  rewind(f);
  return f;
}
int main() {
  WavRecording info{};
  FILE *f = wav();
  assert(readRecording(f, info));
  assert(info.sampleRate == 24000 && info.channels == 2 &&
         info.dataBytes == 8 && ftell(f) == 44);
  fclose(f);
  f = wav(1);
  assert(readRecording(f, info));
  fclose(f);
  f = wav(3);
  assert(!readRecording(f, info));
  fclose(f);
  f = wav(2, 8);
  assert(!readRecording(f, info));
  fclose(f);
  f = wav(2, 16, 7);
  assert(!readRecording(f, info));
  fclose(f);
  f = wav(2, 16, 0);
  assert(!readRecording(f, info));
  fclose(f);
  f = wav();
  fseek(f, 40, SEEK_SET);
  unsigned bad = 400;
  fwrite(&bad, 4, 1, f);
  rewind(f);
  assert(!readRecording(f, info));
  fclose(f);
  f = wav();
  fseek(f, 0, SEEK_SET);
  fwrite("NOPE", 4, 1, f);
  rewind(f);
  assert(!readRecording(f, info));
  fclose(f);
  RecordingPage page;
  page.offer("REC-02.wav");
  page.offer("REC-01.wav");
  page.offer("REC-04.wav");
  page.offer("REC-03.wav");
  page.offer("REC-06.wav");
  page.offer("REC-05.wav");
  page.offer("REC-07.wav");
  page.offer("REC-08.wav.partial");
  page.offer("../bad.wav");
  assert(page.count == 6 && page.hasMore());
  assert(page.names[0] == "REC-07.wav" && page.names[4] == "REC-03.wav");
  RecordingPage next("REC-03.wav");
  for (auto name : {"REC-01.wav", "REC-03.wav", "REC-02.wav", "REC-04.wav"})
    next.offer(name);
  assert(next.count == 2 && !next.hasMore() && next.names[0] == "REC-02.wav");
  assert(canStartRecorderWork(true, false, false));
  assert(!canStartRecorderWork(false, false, false));
  assert(!canStartRecorderWork(true, true, false));
  assert(!canStartRecorderWork(true, false, true));
  puts("Recorder tests passed: WAV validation, bounded library paging and "
       "audio ownership gating");
}
