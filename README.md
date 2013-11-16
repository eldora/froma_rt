#### v1.2(20131113) ####
- compile waring 제거

#### v1.4(20131114) ####
- startup.S Stack 할당하는 것 각각 주소 배정 및 CommomSection 삭제
- port.c: pxCurrentTCB, px*Stack들을 portNUM_PROCESSORS 배열로 만듬 

#### v1.5(20131116) ####
- L1 D&I Cache Disabled, Using Memory Barrier - data sharing 확인
- CPU 초기화 후 동기화 루틴 구현 완료

#### 해야할 것 ####
- Create Prime1, 2 생성 및 Shell에서 명령을 줬을 때 동작 후 ready task로 이동
- spinlock에서 변수 확인할 때 release 후 0이 안된 것을 확인했는데 이것이 바로 0이 되게 고쳐야 함
- spinlock 함수에 대해 고찰
- L1 Cache를 Enable 시키고도 캐쉬 값을 flush하여 동기화를 구현할 수 있도록 방법 모색
