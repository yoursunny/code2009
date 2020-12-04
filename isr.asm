//======================================================
// �ļ����ƣ�	ISR.asm
// ����������	����¼�ŵ��жϷ������
//	����������	��������е�������Դ�͵�DAC���в���
//======================================================

.INCLUDE spce061a.inc
.include DVR.inc
.INCLUDE s480.inc

.TEXT
.PUBLIC _FIQ

_FIQ:
	push r1,r5 to [sp]
	//����¼��
	r1 = C_FIQ_TMA
	call F_FIQ_Service_SACM_DVR

	//S480����
	r1 = C_FIQ_TMA
	test r1,[P_INT_Ctrl]
	jnz L_FIQ_TimerA
	r1 = C_FIQ_TMB
	test r1,[P_INT_Ctrl]
	jnz L_FIQ_TimerB
L_FIQ_PWM:
	r1 = C_FIQ_PWM
	[P_INT_Clear] = r1
	pop r1,r5 from [sp]
	reti
L_FIQ_TimerA:
	[P_INT_Clear] = r1
	call F_FIQ_Service_SACM_S480
	pop r1,r5 from [sp]
	reti
L_FIQ_TimerB:
	[P_INT_Clear] = r1
	pop r1,r5 from [sp]
	reti