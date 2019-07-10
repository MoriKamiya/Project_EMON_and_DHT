# EMON Library with DE-SCT10
比流器實際上是一種特殊設計的變壓器。根據安培右手定則，當導線內通有電流時將會產生磁場，此時透過比流器將該磁場轉換為電流，該電流便為線路內流通電流之比值。

本次使用的比流器型號為DE-SCT10，匝數比為\\(3000:1\\)，最大可量測交流電流有效值達80安培。
由於該比流器的規格與較普遍的SCT013有顯著的差別，所以我們必須重新計算一部分電路才能夠將DE-SCT10裝上EMON Library.

## ***電路設計思路***

根據基本電學：
首先：已知線圈匝數比為\\(3000:1\\)，最大電流為\\(80A\\)的情況下，求出比流器二次側的最大電流。
	為求解這個問題，我們可以先求出其一次測峰值電流，並得出峰對峰值電流：
$$
\begin{align}
&I_{1_{peak}} = \sqrt{2}I_{RMS} = 80\times\sqrt{2} \approx 113.13\ A \ \mid \ if\ Sine\ Wave \\ 
&I_{{1}_{p-p}} = 2I_{{1}_{peak}} = 226.26\ A
\end{align}
$$
	但由於線圈比為\\(3000:1\\)，故於二次測量測電流時，其數值應除上\\(3000\\)：
$$
I_{2_{p-p}} = I_{1_{p-p}}\div 3000 = 75.42\ mA
$$
故得知，該比流器最高可能輸出\\(75.42\ mA\\)的電流。
並且已知，Arduino UNO (ATMega系列MCU) 的類比信號輸入最多可以解析\\(5V\\)之電壓值，而不能直接量測電流，或超過\\(5V\\)之電壓，
故我們希望比流經過負載電阻時產生的電壓小於\\(5V\\)：

根據\\(V = I \times R\\)

$$
\begin{align}
&V_{1_{p-p}} &\le 5\ V \\
&I_{1_{p-p}} \times R_{Burden} &\le 5\ V \\
&R_{Buredn} &\le 5 \div 75.42\ mA\\
&R_{Burden} &\le 66.3\ \Omega \tag#
\end{align}
$$
得出以下電路：

