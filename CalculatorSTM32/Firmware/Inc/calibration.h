#ifndef MC_CALIBRATION_H
#define MC_CALIBRATION_H
#include <stdint.h>
typedef struct { double x[3],y[3];uint8_t calibrated; } McCalibration;
typedef struct { int16_t x,y; } McPoint;
extern const McPoint mc_calibration_targets[3];
void mc_calibration_default(McCalibration *c,int revision_d);
int mc_calibration_solve(McCalibration *c,const McPoint raw[3]);
McPoint mc_calibration_map(const McCalibration *c,int raw_x,int raw_y);
#endif
