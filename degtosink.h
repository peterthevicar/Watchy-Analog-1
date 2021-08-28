#ifndef DEGTOSINK
#define DEGTOSINK
int degToSinK [] = {
0,17,35,52,70,87,105,122,139,156,
174,191,208,225,242,259,276,292,309,326,
342,358,375,391,407,423,438,454,469,485,
500,515,530,545,559,574,588,602,616,629,
643,656,669,682,695,707,719,731,743,755,
766,777,788,799,809,819,829,839,848,857,
866,875,883,891,899,906,914,921,927,934,
940,946,951,956,961,966,970,974,978,982,
985,988,990,993,995,996,998,999,999,1000
};

int sinK(int d) {
  if (d >= 360) d = d % 360;
  else if (d < 0) d = d % 360 + 360;
  if (d >= 270) return -degToSinK[359 - d];
  if (d >= 180) return -degToSinK[d - 180];
  if (d >= 90) return degToSinK[179 - d];
  return degToSinK[d];
}
int cosK(int d) {
  return(sinK(d+90));
}

// Macro do convert back from x1000 values with rounding
#define FROMK(x) (((x) + 500)/1000)

#endif
