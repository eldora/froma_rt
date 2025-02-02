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

#### v1.9(20131119) ####
- 1. Core1에서 인터럽터를 인에이블 시키지 않고 vPortInstallInterruptHandler( (void (*)(void *))vPortYieldFromISR...) 설정도 안해줬는데 왜 반대로 Core1에서만 작동하고 Core0에서는 작동안할까?
- 1번의 이유: vPortInstallInterruptHandler에서 정의된 것이 uxProcessorTargets을 portCORE_ID()와 동일하게 봐서 이 함수에서는 이것을 사용했는데 다르다. 시프트연산이 적용된 것이므로 portCORE_ID를 적어줘야 함, 그래서 aboart panic이 나지 않았나 예상해봄
- 2. 아직도 prvSetupTimerInterrupt()를 호출하여 타이머를 설정하면 abort panic이 발생함
- 3. 두 개의 prime 명령을 실행시켰을 시 한쪽이 제대로 돌아오지 않음
- 3번의 이유: PRIME_CPU0, 1은 인자값으로 받은 CPU를 가지고 Resume할 테스크를 선택했기 때문에 primeTask 내부에서 portCORE_ID()를 통해 Resume할 테스크를 선택해주는 방식으로 변경하였다.
- spinlock.c: 함수 내부에 인터럽트 Enable/Disable를 삽입함
- 4. 자신의 CPU에서 생성한 태스크를 다른 태스크가 실행시키고 Resume하려면 정상작동하지 않는다 왜?
- 우선순위를 변경하여 각 CPU에서 생성한 것들은 자신의 CPU에 할당되도록 하여 문맥전환이 제대로 수행되는 것을 확인함

#### v2.0(20131119) ####
- 멀티코어활용을 가시적으로 보여주기 위해 primeSMP명령어를 추가하였다.

#### v2.1(20131228) ####
- Core1만 타이머 인터럽트를 살릴 경우 정상 작동(Context Switch는 제외하고)

#### v2.2(20131230) ####
- Core1의 타이머만을 사용
- doc, boot폴더 추가
- NOTE: M1테스크의 파라메터를 2-100000하면 Abort Error, 2-50000하면 이상없음

#### v3.0(20140104) ####
- tasks.c: vContextSwitch() - 스케줄러 코드 변경 및 schedulerLock 추가 
- spinlock.h: spinlock 코드를 어셈블리어로 바꾸고 static inline 함수로 작성, spinlock.c 파일 삭제
- shell.c: 102: FROMA Version 2.17 / 90제거 / 11-15:\t- 추가, 16:- 추가
- tasks.c: 960, 961: Suspend: 제거 / 1042, 1043: Resume: 제거 / 
- apps.c: 45, 95: (int)(xTaskGetTickCount() - xLastExecutionTime)에 /10 해줌 & ms표시
- port.c: 329: RATE - 10,000 으로 0.1ms마다 클럭뛰는걸로 추정

#### NOTE ####
- Prime 알고리즘은 Trial Division을 사용하였다. 에라토스테네스의 체 알고리즘은 기존 prime값을 배열로 기억하고 있어야 하기 때문에 사용하지 않았다.

#### 해야할 것 ####
- Create Prime1, 2 생성 및 Shell에서 명령을 줬을 때 동작 후 ready task로 이동
- spinlock에서 변수 확인할 때 release 후 0이 안된 것을 확인했는데 이것이 바로 0이 되게 고쳐야 함
- spinlock 함수에 대해 고찰
- L1 Cache를 Enable 시키고도 캐쉬 값을 flush하여 동기화를 구현할 수 있도록 방법 모색
