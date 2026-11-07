#include "sys.h"
#include "Cal_Freq.h"

RCC_ClocksTypeDef RCC_Clocks;

// 重启标志
uint8_t flag = 0;
// 模式标志
uint8_t mode_flag = 0;


short temp_short;
double angle;
int delayTime0;
int delayTime1;

// 输出波形
extern uint16_t Dac_Val_A[DA1_Value_Length];
extern uint16_t Dac_Val_B[DA1_Value_Length];

extern short adc_complex_value[1000];


// 采样完成标志，保证同步调相
extern u8 do_ad_flag1;
extern u8 do_ad_flag2;

int main()
{
  // 系统时钟配置查看
    RCC_GetClocksFreq(&RCC_Clocks);

  // PF14 LED初始化
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;  
    GPIO_Init(GPIOF, &GPIO_InitStructure);
    GPIO_WriteBit(GPIOF, GPIO_Pin_14, Bit_RESET);

    wave_type[0] = 0;
    wave_type[1] = 0;

    ADC_GPIO_Init();             // ADC引脚初始化。
    TIM3_Config();               // 触发ADC采样频率，采样频率1MHz
    
    ADC_Config();                // ADC 采集1000个数据，需要花费1ms,1000K采样频率，
    ADC_DMA_Trig(ADC1_DMA_Size); // 开始AD采集，设置采样点数

    for (int i = 0; i < 10000; i++)
        ; // 延时确保初始化完成
        
    Cal_Freq(fre);
    FFT_Detect_Type(fre, wave_type);

  //判断类型
    short *wave_src_0 = (wave_type[0] == 0) ? DAC_SIN : Tri_Wave;
    short *wave_src_1 = (wave_type[1] == 0) ? DAC_SIN : Tri_Wave;
    
    // 计算频率补偿系数（从10kHz开始，每5kHz衰减20mV，约衰减1.9%）


    double gain_0 = 1.0 + fre[0] * 0.03;  // 通道0补偿系数
    double gain_1 = 1.0 + fre[1] * 0.03;  // 通道1补偿系数
    
    // 限制补偿范围，避免超出DAC量程
//    if (gain_0 > 1.5) gain_0 = 1.5;  // 最大补偿50%
//    if (gain_1 > 1.5) gain_1 = 1.5;
    
    for (int i = 0; i < 200; i++)
    {
        // 应用频率补偿，以2048为中心进行放大
        int temp_val_0 = wave_src_0[fre[0] * 200 + i];
        int temp_val_1 = wave_src_1[fre[1] * 200 + i];
        
        // 补偿公式: new_val = 2048 + (old_val - 2048) * gain
        temp_val_0 = 2048 + (int)((temp_val_0 - 2048) * gain_0);
        temp_val_1 = 2048 + (int)((temp_val_1 - 2048) * gain_1);
        
        // 限幅保护 (12位DAC: 0-4095)
        if (temp_val_0 > 4095) temp_val_0 = 4095;
        if (temp_val_0 < 0) temp_val_0 = 0;
        if (temp_val_1 > 4095) temp_val_1 = 4095;
        if (temp_val_1 < 0) temp_val_1 = 0;
        
        Dac_Val_A[i] = (uint16_t)temp_val_0;
        Dac_Val_B[i] = (uint16_t)temp_val_1;
			
//			  Dac_Val_A[i] = wave_src_0[fre[0] * 200 + i];
//        Dac_Val_B[i] = wave_src_1[fre[1] * 200 + i];
    }
		
    TIM4_Init();
    DA1_Init();
    TIM6_Init();
    DA2_Init();

    // 同时启动定时器，保证DA初相无偏移
    TIM_Cmd(TIM4, ENABLE);
    TIM_Cmd(TIM6, ENABLE);

    // 实时相位校正 - 交替调相避免相互干扰
    static uint8_t adjust_counter = 1;
    
    while (1)
    {
        if (adjust_counter == 1) 
        {
            // 只调高频通道
            do_ad_flag2 = 0;
            while (do_ad_flag2 == 0);  // 等待DAC2完成一个周期
            
            adc_complete_flag = 0;
            ADC_DMA_Trig(ADC1_DMA_Size);
            while (adc_complete_flag == 0);  // 等待ADC采集完成 ，adc没有采集完成将会导致数据错误
			
            Get_Pos_Angle(1);//只计算高频通道相位，减少运算量
            
            delayTime1 = (9000.0 * pos_angle_1 / (3.1415926 * ((double)fre[1] + 2.0)) + 0.5);
            TIM6->CNT = 65535 - (delayTime1 - TIM6->CNT);
            
            adjust_counter = 0;
        }
        else 
        {
            // 只调低频通道
            do_ad_flag1 = 0;
            while (do_ad_flag1 == 0);  // 等待DAC1完成一个周期
            
            adc_complete_flag = 0;
            ADC_DMA_Trig(ADC1_DMA_Size);
            while (adc_complete_flag == 0);  // 等待ADC采集完成
            Get_Pos_Angle(0);
            
            delayTime0 = (9000.0 * pos_angle_0 / (3.1415926 * ((double)fre[0] + 2.0)) + 0.5);
            TIM4->CNT = 65535 - (delayTime0 - TIM4->CNT);
            
            adjust_counter = 1;
        }
    }
}
