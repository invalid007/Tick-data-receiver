# Tick-data-receiver

This project connects to a trading server, receives market trade data, and saves it to a JSON file. It also requests any missing trade sequences from the server.

## Requirements

- Windows OS
- Visual Studio (or MinGW for GCC)
- Winsock2 (included in Windows SDK)

## Setup Instructions

### 1. Install Dependencies

Ensure you have a C++ compiler installed. If using MinGW, install it from:

- [MinGW-w64](https://www.mingw-w64.org/)

### 2. Clone the Repository

```sh
git clone https://github.com/your-repo/trade-data-collector.git
cd trade-data-collector
```

### 3. Compile the Code
```sh
g++ main.cpp -o trade_collector -lws2_32
```

Using Visual Studio Developer Command Prompt:
```sh
cl main.cpp /link ws2_32.lib
```

### 4. Run the Program
```sh
trade_collector.exe
```

### 5. Output

- The collected trade data is saved in **`abx_trades.json`**.
- Missing sequences are automatically requested from the server.
- The console logs received and recovered trade sequences.

## Notes

- Ensure the trading server is running and accessible at **`127.0.0.1:3000`**.
- Modify `SERVER_ADDRESS` and `SERVER_PORT` in the code if needed.
- Run the program with administrator privileges if required.

## License

This project is licensed under the **MIT License**.

