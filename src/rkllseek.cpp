#include <string>
#include <iostream>
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
#define ASSISTANT_TOKEN "\n<｜Assistant｜>"
#define THINK_START_TOKEN "<think>\n"
#define THINK_STOP_TOKEN "</think>"
#define EOS_TOKEN "<｜end▁of▁sentence｜>\n"

static LLMHandle llm_handle = nullptr;

static std::string history;

static std::string request;

static std::string response;

static const char *is_think = NULL;

static bool is_abort = false;

static void exit_handler(int signal)
{
    std::cout << std::endl << "rkllseek exiting..." << std::endl;

    if (llm_handle != nullptr)
    {
        rkllm_destroy(llm_handle);
    }

    exit(signal);
}

static void abort_handler(int signal)
{
    if (llm_handle != nullptr && rkllm_is_running(llm_handle))
    {
        rkllm_abort(llm_handle);

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

        if (is_think == NULL)
        {
            is_think = strstr(result->text, THINK_STOP_TOKEN);
        }

        if (is_think != NULL)
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
    std::cout << std::endl << "/clear           Clear session context";
    std::cout << std::endl << "/bye             Exit";
    std::cout << std::endl << "/? shortcuts     Regenerate last response";
    std::cout << std::endl;
}

static void print_help_shortcuts(void)
{
    std::cout << std::endl << "Ctrl + l         Clear the screen";
    std::cout << std::endl << "Ctrl + c         Stop the model from responding";
    std::cout << std::endl << "Ctrl + d         Exit (/bye)";
    std::cout << std::endl;
}

int main(int argc, char **argv)
{
    RKLLMParam llm_param;

    RKLLMInferParam rkllm_infer_param;

    RKLLMInput rkllm_input;

    int ret;

    if (argc < 4)
    {
        std::cout << "Usage: " << "rkllseek" << " /path/to/model.rkllm max_new_tokens max_context_len" << std::endl;

        exit_handler(0);
    }

    std::cout << "rkllseek initializing..." << std::endl << std::endl;

    llm_param = rkllm_createDefaultParam();

    llm_param.model_path = argv[1];

    llm_param.top_k = 20;
    llm_param.top_p = 0.95;
    llm_param.temperature = 0.6;
    llm_param.repeat_penalty = 1.1;
    llm_param.frequency_penalty = 0.0;
    llm_param.presence_penalty = 0.0;

    llm_param.max_new_tokens = std::atoi(argv[2]);
    llm_param.max_context_len = std::atoi(argv[3]);
    llm_param.skip_special_token = true;
    llm_param.extend_param.base_domain_id = 0;

    ret = rkllm_init(&llm_handle, &llm_param, rkllm_callback);

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

        if (strcmp(prompt, "/? shortcuts") == 0)
        {
            print_help_shortcuts();

            continue;
        }

        if (strcmp(prompt, "/clear") == 0)
        {
            history.clear();

            response.clear();

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

            history.append(request);
        }

        response.clear();

        rkllm_input.prompt_input = (char *)history.c_str();

        std::cout << std::endl << "DeepSeek: ";

        ret = rkllm_run(llm_handle, &rkllm_input, &rkllm_infer_param, NULL);

        if (ret != 0)
        {
            std::cout << "RKLLM run error!" << std::endl;

            exit_handler(ret);
        }
    }

    exit_handler(0);
}
