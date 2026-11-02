"""Conversations rendered through every template in templates/.

Each entry is the keyword arguments of transformers' render_jinja_template for one chat:
``messages`` plus optional ``tools``, ``documents``, ``add_generation_prompt`` and extra
template variables (``enable_thinking``). generate.py adds the special tokens and the
reasoning-field aliases transformers adds, and stores the resulting context in
expected/conversations.json, which is what the C++ suite renders.
"""

TOOLS = [
    {
        "type": "function",
        "function": {
            "name": "get_weather",
            "description": "Get the current weather in a city.",
            "parameters": {
                "type": "object",
                "properties": {
                    "city": {"type": "string", "description": "City name, e.g. Paris"},
                    "unit": {"type": "string", "enum": ["celsius", "fahrenheit"]},
                },
                "required": ["city"],
            },
        },
    }
]

CALL = {"type": "function", "id": "call_0001",
        "function": {"name": "get_weather", "arguments": {"city": "Paris", "unit": "celsius"}}}
CALL_STR_ARGS = {"type": "function", "id": "call_0002",
                 "function": {"name": "get_weather", "arguments": "{\"city\": \"Tōkyō\"}"}}

CONVERSATIONS = {
    # One user turn and the generation prompt
    "single": dict(messages=[{"role": "user", "content": "Hello!"}], add_generation_prompt=True),
    # A system prompt and several turns; quotes, markup characters and non-ASCII text, which the
    # transformers tojson neither sorts nor HTML-escapes
    "system": dict(messages=[
        {"role": "system", "content": "You are a helpful assistant."},
        {"role": "user", "content": "What is 2+2?"},
        {"role": "assistant", "content": "2+2 = 4."},
        {"role": "user", "content": "And \"3+3\" <in> café ünïcode 日本 & emoji 🙂?"},
    ], add_generation_prompt=True),
    # Training-style render: ends with the assistant turn, no generation prompt
    "nogen": dict(messages=[
        {"role": "user", "content": "Hi"},
        {"role": "assistant", "content": "Hello! How can I help?"},
    ], add_generation_prompt=False),
    # Tool schema, a call with dict arguments, its result and a final answer
    "tools": dict(messages=[
        {"role": "system", "content": "You can call tools."},
        {"role": "user", "content": "Weather in Paris?"},
        {"role": "assistant", "content": "", "tool_calls": [CALL]},
        {"role": "tool", "name": "get_weather", "tool_call_id": "call_0001",
         "content": "{\"temp\": 18, \"sky\": \"cloudy\"}"},
        {"role": "assistant", "content": "It is 18°C and cloudy in Paris."},
        {"role": "user", "content": "Thanks"},
    ], tools=TOOLS, add_generation_prompt=True),
    # Tool-call arguments given as a JSON string, as OpenAI-style clients send them
    "tool_args_str": dict(messages=[
        {"role": "user", "content": "Weather in Tokyo?"},
        {"role": "assistant", "content": None, "tool_calls": [CALL_STR_ARGS]},
        {"role": "tool", "tool_call_id": "call_0002", "content": "sunny"},
    ], tools=TOOLS, add_generation_prompt=True),
    # A past assistant turn with its reasoning
    "reasoning": dict(messages=[
        {"role": "user", "content": "Is 17 prime?"},
        {"role": "assistant", "content": "Yes, 17 is prime.",
         "reasoning_content": "17 has no divisors other than 1 and itself."},
        {"role": "user", "content": "And 21?"},
    ], add_generation_prompt=True),
    # Thinking switched off (Qwen3 and others read enable_thinking)
    "no_thinking": dict(messages=[{"role": "user", "content": "Quick: capital of France?"}],
                        add_generation_prompt=True, enable_thinking=False),
    # Content as a list of typed parts (multimodal message format, text only)
    "parts": dict(messages=[
        {"role": "system", "content": [{"type": "text", "text": "Be brief."}]},
        {"role": "user", "content": [{"type": "text", "text": "Describe"}, {"type": "text", "text": "a cat."}]},
    ], add_generation_prompt=True),
    # Retrieval documents (Command-R and Granite read them)
    "documents": dict(messages=[{"role": "user", "content": "Who wrote it?"}],
                      documents=[{"title": "Hamlet", "text": "A tragedy written by William Shakespeare."},
                                 {"title": "Notes", "text": "Written around 1600."}],
                      add_generation_prompt=True),
    # Two user turns in a row: templates that require alternation call raise_exception
    "no_alternation": dict(messages=[
        {"role": "user", "content": "First"},
        {"role": "user", "content": "Second"},
    ], add_generation_prompt=True),
}

# Conversations for one template only, for templates that read variables or a message format
# of their own; keyed by the template's file name without ".jinja"
TEMPLATE_CONVERSATIONS = {
    # firefunction-v2 reads the tool list as a JSON string in `functions` and the date in `datetime`
    "fireworks-ai-llama-3-firefunction-v2": {
        "functions": dict(messages=[{"role": "user", "content": "Weather in Paris?"}],
                          functions="[{\"name\": \"get_weather\", \"parameters\": {\"city\": \"string\"}}]",
                          datetime="2025-03-14", add_generation_prompt=True),
    },
    # TranslateGemma takes one typed text part with the language pair
    "google-translategemma-4b-it": {
        "translate": dict(messages=[{"role": "user", "content": [
            {"type": "text", "source_lang_code": "en", "target_lang_code": "de", "text": "Good morning"}]}],
                          add_generation_prompt=True),
    },
}
