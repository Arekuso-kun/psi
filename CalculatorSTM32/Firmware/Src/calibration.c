#include "calibration.h"
#include <math.h>
const McPoint mc_calibration_targets[3]={{24,48},{215,112},{105,285}};
void mc_calibration_default(McCalibration *c,int revision_d) {
    c->x[0]=-1.0/15.0;c->x[1]=0;c->x[2]=3870.0/15.0;
    c->y[0]=0;c->y[1]=revision_d?-1.0/11.0:1.0/11.0;
    c->y[2]=revision_d?3700.0/11.0:-360.0/11.0;c->calibrated=0;
}
int mc_calibration_solve(McCalibration *c,const McPoint p[3]) {
    double x0=p[0].x,x1=p[1].x,x2=p[2].x,y0=p[0].y,y1=p[1].y,y2=p[2].y;
    double determinant=x0*(y1-y2)+x1*(y2-y0)+x2*(y0-y1);
    McCalibration next;unsigned axis;
    if(fabs(determinant)<10000) return 0;
    for(axis=0;axis<2;++axis) {
        double a0=axis?mc_calibration_targets[0].y:mc_calibration_targets[0].x;
        double a1=axis?mc_calibration_targets[1].y:mc_calibration_targets[1].x;
        double a2=axis?mc_calibration_targets[2].y:mc_calibration_targets[2].x;
        double *out=axis?next.y:next.x;
        out[0]=(a0*(y1-y2)+a1*(y2-y0)+a2*(y0-y1))/determinant;
        out[1]=(a0*(x2-x1)+a1*(x0-x2)+a2*(x1-x0))/determinant;
        out[2]=(a0*(x1*y2-x2*y1)+a1*(x2*y0-x0*y2)+a2*(x0*y1-x1*y0))/determinant;
        if(!isfinite(out[0])||!isfinite(out[1])||!isfinite(out[2])) return 0;
    }
    next.calibrated=1;*c=next;return 1;
}
McPoint mc_calibration_map(const McCalibration *c,int raw_x,int raw_y) {
    McPoint out;
    double x=c->x[0]*raw_x+c->x[1]*raw_y+c->x[2];
    double y=c->y[0]*raw_x+c->y[1]*raw_y+c->y[2];
    if(x<0)x=0;
    if(x>239)x=239;
    if(y<0)y=0;
    if(y>319)y=319;
    out.x=(int16_t)(x+0.5);out.y=(int16_t)(y+0.5);return out;
}
