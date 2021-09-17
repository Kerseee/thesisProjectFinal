以下簡介論文專案用的檔案：

一、程式碼

main.cpp：主程式，論文上用的實驗
src/：主程式會 call 的其他 function
src/planner：各 policies 和 DP 程式碼
src/data：讀資料用的程式碼
src/experimentor：用來測 upgrade algorithm 效能用的程式碼
model.py：生數值實驗資料、upgrade algorithm 效能實驗資料和 gurobi 的程式碼
備註：upgrade algorithm 效能原本是用來測試 upgrade algorithm 跟 gurobi 的差異，但裡面的 MSsmart 部分後來不用了

————————————————————————————————————————————————————————————————————————————————

二、input 資料

Numerical experiments 用的 input：
- input/params
- input/prob
- Example：上面兩種檔案的範例，也可以從 model.py 裡面看怎麼生的

Upgrade algorithm 實驗用的 input：
- input/MDexperiment：deterministic policies 用的 input
- input/MSexperiment：stochastic policies 用的 input

————————————————————————————————————————————————————————————————————————————————

三、實驗結果和分析

results/numerical_experiments：底下裝的是第五章數值實驗的結果和分析，以下介紹此資料夾下的檔案（省略此資料夾路徑）

- analysis.ipynb：用來分析的實驗結果的 jupyter lab notebook
- analysis/：分析結果存檔，裡面有 geometric/ 跟 poisson/ 兩個資料夾，分別存這兩種實驗的分析結果

- result：實驗結果原檔，裡面放的是各種 scenario 的結果 csv 檔，底下有兩層：
- result/[scenario]：各種 scenario 的檔名說明如下：
    - S：服務期數量
    - I：房型數量
    - C：房間數量
    - m：論文中提到的參數 m，後面數字是原數值*10，例如 m10 代表是 m=1
    - b：論文中提到的參數 beta，後面數字是原數值*100，例如 b70 代表是 beta=0.7
    - r：論文中提到的參數 theta，後面數字是原數值*10，例如 r2 代表是 theta=0.2
    - d：論文中提到的參數 gamma，後面數字是原數值*100，例如 d25 代表是 gamma=0.25
    - K：旅行社訂單數量
    - T：接單期數量
    - _poisson：代表散客機率分佈假設為 poisson，如果沒有這個 suffix 則代表假設 geometric
- result/[scenario]/[policy]_[suffix].csv：各 policies 的結果檔案，說明如下：
    - [suffix]：
        - _Near0N1000：最後用在論文上的資料，alpha 取 0 ~ 0.2 之間
        - _N1000：沒有用在論文上的資料，alpha 取 0 ~ 1 之間
        - 無 suffix：沒有用在論文上的資料，一 round 只跑 50 次，alpha 取 0 ~ 1 之間 
    - [policy]：
        - MD_naive：ND policy
        - MS_naive：NS policy
        - MD_bs_b[x]：AD policy，[x] 在 _Near0N1000 裡代表 alpha*100，在其他兩種 suffix 檔案裡則代表 alpha * 10
        - MS_bs_b[x]：AS policy，[x] 跟 AD policy 相同

————————————————————————————————————————————————————————————————————————————————

四、其他：
results/not_used_in_thesis：裡面放了一些寫論文過程中曾經試過的實驗結果，但後來沒有用在論文裡
main、main.exe：main.cpp 生出的執行檔，建議還是用 main.cpp 重生一遍再用比較好






