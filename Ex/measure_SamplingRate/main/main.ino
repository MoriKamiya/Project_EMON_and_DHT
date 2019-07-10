#include <EmonLib.h>
#define CALIBRATION 45.25
#define BANDRATE 9600
EnergyMonitor CT;
unsigned long timeStamp[2]; //[0]儲存單機片運作時間戳記，[1]儲存函數執行時間
unsigned int i, j;
double Irms;
void setup(){
  Serial.begin(BANDRATE);
	CT.current(A0, CALIBRATION); //實例化智慧電表函數庫的電流量測物件
}
void loop(){
	for(i = 1 ; i <= 400 ; i++){ //測試400個採樣值。
		for(j = 0 ; j < 5 ; j++){ //執行五次函數，並且把五次函數執行時間平均。
			timeStamp[0] = micros(); 
			//獲取函數執行前的時間，以微秒為單位
			Irms = CT.calcIrms(i); //運作一次函數，以當前的i為採樣值
			timeStamp[1] = (timeStamp[1] + (micros() - timeStamp[0])) / 2;
			//拿現在時間減掉函數執行前的時間便是函數執行時間，
			//並且將其與之前計算出來的函數執行時間平均，盡量削減誤差。
		}
		//如果此採樣值令函數執行時間在16~17毫秒，代表該採樣值能很好運作在60Hz電力系統上。
		if (timeStamp[1] >= 16664 && timeStamp[1] <= 16668){
			Serial.print("Sampling數值: " + String(i) + " 為最佳解！\n");
		}
		else if (timeStamp[1] >= 16600 && timeStamp[1] <= 16832){ 
			Serial.print("Sampling數值: " + String(i) + " 接近最佳解！\n");
		}
		else if (timeStamp[1] >= 16300 && timeStamp[1] <= 16900){ 
			Serial.print("Sampling數值: " + String(i) + " 是個極佳的選擇！\n");
		}
		else if (timeStamp[1] >= 16000 && timeStamp[1] <= 17000){ 
			Serial.print("Sampling數值: " + String(i) + " 是個好選擇！\n");
		}
		Serial.print("採樣" + String(i) + "次時，函數時間為：" + String(timeStamp[1]) + "微秒");
		Serial.print("\n----------------\n");
		
	}
	Serial.print("-------------程式結束-------------");
	while(1);
}
