#ifndef __CAL_FREQ_H__
#define __CAL_FREQ_H__

#include "sys.h"

double modulus(double I, double Q);
double corr1000_200(short *data, short *mask);
void Get_Pos_Angle(uint8_t flag);
void Cal_Freq(unsigned char *frq);
void FFT_Detect_Type(unsigned char *frq, unsigned char *wave_type);

//调用参数
extern unsigned char fre[2];// fre[0] = 0; --- 10KHz, fre[1] = 1; --- 15KHz
// 信号种类
extern unsigned char wave_type[2];// 0 --- 正/余弦，1 --- 三角形方波
extern double pos_angle_0; 
extern double pos_angle_1; 

extern short DAC_SIN[200*19];
extern short DAC_COS[200 * 19];
extern short Tri_Wave[200 * 19];
#endif
