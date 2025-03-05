## 如何安装

```shell
conda create -n ngram python=3.11
conda activate ngram
pip install -r requirments.txt

cd csrc
cmake -B build -DPython_EXECUTABLE=$(which python)
cmake --build build
```