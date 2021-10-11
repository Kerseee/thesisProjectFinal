# 安裝說明 (Windows vscode)
1. 將此專案 clone 到 local 上
2. 下載[這個 vscode 的設定檔 zip](https://drive.google.com/file/d/1IAMeKdYFQjQ0zQ6Cb0M_iDFldrOnbWSW/view?usp=sharing)，並解壓縮。
3. 上述設定檔資料夾解壓縮後，確認資料夾名稱為 .vscode，將該資料夾放入專案中，與 main.cpp 同一層
4. 用 vscode 開啟，按 ctrl + shift + b 就可編譯，生出 main.exe 檔

# Input data 更改說明
以下列出幾項需要更改的資料，更改後的資料格式與範例請見 input/CaseStudy/data1/ 資料夾，檔名請勿更動。
另外請注意所有 csv 檔裡不可有空值。

新增 metadata.txt：
- metadata.txt：紀錄整份資料規模的 txt 檔，裡面內容數字可改，但項目順序與格式不可變更，不可有其他註解或文字，
    - roomtype 3：3 種房型
    - before 60：其他檔案裡 before 最大值
    - booking_days 30：接單日編號最大值，30 代表整個系統裡接單日編號從 30 到 0，總共 31 天
    - periods_per_day 6：接單日一日有幾期，總接單期數為 (booking_days + 1) * periods_per_day
    - service_period 30：服務期編號最大值，30 代表服務期編號從 1 到 30，總共 30 期，服務期一期等於一天
        - 服務期第一期 = 接單期第 0 日 = 接單期 6~1 期
        - before、booking_days、service_period 的關係為： before = booking_days + service_period - 1，例如接單日 1, 服務期 1 的話，before 為 1
    - room_num 25 25 25：代表 3 種房型各有幾間，左到右為房型低到高（1 到 3），roomtype 數字為多少，room_num 就要有多少個數字，數字間以一個空白隔開

p_si.csv 更改：
- 最左邊加上一個 column 代表是服務期第幾期，從 1 開始。
- column 編號改成 1, 2, 3，代表原本的 L, M, H

# 操作說明
執行：在此專案 main.exe 所在的 directory 輸入指令： ./main.exe
以下為操作範例：

輸入資料檔所在的資料夾<br>
![螢幕快照 2021-10-11 下午4 04 01](https://user-images.githubusercontent.com/31405635/136754309-080ef21f-9a95-4a96-b5d4-f6d8e01c3801.png)

輸入規劃結果存放的資料夾（如果該資料夾不存在的話會自動新增資料夾，但該資料夾的上一層資料夾必須存在！）<br>
![螢幕快照 2021-10-11 下午4 05 56](https://user-images.githubusercontent.com/31405635/136754578-385e7d47-6fbd-4bd8-b35e-40d897c5fd17.png)

輸入 sample size，此數字會決定 stochastic methods 預測未來需求的 scenarios 數量，越大會跑越久，但理論上會越準<br>
![螢幕快照 2021-10-11 下午4 07 43](https://user-images.githubusercontent.com/31405635/136754814-4b720bf7-392c-49c3-a4ae-544c0fa49a25.png)

輸入 alpha 的區間與 step size，此處需要輸入三個介於 0 到 1 之間的數值，皆以空格隔開。這些 alpha 為 adjusted methods 的參數。
舉例來說輸入 0 1 0.1 代表 adjusted methods 需要跑過這些 alphas: [0, 0.1, 0.2, ..., 0.8, 0.9, 0.1]
所以如果輸入 0.5 1 0.5 則代表需要跑過的 alphas 只有 [0.5, 1]<br>
![螢幕快照 2021-10-11 下午4 10 07](https://user-images.githubusercontent.com/31405635/136755156-576cf703-1265-441a-870d-9edf438e118c.png)

輸入想要跑幾次實驗<br>
![螢幕快照 2021-10-11 下午4 14 04](https://user-images.githubusercontent.com/31405635/136755746-c9addc17-b01f-4667-be95-d60d287b24dd.png)

以上資訊輸入完後專案就會開始跑，並陸續印出以下結果：<br>
![螢幕快照 2021-10-11 下午4 14 55](https://user-images.githubusercontent.com/31405635/136755901-e77759f1-fabe-44a3-bef5-73d0f217177e.png)

補充說明，系統跑的流程如下：
1. 讀資料
2. 生 expected & estimated demands
    - 這兩個資料結構分別是 deterministic/stochastic 演算法用來預測未來狀況的，以判斷目前這張單是否需要接。這兩個結構跟只 based on input data 但不 based on experiments，因此在執行實驗前會先生。如果期數越多（booking_periods, service_periods）或 sample_size 越大，就會升越久。
3. 跑實驗
    - 每次實驗都會生一組 orders 和 一組 demands，逐一跑完所有的方法並各自存檔後，才會開啟下一次實驗。
    - 所以假設在第 501 次時跑太久想切掉也沒關係，因為前 500 次每種方法都是跑完就直接紀錄了。

# Code 架構簡介
這次的 case study 把以前的 code 整個 refactor 過，如果需要的話只要看 src/ 資料夾裡面檔名叫 caseXXXX 的即可。
總共有五組檔案，以下分別簡介這五組檔案的作用，如果需要的話可自行修改。

- caseController
    - 實驗主控台，控制整個實驗流程，包括呼叫讀檔、呼叫生 orders, demands 等 events、呼叫 planners 進行規劃以及存檔
- casePlanner 
    - 規劃演算法，執行一次實驗所需要的所有規劃，並回傳結果
- caseGenerator
    - 產生隨機事件，輸入資料檔後會生成演算法需要的 orders, demands, expected demands, estimated demands 等資料
- caseData
    - 讀資料檔相關
- caseStructures
    - 實驗進行中需要的其他資料結構   