![](https://i.imgur.com/1Eoh435.png)

然而，還有一點必須留意：我們目前所知道的\\(V_{1_{p-p}}\\) 實際上是交流信號，其根據週期存在正負電的特徵，如圖：

![](https://i.imgur.com/ND9dLk0.png)

我們知道，Arduino的類比信號輸入並沒有辦法檢測負電，所以其實並不能直接將這樣的信號傳入；在傳入該信號前必須對這個信號進行適當的調整。

由於已知\\(V_{1_{p-p}} \approx 5V\\)，
故可知一個波峰\\(V_{1_p} = 2.5V\\)，
所以可以得知\\(V_1 = \pm2.5V\\)

此時倘若將\\(V_1\\)加上\\(+2.5V\\)，則可以將原先的負電位提升至零點，相當於讓整個波形向上移動，而能讓Arduino的類比信號輸入正常工作。
故我們可以設計如圖右方的分壓電路，得到\\(2.5V\\)的電壓，並且接上原始信號。
![](https://i.imgur.com/Bf5iiCk.png)
* \\(C_1\\)電容用作低通濾波器，用以緩解電力系統故障時產生的暫時性地高頻脈衝。
* \\(TVS\\)二極體用於避免出乎意料的大電流轉換為電壓時損壞電路；此元件為比流器內自帶。

如此一來便大功告成。

##  ***Calibration 計算***

根據EMON Library官方給出的公式：
$$
\begin{align}
Calibration = CT\ Ratio^{-1} \div R_{Burden}
\end{align}
$$
其中，\\(CT\ Ratio^{-1}\\)代表著比流器的線圈匝數之倒數，故可計算得出：
$$
\begin{align}
Calibration = 3000 \div 66.3 \approx 45.25\ \Omega^{-1}
\end{align}
$$
根據EMON Library的官方文件，其要求使用者在實例化EMON物件後，於設定階段給定計算出的\\(Calibration\\)數值，如下：
```cpp
#include <Emonlib.h>
EnergyMonitor CT;
void setup(){
	CT.current(A0, 45.25); //給定輸入接腳與Calibration
}
void loop(){
	/*Do something*/
}
```

另外，這個參數在實際意義上，有這樣一個恆等式：
$$
\begin{align}
I_{1_{RMS}} = V_{2_{RMS}}\  \times \ Calibration
\end{align}
$$
## ***Sampling 計算***

由於EMON Library使用了採樣計算的方式；也就是透過不斷讀取當下電壓信號的方式來換算獲取當下電流，並且將該數值進行次方、平均、方根運算，故我們需要計算出採樣引數，以確保每次採樣計算結束的時間都剛好是一個完整之弦波。
台灣使用的電力系統為\\(60Hz\\)，這代表著每一秒就會有60個完整的弦波，也意味著要產生一個完整的弦波，需要用時：
$$
\begin{align}
T = 1 \div f = \frac{1}{60} = 16.\overline{6}\ ms \ \ \mid \ if\ 60Hz\ Power\ system
\end{align}
$$
也就是說，我們會希望在電流被採樣計算的函數執行時，函數的總執行時間必須盡量等於\\(16.\overline{6}\ 毫秒\\)。
那麼，我們便可以透過一支小程式來找尋出：幾次採樣計算時，函數執行時間為\\(16.\overline{6}\ 毫秒\\)。

```cpp
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
```
## ***實際製作***
### 程式碼：
```cpp
#include <Emonlib.h>
#define Calibration 45.25
#define Sampling 86
EnergyMonitor CT;
double Irms;
void setup(){
    CT.current(A0, Calibration);
}

void loop(){
    Irms = CT.calcIrms(Sampling);
    Serial.print("電流有效值：" + String(Irms) + " (安培) \n");
    Serial.print("視在功率：" + String(Irms * 110.0) + "(伏安)\n" );
    Serial.print("--------------------------\n");
}
```
### 電路圖：
![](https://i.imgur.com/Bf5iiCk.png)

## ***小結***
本文，我們詳細地分析了單一比流器的相關事項，可以歸納出以下幾點︰
* 負載電阻在\\(66.3\Omega\\)時，比流器轉換出的峰對峰電壓在五伏特內。
    * 此時\\(Calibration\\)為\\(45.25\ \Omega^{-1}\\)。
* 原始電壓必須拉升準位，調整負電壓部分為正值。
    * 可以透過\\(V_{cc}\\)分壓電路，分出一半的電源電壓進行拉升。
* 可以透過程式找尋出電力系統在不同頻率下，此單晶片應設定的的採樣值。


## ***補充***
* 由於ARM Cortex系列的單晶片，其電源電壓僅3.3V，根據本文的計算思路：
    * 負載電阻\\(R_{Burden} \le 43.7\ \Omega \ \mid \ 使用此電阻令信號電壓峰對峰在3.3V以內\\)
    * \\(Calibration = 3000\ \div\ 73.75 \approx 68.6\ \Omega^{-1}\\)
    * 採樣值\\(Sampling\\) 沒有改變，因為這個數值是基於電力系統頻率。
    * **不需要在軟體層面設定任何東西！**，EMONLib會在底層偵測該單晶片提供的電源電壓，自動偵測到拉升電壓並自動調整算法。
* 關於比流器SCT013，由於線圈匝數不同，需要重新計算，根據本文的計算思路：
    * SCT013的線圈匝數比為\\(2000:1\\)，最大可量測\\(100\ A\\)的電流量
    * 負載電阻\\(R_{Burden} \le 35.4\ \Omega \\)
    * \\(Calibration = 2000\ \div\ 35.4 \approx 56.5\ \Omega^{-1}\\)
    * 採樣值\\(Sampling\\) 沒有改變，因為這個數值是基於電力系統頻率。
* 對於計算\\(50Hz\\)電力系統的採樣值 \\(Sampling\\)的程式該如何修改：
    * 根據 $$ T = 1 \div f = \frac{1}{60} = 20 ms \ \ \mid \ if\ 50Hz\  $$
    我們可以修改判斷式內的數值，以對20毫秒的採樣值做檢測：
```cpp
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
		//如果此採樣值令函數執行時間在19~21毫秒，代表該採樣值能很好運作在50Hz電力系統上。
		if (timeStamp[1] >= 19998 && timeStamp[1] <= 20002){
			Serial.print("Sampling數值: " + String(i) + " 為最佳解！\n");
		}
		else if (timeStamp[1] >= 19980 && timeStamp[1] <= 20020){ 
			Serial.print("Sampling數值: " + String(i) + " 接近最佳解！\n");
		}
		else if (timeStamp[1] >= 19700 && timeStamp[1] <= 20300){ 
			Serial.print("Sampling數值: " + String(i) + " 是個極佳的選擇！\n");
		}
		else if (timeStamp[1] >= 19000 && timeStamp[1] <= 21000){ 
			Serial.print("Sampling數值: " + String(i) + " 是個好選擇！\n");
		}
		Serial.print("採樣" + String(i) + "次時，函數時間為：" + String(timeStamp[1]) + "微秒");
		Serial.print("\n----------------\n");
		
	}
	Serial.print("-------------程式結束-------------");
	while(1);
}
```