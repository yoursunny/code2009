#include "spce061a.h"
#include "Flash.h"
#include "DVR.h"
#include "s480.h"

#define START_ADDR	0xB800								// ����¼����ʼ��ַ
#define END_ADDR	0xFBFF								// ����¼��������ַ

void Delay(void)
{
	unsigned int uiCount;
	for(uiCount = 0;uiCount <= 1000;uiCount++)
	{
		*P_Watchdog_Clear = 0x0001;    //�忴�Ź�
	}
}

void PlaySnd_Auto(unsigned int uiSndIndex,unsigned int uiDAC_Channel)
{
	SACM_S480_Initial(1);						//��ʼ��Ϊ�Զ����ŷ�ʽ
	SACM_S480_Play(uiSndIndex,uiDAC_Channel,3);	//����
	while((SACM_S480_Status() & 0x0001) != 0)
	{											//�жϲ���״̬���绹�ڲ��������ѭ��
		SACM_S480_ServiceLoop();				//����ϵͳ�������
		*P_Watchdog_Clear = 0x0001;			
	}	
	SACM_S480_Stop();							//ֹͣ����
}

void PlayRecord()
{
	unsigned long Addr,EndAddr;
	unsigned int Ret;

	SACM_DVR_Initial(0);								// ��ʼ������			
	SACM_DVR_InitQueue();								// ��ʼ���������
	SACM_DVR_InitDecoder(3);							// ��ʼ�������㷨��������DAC1��DAC2���

	Addr = START_ADDR + 1;
	EndAddr = Flash_ReadWord(START_ADDR);				// ��¼���׵�ַ�ж���¼��������ַ
	
	while(1)
	{
		*P_Watchdog_Clear = 0x01;						// �忴�Ź�
		while(SACM_DVR_TestQueue()!=1)					// ����������δ��
		{
			Ret = Flash_ReadWord(Addr);					// ȡ¼������
			SACM_DVR_FillQueue(Ret);					// �����������
    		Addr++;
			if(Addr > EndAddr) break;
    	}
		if((SACM_DVR_Status()&0x01)==0 || (*P_IOA_Data&0x0002))	// ���������ϻ�Key2��������ֹͣ
		{
        	SACM_DVR_Stop();							// ֹͣ����
        	break;
		}
        else
        	SACM_DVR_Decoder();							// ���ݽ���
	}
}

void Record(void)
{
	unsigned int Addr;
	unsigned int Ret;
	
	for(Addr=START_ADDR;Addr<=END_ADDR;Addr+=0x0100)
	{
		Flash_Erase(Addr);								// ����¼������Flash�ռ�
		*P_Watchdog_Clear = 0x01;
	}
	
	PlaySnd_Auto(3,3);//¼����ʼ��ʾ��
	
	SACM_DVR_Initial(0);								// ��ʼ��¼����
	SACM_DVR_InitQueue();								// ��ʼ���������
	SACM_DVR_InitEncoder(0,0);							// ��ʼ�������㷨
	
	Addr = START_ADDR + 1;								// ��һ����Ԫ���ڱ���¼��������ַ
	while(1)											// ¼��ѭ��
	{
		*P_Watchdog_Clear = 0x01;						// �忴�Ź�
		SACM_DVR_Encoder();								// ��AD�ɼ��������ݱ���
		if(SACM_DVR_TestQueue()!=2)						// ���������в�Ϊ��
		{	
			Ret = SACM_DVR_FetchQueue();				// �ӱ��������ȡ����
			Flash_WriteWord(Addr, Ret);					// ������д��flash 
			Addr += 1;
			if(Addr>END_ADDR || (*P_IOA_Data&0x0002))		// �������β��ַ��Key2�����������¼��
			{
				Flash_WriteWord(START_ADDR, Addr-1);	// ¼��������ַ������[START_ADDR]��
				SACM_DVR_Stop();						// ֹͣ¼��
				break;
			}
		}
	}
}

//��8λ��żУ�飬ʹ��9λ��ż����1����7λ����0
int Ebit(unsigned int n)
{
	unsigned int count=0;
	if (0x0001&n) ++count;
	if (0x0002&n) ++count;
	if (0x0004&n) ++count;
	if (0x0008&n) ++count;
	if (0x0010&n) ++count;
	if (0x0020&n) ++count;
	if (0x0040&n) ++count;
	if (0x0080&n) ++count;
	return ((count<<8)|n)&0x01ff;
}

