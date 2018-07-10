#include <math.h>

void bearing(float initlat, float initlong, float finallat, float finallong, float& distance, float& direction) {
  initlat= initlat*(M_PI/180);
  initlong= initlong*(M_PI/180);
  finallat= finallat*(M_PI/180);
  finallong= finallong*(M_PI/180);
  float deltalong= finallong-initlong;
  float deltalat= finallat-initlat;
  float R=6371;//quick math
  float a= pow(sin(deltalat/2),2) + cos(initlat)*cos(finallat)*pow(sin(deltalong/2),2);
  float c=2*atan2(sqrt(a),sqrt(1-a));
  
  float X= cos(finallat)*sin(deltalong);
  float Y= cos(initlat)*sin(finallat)-sin(initlat)*cos(finallat)*cos(deltalong);
  distance=R*c*1000;
  direction= atan2(X,Y)*180/M_PI;
}