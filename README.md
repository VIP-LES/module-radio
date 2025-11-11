# Raspberry Pi Pico Module Template

A starting point for all Pico family software development on our module-based PCB. This template provides a foundation for developing embedded applications that will eventually include OpenCyphal communication capabilities for interoperability with other systems.

## Overview

This template includes:
- Basic hardware initialization (SPI, I2C, UART, Timers)
- VS Code workspace configuration
- CMake build system setup
- Example code demonstrating common peripherals
- Future support for OpenCyphal communication protocol

## Prerequisites

- **VS Code** - Download from [https://code.visualstudio.com/](https://code.visualstudio.com/)
- **Raspberry Pi Pico VS Code Extension** - Required for development

## Installation & Setup

### 1. Install the Raspberry Pi Pico VS Code Extension

The easiest way to get started with Pico development is using the official VS Code extension:

1. Open VS Code
2. Go to the Extensions view (`Ctrl+Shift+X` or `Cmd+Shift+X`)
3. Search for "Raspberry Pi Pico"
4. Install the official extension by Raspberry Pi Foundation

Alternatively, you can install it from the command line:
```bash
code --install-extension raspberrypi.raspberry-pi-pico
```

### 2. Setup Development Environment

The VS Code extension will automatically handle:
- Pico SDK installation and management
- Toolchain setup (ARM GCC compiler)
- CMake configuration
- Build tools (Ninja)
- Debugging tools (OpenOCD, GDB)

For detailed setup instructions, refer to the official [Getting Started with Pico Guide](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf).

### 3. Using This Template

1. **Create a new repository** from this template:
   - Go to this repository on GitHub
   - Click "Use this template" → "Create a new repository"
   - Choose your repository name and settings
   - Clone your new repository locally

2. **Initialize submodules**:
   ```bash
   git submodule update --init --recursive
   ```

3. The VS Code extension should automatically configure the project. If prompted, select your Pico board type.

## Git Submodules

This template includes Git submodules for external dependencies, including the MCP251XFD CAN controller driver.

### Initializing Submodules
```bash
# Initialize and update all submodules
git submodule update --init --recursive
```

### Adding New Submodules
```bash
# Add a new submodule
git submodule add <repository-url> <path>
# Example: Adding OpenCyphal library (future enhancement)
# git submodule add https://github.com/OpenCyphal/libcanard.git lib/libcanard
```

### Working with Submodules
- Always commit submodule changes before committing the parent repository
- When pulling updates, use `git pull --recurse-submodules`
- To remove a submodule, follow Git's standard submodule removal process

## Building and Running

### Using VS Code Tasks

The project includes pre-configured VS Code tasks:

1. **Compile Project**: Use `Ctrl+Shift+P` → "Tasks: Run Task" → "Compile Project"
2. **Flash to Device**: Use "Run Project" or "Flash" task
3. **Debug**: Use the built-in debugger with the configured launch settings

### Command Line Build

```bash
# Create build directory (if not exists)
mkdir -p build
cd build

# Configure with CMake
cmake ..

# Build the project
make -j4
```

## Hardware Configuration

This template is configured for the following default pin assignments:

### SPI (SPI0)
- **MISO**: GPIO 16
- **CS**: GPIO 17
- **SCK**: GPIO 18
- **MOSI**: GPIO 19

### I2C (I2C0)
- **SDA**: GPIO 8
- **SCL**: GPIO 9

### UART (UART1)
- **TX**: GPIO 4
- **RX**: GPIO 5

### Pinout Reference

For complete pinout information, refer to: [https://pico.pinout.xyz/](https://pico.pinout.xyz/)

## Code Structure

```
├── module-name.c          # Main application code
├── CMakeLists.txt         # Build configuration
├── pico_sdk_import.cmake  # SDK import script
├── .vscode/               # VS Code configuration
│   └── c_cpp_properties.json
└── build/                 # Build output (auto-generated)
```

## Included Libraries

### MCP251XFD Driver
This template uses the MCP251XFD driver for CAN communication:
- **Repository**: [Emandhal/MCP251XFD](https://github.com/Emandhal/MCP251XFD)
- **Documentation**: [MCP251XFD Driver Library Guide](https://github.com/Emandhal/MCP251XFD/blob/master/Docs/MCP251XFD%20driver%20library%20guide%20(v1.0.5%20synchronous%20driver).pdf)

## Future Enhancements

This template is planned to include:

- **OpenCyphal Integration**: Communication protocol for distributed embedded systems
- **CAN Bus Support**: Hardware abstraction for CAN communication
- **Advanced Peripheral Examples**: PWM, ADC, DMA usage
- **Power Management**: Low-power modes and optimization
- **Unit Testing Framework**: Automated testing capabilities

## Documentation & Resources

- **Pico C/C++ SDK Documentation**: [https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html](https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html)
- **Getting Started Guide**: [https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf)
- **Pinout Reference**: [https://pico.pinout.xyz/](https://pico.pinout.xyz/)
- **Pico Examples Repository**: [https://github.com/raspberrypi/pico-examples](https://github.com/raspberrypi/pico-examples)

## Contributing

We welcome contributions to improve this template! Please follow these guidelines:

### Development Workflow

1. **Fork the repository** and create a feature branch:
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Make your changes** following the project coding standards:
   - Use consistent indentation (4 spaces for C code)
   - Add comments for complex logic
   - Follow existing naming conventions
   - Test your changes on actual hardware when possible

3. **Commit your changes** with descriptive messages:
   ```bash
   git commit -m "Add SPI flash memory support with wear leveling"
   ```

4. **Update documentation** as needed:
   - Update README if adding new features
   - Add inline code documentation
   - Update pinout configurations if changed

5. **Submit a Pull Request**:
   - Push your branch to your fork
   - Create a PR with a clear title and description
   - Reference any related issues
   - Include testing details and hardware compatibility

### Pull Request Guidelines

- **Title**: Use clear, descriptive titles (e.g., "Add I2S audio support for RP2040")
- **Description**: Explain what changes were made and why
- **Testing**: Describe how you tested the changes
- **Breaking Changes**: Clearly note any breaking changes
- **Documentation**: Ensure all new features are documented

### Code Standards

- Follow the existing code style and structure
- Add error handling where appropriate
- Include examples for new peripheral configurations
- Maintain compatibility with the VS Code extension workflow
- Test builds with both Debug and Release configurations

### Reporting Issues

When reporting bugs or requesting features:
- Use descriptive titles
- Include hardware details (Pico model, board revision)
- Provide steps to reproduce issues
- Include relevant code snippets or error messages

## License

This project is licensed under the terms specified in the LICENSE file.