//�������ݣ����أ�
//0=�ɹ�
//1=��ʼ����ʱ
//2=�����г�ʱ
//3=������ʱ
//4=��¼������
int Send()
{
	unsigned int Addr;
	unsigned int EndAddr;
	unsigned int Ret;
	unsigned int addrH;
	unsigned int addrL;
	unsigned int dataH;	unsigned int dataL;
	unsigned int i;
	unsigned int k;
	
	EndAddr = Flash_ReadWord(START_ADDR);				// ��¼���׵�ַ�ж���¼��������ַ
	if (EndAddr==0xffff) return 4;

	*P_IOB_Dir = 0xffff;				//��ʼ��IOB��Ϊͬ�������
	*P_IOB_Attrib = 0xffff;
	//׼�����ͣ�����1100 1111 1001 0110���ȴ�1111 1100����
	*P_IOB_Data=0xcf96;
	i=0;
	while (1)
	{
		++i;
		Delay();
		if (i>3000)//�޻�Ӧ������ʧ��
		{
			return 1;
		}
		if ((*P_IOA_Data&0xff00)==0xfc00) break;
	}

	for(Addr=START_ADDR;Addr<=EndAddr;++Addr)
	{
		*P_Watchdog_Clear = 0x01;

		Ret = Flash_ReadWord(Addr);					// ȡ¼������

		//���ݷ��ͷ�ʽ��5������
		//����1��0000 000E ��ַ��8λ
		//����2��0001 000E ��ַ��8λ
		//����3��0010 000E ���ݸ�8λ
		//����4��0011 000E ���ݵ�8λ
		//����5��1101 111E ����1��2��3��4��11λ ����1��2��3��4��15λ
		//��ʱ�ȴ���Ӧ��IOA��8λ��1001 0110����1001 1111�����ط�
		//�ڰ�λE����żУ��λ��Ҫ���9λ��ż����1
		Delay();
		addrH=0x00ff&(Addr>>8);
		*P_IOB_Data=addrH|0x0000|Ebit(addrH);
		Delay();
		addrL=0x00ff&Addr;
		*P_IOB_Data=addrL|0x1000|Ebit(addrL);
		Delay();
		dataH=0x00ff&(Ret>>8);
		*P_IOB_Data=dataH|0x2000|Ebit(dataH);
		Delay();
		dataL=0x00ff&Ret;
		*P_IOB_Data=dataL|0x3000|Ebit(dataL);
		Delay();
		k=((0x0022&addrH)<<3)|((0x0022&addrL)<<2)|((0x0022&dataH)<<1)|((0x0022&dataL)<<0);
		*P_IOB_Data=0xde00|Ebit(k);
		
		i=0;
		while (1)
		{
			++i;
			Delay();
			if (i>1000)//�޻�Ӧ������ʧ��
			{
				return 2;
				break;
			}
			Ret=(*P_IOA_Data&0xff00);
			if (Ret==0x9600) break;
			if (Ret==0x9f00)
			{
				--Addr;//�ط���ǰ��Ԫ
				break;
			}
		}	
	}
	
	//ȫ��������ϣ�����1010 1111 1001 0110���ȴ�1111 1010����
	*P_IOB_Data=0xaf96;
	while (1)
	{
		++i;
		Delay();
		if (i>1000)//�޻�Ӧ������ʧ��
		{
			return 3;
		}
		if ((*P_IOA_Data&0xff00)==0xfa00) break;
	}
	return 0;
}

