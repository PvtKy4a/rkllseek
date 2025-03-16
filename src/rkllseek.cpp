#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>

#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h> // This include MUST come before "rkllm.h" because "rkllm.h" uses types from <stdint.h> but does not include it!

#include "rkllm.h"

#include <readline/readline.h>
#include <readline/history.h>

#define BOS_TOKEN "<｜begin▁of▁sentence｜>"
#define USER_TOKEN "<｜User｜>"
#define ASSISTANT_TOKEN "<｜Assistant｜>"
#define THINK_START_TOKEN "<think>\n"
#define THINK_STOP_TOKEN "</think>"
#define EOS_TOKEN "<｜end▁of▁sentence｜>\n"

static LLMHandle rkllm_handle = NULL;

static std::string history;

static std::string request;

static std::string response;

static bool include_reasoning = true;

static bool include_history = true;

static const char *is_think = NULL;

static bool is_abort = false;

static void exit_handler(int signal)
{
    std::cout << std::endl << "rkllseek exiting..." << std::endl;

    if (rkllm_handle != NULL)
    {
        rkllm_destroy(rkllm_handle);
    }

    exit(signal);
}

static void abort_handler(int signal)
{
    if (rkllm_handle != nullptr && rkllm_is_running(rkllm_handle))
    {
        rkllm_abort(rkllm_handle);

        is_abort = true;
    }
    else
    {
        if (rl_end == 0)
        {
            std::cout << std::endl << "Use Ctrl + d or /bye to exit.";
        }
        rl_crlf();
        rl_free_line_state();
        rl_on_new_line();
        rl_initialize();
        rl_redisplay();
    }
}

static void rkllm_callback(RKLLMResult *result, void *userdata, LLMCallState state)
{
    (void)userdata;

    switch (state)
    {
    case RKLLM_RUN_FINISH:
        if (is_abort)
        {
            history.erase(history.end() - request.length(), history.end());

            request.clear();

            is_abort = false;
        }
        else
        {
            response.append(EOS_TOKEN);

            if (!include_reasoning)
            {
                history.erase(history.end() - strlen(THINK_STOP_TOKEN), history.end());
            }

            history.erase(history.end() - strlen(THINK_START_TOKEN), history.end());

            history.append(response);
        }

        is_think = NULL;

        std::cout << std::endl;
        break;

    case RKLLM_RUN_ERROR:
        std::cout << std::endl << "RKLLM run error!" << std::endl;

        exit_handler(RKLLM_RUN_ERROR);
        break;

    case RKLLM_RUN_NORMAL:
        std::cout << result->text;

        if (include_reasoning)
        {
            if (is_think == NULL)
            {
                is_think = strstr(result->text, THINK_STOP_TOKEN);
            }
            else
            {
                response.append(result->text);
            }
        }
        else
        {
            response.append(result->text);
        }
        
        break;

    default:
        break;
    }
}

static char *rl_gets(void)
{
    static char *line_read = (char *)NULL;

    /* If the buffer has already been allocated, return the memory
    to the free pool. */
    if (line_read)
    {
        free(line_read);
        line_read = (char *)NULL;
    }

    /* Get a line from the user. */
    line_read = readline("You: ");

    /* If the line has any text in it, save it on the history. */
    if (line_read && *line_read)
    {
        add_history(line_read);
    }

    return line_read;
}

static void print_help(void)
{
    std::cout << std::endl << "/regenerate      Regenerate last response";
    std::cout << std::endl << "/clear           Clear chat history";
    std::cout << std::endl << "/set reasoning   Enable DeepThink (enabled by default)";
    std::cout << std::endl << "/unset reasoning Disable DeepThink";
    std::cout << std::endl << "/set history     Enable chat history (enabled by default)";
    std::cout << std::endl << "/unset history   Disable chat history";
    std::cout << std::endl << "/save <file>     Save chat history to specified file";
    std::cout << std::endl << "/load <file>     Load chat history from specified file";
    std::cout << std::endl << "/bye             Exit";
    std::cout << std::endl;
    std::cout << std::endl << "Ctrl + l         Clear the screen";
    std::cout << std::endl << "Ctrl + c         Stop the model from responding";
    std::cout << std::endl << "Ctrl + d         Exit (/bye)";
    std::cout << std::endl;
}

static void save_history(const char *file_path)
{
    std::ofstream out;

    out.open(file_path);

    if (!out.is_open())
    {
        std::cout << std::endl << "Save failed!" << std::endl;

        return;
    }

    out << history;

    out.close();

    std::cout << std::endl << "Save success!" << std::endl;
}

