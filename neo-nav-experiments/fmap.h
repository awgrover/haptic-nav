#pragma once

long map(int x, int in_min, int in_max, int out_min, int out_max)
{
  return map((long)x,(long)in_min,(long)in_max,(long) out_min, (long)out_max);
}

/*float map(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}*/
double map(double x, double in_min, double in_max, double out_min, double out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
