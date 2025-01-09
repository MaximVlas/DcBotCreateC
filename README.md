# Discord Bot Creation in C

A lightweight Discord bot implementation using the Discord Gateway API and libwebsockets.

## Installation

```bash
# Install dependencies on Arch Linux
sudo pacman -S gcc make cmake json-c libwebsockets curl
```

## Features

- Websocket connection handling
- Heartbeat management
- Command processing system
- Message sending capabilities

## Command List

| Command | Description                |              
|---------|----------------------------|
| !ping   | Check if bot is alive      |
| !hello  | Get a greeting             |
| !help   | Show help message          |  
| !roll   | Roll random number (1-100) |
| !fact   | Get a random fact          |
| !time   | Show server time           |
| !echo   | Repeat your message        |

## Project Structure

```
.
├── main.c
├── discord_bot.c
├── discord_bot.h
├── commands.c
├── commands.h
├── LICENSE
└── README.md
```

## Todo

- [ ] Add proper error handling
- [ ] Implement rate limiting
- [ ] Add slash command support


## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the  Apache License - see the LICENSE file for details.
