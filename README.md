## 如何安装Trie和SA

```shell
conda create -n ngram python=3.11
conda activate ngram
pip install -r requirements.txt

cd csrc
cmake -B build -DPython_EXECUTABLE=$(which python)
cmake --build build
```

### 如何安装REST的SA

```shell
conda activate ngram
cd DraftRetriever
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
maturin build --release --strip -i python3.9 # will produce a .whl file
cd target
cd wheel
pip3 install [.whl]
```