//�������ݣ����أ�
//0=�ɹ�
//1=������
//2=���ͷ�����
//3=��ʱ
int Accept()
{
	unsigned int Ret;
	unsigned int last=0;
	unsigned int addrH;
	unsigned int addrL;
	unsigned int dataH;
	unsigned int dataL;
	unsigned int Addr;
	unsigned int Data;
	//unsigned int i;
	unsigned int k;
	unsigned int rounds=0;
	
	if (*P_IOB_Data!=0xcf96) return 1;//δ�յ����ݣ�����

	PlaySnd_Auto(2,3);//����

	for(Addr=START_ADDR;Addr<=END_ADDR;Addr+=0x0100)
	{
		Flash_Erase(Addr);								// ������������Flash�ռ�
		*P_Watchdog_Clear = 0x01;
	}

	//׼�����գ��յ�1100 1111 1001 0110������1111 1100
	*P_IOA_Dir = 0xffff;				//��ʼ��IOA��Ϊͬ�������
	*P_IOA_Attrib = 0xffff;

	*P_IOA_Data=0xfc00;//����Ӧ������ź�
	
	//i=0;
	while (1)
	{
		*P_Watchdog_Clear = 0x01;
		
		//++i;
		//if (i>60000)//��ʱ������
		//	return 3;

		Ret=*P_IOB_Data;
		
		//if (Ret==0x0000||Ret==0xffff)//���ͷ��Ѿ���������
		//	return 2;
		
		if (Ret==last) continue;//����δ�䣬������
		else last=Ret;
		
		//׼�����գ��յ�1100 1111 1001 0110������1111 1100
		if (Ret==0xcf96)
		{
			*P_IOA_Data=0xfc00;
			continue;
		}

		//ȫ���������յ�1010 1111 1001 0110������1111 1010
		if (Ret==0xaf96)
		{
			*P_IOA_Data=0xfa00;
			++Addr;
			Flash_WriteWord(Addr,0xffff);
			break;
		}

		//һ���������ѽ��գ��յ�1101 111E ����1��2��3��4��11λ ����1��2��3��4��15λ����ȷ����1001 0110���������1001 1111
		if ((Ret&0xfe00)==0xde00)
		{
			k=((0x0022&addrH)<<3)|((0x0022&addrL)<<2)|((0x0022&dataH)<<1)|((0x0022&dataL)<<0);
			if (Ret==(0xde00|Ebit(k)) && (rounds&0x000f)==0x000f)//У����ȷ���Ҿ�����4������
			{
				Addr=(addrH<<8)+addrL;
				Data=(dataH<<8)+dataL;
				if (Addr>=START_ADDR && Addr<=END_ADDR)//��ֹд���������
					Flash_WriteWord(Addr,Data);
				*P_IOA_Data=0x9600;
			}
			else
			{
				*P_IOA_Data=0x9f00;				
			}
			rounds=0;
			//i=0;
		}
		switch (Ret&0x3000)
		{
			case 0x0000:
				//*P_IOA_Data=0x0000;
				k=Ret&0x00ff;
				if (Ebit(k)==(Ret&0x01ff)) {
					addrH=k;
					rounds=rounds|0x0001;
				}
				break;
			case 0x1000:
				//*P_IOA_Data=0x0000;
				k=Ret&0x00ff;
				if (Ebit(k)==(Ret&0x01ff)) {
					addrL=k;
					rounds=rounds|0x0002;
				}
				break;
			case 0x2000:
				//*P_IOA_Data=0x0000;
				k=Ret&0x00ff;
				if (Ebit(k)==(Ret&0x01ff)) {
					dataH=k;
					rounds=rounds|0x0004;
				}
				break;
			case 0x3000:
				//*P_IOA_Data=0x0000;
				k=Ret&0x00ff;
				if (Ebit(k)==(Ret&0x01ff)) {
					dataL=k;
					rounds=rounds|0x0008;
				}
				break;
		}
	}
	return 0;
}

int main()
{
	unsigned int Ret;
	unsigned int i;
	
	
	while(1)
	{
		*P_IOA_Dir = 0x0000;				//��ʼ��IOA��Ϊ���������������
		*P_IOA_Attrib = 0x0000;
		
		*P_IOB_Dir = 0x0000;				//��ʼ��IOB��Ϊ���������������
		*P_IOB_Attrib = 0x0000;
	
		Ret=Accept();
		switch (Ret)
		{
			case 0:
				PlaySnd_Auto(2,3);
				break;
			case 2:
			case 3:
				PlaySnd_Auto(1,3);
				break;
		}

		Ret=*P_IOA_Data;
		if (Ret&0x0001)									// Key1��������¼��
		{
			Record();
			for (i=0;i<100;++i) Delay();
		}
		if (Ret&0x0002)									// Key2�������򲥷�
		{
			while (1)//�ȴ�Key2�ͷţ�������ܵ��²�������ֹͣ
			{
				*P_Watchdog_Clear = 0x01;
				if (!(*P_IOA_Data&0x0002))
				{
					break;
				}
			}
			PlayRecord();
			for (i=0;i<100;++i) Delay();
		}
		if (Ret&0x0004)									// Key3����������
		{
			Ret=Send();
			switch (Ret)
			{
				case 0:
					PlaySnd_Auto(0,3);
					break;
				case 1:
				case 2:
				case 3:
					PlaySnd_Auto(1,3);
					break;
			}
		}
		*P_Watchdog_Clear = 0x01;
	}
}
