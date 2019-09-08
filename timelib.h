#pragma once

typedef struct {
  int y, m, d;
  int h, i, s;
  int invert;
} reltime;

void diff_times(signed long long startSeconds, signed long long endSeconds, reltime *rel);
