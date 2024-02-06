# llama.cpp webserver based api + RPBot

used llama embedded webserver api for easier portability

## LlamaCppWeb config options

- `LlamaCppWeb`, `server`
  - type: `string`
  - host + port
  - default: `localhost:8080`


## RPBot config options
TODO: move rpbot to own repo

- `RPBot`, `system_prompt`(, opt contact ID)
  - type: `string`
  - System prompt that is prefixed
  - can contain spcific formatters
    - `{self_name}` username for specified chat
  - default: `Transcript of a group chat, where {self_name} talks to online strangers.\n{self_name} is creative and curious. {self_name} is writing with precision, but also with occasional typos.\n`

- `RPBot`, `min_messages`(, opt contact ID)
  - type: `int`
  - TODO: implement per ID
  - default: `4`

- `RPBot`, `max_interactive_delay`(, opt contact ID)
  - type: `float`
  - TODO: implement per ID
  - default: `4.0`


## example config

```json
{
	"LlamaCppWeb": {
		"server": "localhost:8080"
	},
	"RPBot": {
		"min_messages": 1,
		"system_prompt":  "A group chat with an artificial intelligence assistant called {self_name}. The assistant gives helpful and detailed answers to the chat participants's questions. The assistant has it's own opinions. {self_name} only ever answers when explicitly asked.\n"
		}
	}
}
```

