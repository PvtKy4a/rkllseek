# Overview
This project is designed to run DeepSeek-R1-Distill on Rockchip RK3588 NPU in chat mode.

The open-source models for this project is available at [Hugging Face](https://huggingface.co/models?sort=trending&search=rk3588).

## Build

Clone rkllseek

```bash
git clone https://github.com/PvtKy4a/rkllseek.git
```

Clone rkllm toolkit

`Please note that the version of the toolkit that the model you are going to use is converted for and the version of the toolkit you download must match. For example, if the model is converted for version 1.1.4, make sure you download version 1.1.4 of the toolkit!`

```bash
git clone https://github.com/airockchip/rknn-llm.git
```

The directory tree where you are building should look like this:

```bash
/path/where/you/build
├── rkllseek
└── rknn-llm
```

Install libreadline

This library provides a normal terminal input experience with familiar shortcuts, navigation, and history.

```bash
sudo apt install libreadline-dev
```

Build

```bash
cd ./rkllseek

chmod +x ./build.sh

./build.sh
```

You should get output like this:

```bash
[ 50%] Building CXX object CMakeFiles/rkllseek.dir/src/rkllseek.cpp.o
[100%] Linking CXX executable rkllseek
[100%] Built target rkllseek
-- Install configuration: "Release"
-- Installing: ../rkllseek/install/bin/rkllseek
-- Set non-toolchain portion of runtime path of "../rkllseek/install/bin/rkllseek" to ""
-- Installing: ../rkllseek/install/lib/librkllmrt.so
```

## Run

Enter the `install` directory and run using the following code:

```bash
cd ./install
```

Export library path

```bash
export LD_LIBRARY_PATH=./lib
```

Run

```bash
./bin/rkllseek /path/to/your/model.rkllm <max_new_tokens> <max_context_len>
```

About `max_new_tokens` and `max_context_len`:

In general, I am not an expert in LLM, and I do not fully understand how to choose these values ​​correctly. But, what I roughly understood:

- max_new_tokens - this is the maximum number of tokens the model can generate in response to your query. Once this limit is reached, it will stop, even if the response is not complete. This value can be up to 8192.

- max_context_len - this is the maximum number of tokens the model can process at a time, including the conversation history, new request, and generated tokens. Example: if the value is 4096 and your request + chat history takes up 4000 tokens, then the model can only generate 96 tokens in the response. This value can be up to 32768. This parameter determines the use of RAM.

Result

```bash
../rkllseek/install$ export LD_LIBRARY_PATH=./lib
../rkllseek/install$ ./bin/rkllseek ../DeepSeek-R1-Distill-Qwen-7B_W8A8_RK3588_o0.rkllm 2048 8192
rkllseek initializing...

I rkllm: rkllm-runtime version: 1.1.4, rknpu driver version: 0.9.8, platform: RK3588

Initializing success!

Enter "/?" or "/help" for help

You: 
```