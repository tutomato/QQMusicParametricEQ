#ifndef QMEQ_DSP_H
#define QMEQ_DSP_H
#include <math.h>
#include <string.h>
#include <stdint.h>
#define BANDS 10
#define CHANS 8
#define PI 3.14159265358979323846
#define MAX_GAIN 18

typedef struct {int enabled,type;double frequency,gain,q;} Band;
typedef struct {int enabled;double preamp;Band bands[BANDS];unsigned generation;} Config;
typedef struct {double c[BANDS][5],z[BANDS][CHANS][2],gain;} Bank;
typedef struct {Bank banks[2];int current,rate,channels,ready;unsigned generation,remaining,total;} Engine;
static void defaults(Config *c){static const double f[BANDS]={31,63,125,250,500,1000,2000,4000,8000,16000};memset(c,0,sizeof(*c));c->enabled=1;for(int i=0;i<BANDS;i++){c->bands[i].enabled=1;c->bands[i].frequency=f[i];c->bands[i].q=1;}}
static double clamp(double x,double lo,double hi){return !isfinite(x)?lo:x<lo?lo:x>hi?hi:x;}
static void coefficients(Band b,int rate,double *v){v[0]=1;v[1]=v[2]=v[3]=v[4]=0;if(!b.enabled||!b.gain||b.frequency>=rate/2.)return;double w=2*PI*b.frequency/rate,c=cos(w),A=pow(10,b.gain/40),a=sin(w)/(2*b.q),t=2*sqrt(A)*a,n[3],d[3];if(b.type==0){n[0]=1+a*A;n[1]=-2*c;n[2]=1-a*A;d[0]=1+a/A;d[1]=-2*c;d[2]=1-a/A;}else if(b.type==1){n[0]=A*((A+1)-(A-1)*c+t);n[1]=2*A*((A-1)-(A+1)*c);n[2]=A*((A+1)-(A-1)*c-t);d[0]=(A+1)+(A-1)*c+t;d[1]=-2*((A-1)+(A+1)*c);d[2]=(A+1)+(A-1)*c-t;}else{n[0]=A*((A+1)+(A-1)*c+t);n[1]=-2*A*((A-1)+(A+1)*c);n[2]=A*((A+1)+(A-1)*c-t);d[0]=(A+1)-(A-1)*c+t;d[1]=2*((A-1)-(A+1)*c);d[2]=(A+1)-(A-1)*c-t;}for(int i=0;i<3;i++)v[i]=n[i]/d[0];v[3]=d[1]/d[0];v[4]=d[2]/d[0];}
static void configure(Engine *e,const Config*c,int rate,int channels){int fresh=!e->ready||e->rate!=rate||e->channels!=channels;if(!fresh&&e->generation==c->generation)return;if(fresh){memset(e,0,sizeof(*e));e->rate=rate;e->channels=channels;}int next=1-e->current;Bank *b=&e->banks[next];memset(b,0,sizeof(*b));b->gain=c->enabled?pow(10,c->preamp/20):1;for(int i=0;i<BANDS;i++){Band band=c->bands[i];if(!c->enabled)band.enabled=0;coefficients(band,rate,b->c[i]);}e->current=next;e->generation=c->generation;e->remaining=e->total=fresh?0:(unsigned)(rate*.02);e->ready=1;}
static double run(Bank*b,double x,int ch){x*=b->gain;for(int i=0;i<BANDS;i++){double *c=b->c[i],*z=b->z[i][ch],y=c[0]*x+z[0];z[0]=c[1]*x-c[3]*y+z[1];z[1]=c[2]*x-c[4]*y;for(int j=0;j<2;j++)if(fabs(z[j])<1e-24)z[j]=0;x=y;}return isfinite(x)?x:0;}
static int64_t decode(const unsigned char*p,int bits){uint32_t v=p[0]|((uint32_t)p[1]<<8);if(bits>=24)v|=(uint32_t)p[2]<<16;if(bits==32)v|=(uint32_t)p[3]<<24;if(bits==16)return(int16_t)v;if(bits==24)return(v&0x800000)?(int64_t)v-16777216:v;return(int32_t)v;}
static void encode(unsigned char*p,int bits,double x){double scale=ldexp(1,bits-1);int64_t v=(int64_t)round(clamp(x,-1,1)*scale);if(v>(int64_t)scale-1)v=(int64_t)scale-1;if(v<-(int64_t)scale)v=-(int64_t)scale;for(int i=0;i<bits/8;i++)p[i]=(unsigned char)((uint64_t)v>>(8*i));}
static double process(Engine*e,const Config*c,void*data,int frames,int bits,int channels,int rate){if(!data||frames<=0||frames>1048576||channels<1||channels>CHANS||rate<8000||rate>768000||(bits!=16&&bits!=24&&bits!=32))return 0;configure(e,c,rate,channels);if(!c->enabled&&!e->remaining)return 0;unsigned char*p=data;double scale=ldexp(1,bits-1),peak=0;for(int f=0;f<frames;f++){double mix=e->remaining?1.-(double)e->remaining/e->total:1;for(int ch=0;ch<channels;ch++,p+=bits/8){double x=decode(p,bits)/scale,y=run(&e->banks[e->current],x,ch);if(e->remaining)y=y*mix+run(&e->banks[1-e->current],x,ch)*(1-mix);if(fabs(y)>peak)peak=fabs(y);encode(p,bits,y);}if(e->remaining)e->remaining--;}return peak;}
#endif