static void load_history(const char *file_path)
{
    std::ifstream in;

    std::string line;

    in.open(file_path);

    if (!in.is_open())
    {
        std::cout << std::endl << "Load failed!" << std::endl;

        return;
    }

    if (!history.empty())
    {
        history.clear();
    }

    while (std::getline(in, line))
    {
        history.append(line);
    }

    in.close();

    std::cout << std::endl << "Load success!" << std::endl;
}

int main(int argc, char **argv)
{
    RKLLMParam llm_param;

    RKLLMInferParam rkllm_infer_param;

    RKLLMInput rkllm_input;

    int ret;

    if (argc < 4)
    {
        std::cout << "Usage: " << "rkllseek" << " /path/to/model.rkllm <max_new_tokens> <max_context_len>" << std::endl;

        exit_handler(0);
    }

    std::cout << "rkllseek initializing..." << std::endl << std::endl;

    llm_param = rkllm_createDefaultParam();

    llm_param.model_path = argv[1];
    llm_param.max_context_len = std::atoi(argv[3]);
    llm_param.max_new_tokens = std::atoi(argv[2]);
    llm_param.top_k = 20;
    llm_param.top_p = 0.95;
    llm_param.temperature = 0.6;
    llm_param.repeat_penalty = 1.1;
    llm_param.frequency_penalty = 0.0;
    llm_param.presence_penalty = 0.0;
    llm_param.mirostat = 0;
    llm_param.mirostat_tau = 3.0;
    llm_param.mirostat_eta = 0.1;
    llm_param.skip_special_token = true;
    llm_param.extend_param.base_domain_id = 0;

    ret = rkllm_init(&rkllm_handle, &llm_param, rkllm_callback);

    if (ret != 0)
    {
        std::cout << "Initializing failed!" << std::endl;

        exit_handler(ret);
    }

    memset(&rkllm_infer_param, 0, sizeof(RKLLMInferParam));
    rkllm_infer_param.mode = RKLLM_INFER_GENERATE;

    rkllm_input.input_type = RKLLM_INPUT_PROMPT;

    rl_catch_signals = 0;

    signal(SIGINT, abort_handler);

    std::cout << "Initializing success!" << std::endl;

    std::cout << std::endl << "Enter \"/?\" or \"/help\" for help" << std::endl;

    while (true)
    {
        char* prompt;

        std::cout << std::endl;

        prompt = rl_gets();

        if (rl_eof_found || strcmp(prompt, "/bye") == 0)
        {
            exit_handler(0);
        }

        if (prompt == NULL)
        {
            continue;
        }

        if (strcmp(prompt, "/?") == 0 || strcmp(prompt, "/help") == 0)
        {
            print_help();

            continue;
        }

        if (strcmp(prompt, "/clear") == 0)
        {
            history.clear();

            response.clear();

            continue;
        }

        if (strcmp(prompt, "/set reasoning") == 0)
        {
            include_reasoning = true;

            continue;
        }

        if (strcmp(prompt, "/unset reasoning") == 0)
        {
            include_reasoning = false;

            continue;
        }

        if (strcmp(prompt, "/set history") == 0)
        {
            include_history = true;

            continue;
        }

        if (strcmp(prompt, "/unset history") == 0)
        {
            include_history = false;

            continue;
        }

        if (strstr(prompt, "/save") != NULL)
        {
            save_history(prompt + strlen("/save "));

            continue;
        }

        if (strstr(prompt, "/load") != NULL)
        {
            load_history(prompt + strlen("/load "));

            continue;
        }

        if (strcmp(prompt, "/regenerate") == 0)
        {
            if (history.empty())
            {
                std::cout << std::endl << "There have been no requests yet." << std::endl;

                continue;
            }

            if (request.empty())
            {
                std::cout << std::endl << "The last request did not complete successfully." << std::endl;

                continue;
            }

            history.erase(history.end() - response.length(), history.end());
        }
        else
        {
            request.clear();

            request.append(BOS_TOKEN);
            request.append(USER_TOKEN);
            request.append(prompt);
            request.append(ASSISTANT_TOKEN);
            request.append(THINK_START_TOKEN);

            if (!include_reasoning)
            {
                request.append(THINK_STOP_TOKEN);
            }

            if (!include_history)
            {
                history.clear();
            }

            history.append(request);
        }

        response.clear();

        rkllm_input.prompt_input = (char *)history.c_str();

        std::cout << std::endl << "DeepSeek: ";

        ret = rkllm_run(rkllm_handle, &rkllm_input, &rkllm_infer_param, NULL);

        if (ret != 0)
        {
            std::cout << "RKLLM run error!" << std::endl;

            exit_handler(ret);
        }
    }

    exit_handler(0);
}
