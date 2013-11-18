#### v1.2(20131113) ####
- compile waring 제거

#### v1.4(20131114) ####
- startup.S Stack 할당하는 것 각각 주소 배정 및 CommomSection 삭제
- port.c: pxCurrentTCB, px*Stack들을 portNUM_PROCESSORS 배열로 만듬 

#### v1.5(20131116) ####
- L1 D&I Cache Disabled, Using Memory Barrier - data sharing 확인
- CPU 초기화 후 동기화 루틴 구현 완료

#### v1.6(20131117) ####
- CPU0 Core만을 사용하여 쉘 상에서 문맥전환 구현 완료
+ 이유: xTaskCreate시에 xTaskHandle 변수의 주소값을 넘겨줬어야 했다.

#### v1.7(20131118) ####
- tasks.c: pxCurrentTCB, uxTopReadyPriority를 Core의 갯수에 맞게 변경하였으며 관련 코드도 pxCurrentTCB -> pxCurrentTCB[ portCORE_ID() ]형식으로 변경하였다. 그 밖에 것들은 TAG:AJH의 주석을 통해 참고
- port.c: vPortSVCHandler, vPortInterruptContext(아직 안함) 함수에서 pxCurrentTCB 주소 부분 코드 변경
- tasks.c: vTaskSuspend, vTaskResume에서 두번째 인자 CPU_CORE_ID를 인수로 받는 것으로 함수를 변경함, 이유는 vTaskSuspend, vTaskResume를 호출하는 함수는 대부분 메인 CPU이기 때문이다. 그래서 portCORE_ID()를 사용하면 pxCurrentTCB[0]과 pxCurrentTCB[1]이 비교하게 되서 올바르지 않은 결과가 발생함
- 문제: prime을 실행시켰을 시 CPU1에서 pxTCB는 제대로 설정되었는데 실행이 안된다. tasks.c:vTaskSuspend부분부터 해석 portYIELD_WITHIN_API_CORE(xCoreID)
+ 이유: xTaskCreate시에 xTaskHandle 변수의 주소값을 넘겨줬어야 했다.

#### v1.8(20131118) ####
- abort panic 발생 이유 찾아보기
- port.c: v1.7에서 변경안해준 vPortInterruptContext를 변경
- handler, tickCount를 portNUM_PROCESSORS 수로 수정
- secondary_main 함수에서 xPortStartScheduler를 vPortStartFirstTask로 수정하니 core-1에서 aboart panic이 발생하지 않음: 추측컨데 타이머 인터럽트는 한번만 설정하면 각 코어별로 동작하나?, 아니다 원인은 prvSetupTimerInterrupt()에 있었다. FirstTask로 바꿔서 core1에서 prvSetupTimerInterrupt()가 설정되지 않았고 아래에서 ucProcessorTargets인자가 1로 되어있어서 타이머 클럭 인터럽트가 발생하지 않아 abort panic이 일어나지 않았던 것
- clockInterrupt 멈춘 이유: pxInterruptHandlers에서 [ portCORE_ID() ]->[ ucProcessorTargets ]이유 -> main.c:151에서 ucProcessorTargets인자가 1로 되어 있었다. 이를 portCORE_ID로 변경
- prvSetupTimerInterrupt()를 주석처리하고 임시저장, 현재 타이머인터럽트가 일제히 동작안하는 상태에서 쉘상에서 태스크들의 컨택스트 스위칭은 잘 일어난다.

#### 해야할 것 ####
- Create Prime1, 2 생성 및 Shell에서 명령을 줬을 때 동작 후 ready task로 이동
- spinlock에서 변수 확인할 때 release 후 0이 안된 것을 확인했는데 이것이 바로 0이 되게 고쳐야 함
- spinlock 함수에 대해 고찰
- L1 Cache를 Enable 시키고도 캐쉬 값을 flush하여 동기화를 구현할 수 있도록 방법 모색
