// LED Toggle SW
#include <stdio.h>

void microcontroller(const double* adc, double* dac){
    // 입력 변수 (1: 눌림, 0: 안 눌림)
    int Push_switch_1 = (int)*(adc);

    // 상태 변수
    static int Push_switch_1_past = 0; // 이전 상태 저장
    static int LED_1 = 0; // LED 상태
    static int debounce_cnt = 0; // Chattering 방지용 카운터

    // 입력은 HW에서 읽음
    if(debounce_cnt > 0){
        debounce_cnt--;
    }
    else{
        // 버튼이 눌렸을 때 (현재: 1, 과거: 0)
        if(Push_switch_1 == 1 && Push_switch_1_past == 0){
            if(LED_1 == 1)
                LED_1 = 0; // LED가 켜져 있으면 끔
            else
                LED_1 = 1; // LED가 꺼져 있으면 켬
            }
    }
    
    Push_switch_1_past = Push_switch_1; // 현재 상태를 과거 상태로 저장
    
    // 출력 변수
    dac[0] = (double) LED_1;
}