# Simple Character Device Driver

## Overview
This project implements a simple Linux character device driver named `simple_char_driver`. It demonstrates basic interactions with a character device, allowing for reading from and writing to a device buffer. The driver supports standard operations such as reading, writing, and seeking within the buffer.

## Features
- **Character Device Operations**: The module supports essential operations:
  - Read data from the device.
  - Write data to the device.
  - Seek to a specific position in the device buffer.

- **Buffer Management**: The driver manages a fixed-size buffer (512 bytes) where data can be read from and written to.

- **Error Handling**: It includes error handling for various conditions such as oversized writes and invalid seeks.

## Requirements
- Linux kernel headers matching your running kernel version.
- Build tools such as `make` and `gcc`.
- Kernel module utilities (`kmod`).

## Installation

1. **Clone the repository**:
    ```bash
    git clone https://github.com/yourusername/simple-char-driver.git
    cd simple-char-driver
    ```

2. **Build the module**:
    ```bash
    make
    ```

3. **Load the module**:
    ```bash
    sudo depmod -a
    sudo modprobe simple_char_driver
    ```

4. **Verify the module is loaded**:
    ```bash
    dmesg | tail -n 10
    ```

## Interact with the Device

After loading the module, you can interact with the device using standard file operations:

1. **Test writing a simple message and reading it back**:
    ```bash
    echo "hello driver" > /dev/pcd
    cat /dev/pcd
    ```

2. **Test oversized write (should fail)**:
    ```bash
    echo "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. ..." > large_file.txt
    cp large_file.txt /dev/pcd || echo "Write failed as expected (file too large)"
    ```

3. **Test successful write with a smaller file**:
    ```bash
    echo "Lorem ipsum" > small_file.txt
    cp small_file.txt /dev/pcd
    cat /dev/pcd
    ```

4. **Test seek operations**:
    ```bash
    echo "abcdefghij" > /dev/pcd

    # Read first 5 bytes
    dd if=/dev/pcd bs=1 count=5 skip=0 2>/dev/null

    # Read 3 bytes starting from position 3
    dd if=/dev/pcd bs=1 count=3 skip=3 2>/dev/null

    # Read last 2 bytes
    dd if=/dev/pcd bs=1 count=2 skip=$(expr `stat -c %s /dev/pcd` - 2) 2>/dev/null
    ```

5. **Remove the module**:
    ```bash
    sudo modprobe -r simple_char_driver
    ```

## Cleanup

To remove the compiled module and associated files, run:
```bash
make clean
```
## Usage

This kernel module serves as an educational tool for understanding how to implement a character device driver in Linux. It can also be extended to include additional features and behaviors as needed.


## License

This project is licensed under the GNU General Public License v2.0 - see the [LICENSE](LICENSE) file for details.


