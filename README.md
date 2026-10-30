# Linux Programming Project 1

This project presents four Linux programming investigations covering ELF binary forensics and comparative threat assessment, Linux system-call tracing and execution analysis, CPython C extensions for high-performance data analysis, and signal-driven Linux service management. The implementations demonstrate practical application of C programming, Python integration, Linux system interfaces, binary analysis, and system-level debugging techniques.

---

## Compilation & Execution

### Question 1: ELF Binary Forensics

Compile both C programs:

```bash
cd question1
gcc -Wall -Wextra -o data_sync data_sync.c
gcc -Wall -Wextra -o data_sync_v2 data_sync_v2.c
```

Then run using ./data_sync and ./data_syncv2 respectively.

**Note:** The compiled executables are investigated using static analysis tools. They must not be executed.

### Question 2: Linux System Call Forensics

```bash
cd question2
gcc -Wall -Wextra -o backup_tool backup_tool.c
strace -f -o backup_trace.txt ./backup_tool
```

Refer to the question report for successful and failed execution experiments.

### Question 3: CPython C Extension

```bash
cd question3
python3 setup.py build_ext --inplace
python3 main.py
```

Step 2 produces the importable shared library: faststats.cpython-38-x86_64-linux-gnu.so.

### Question 4: Signal-Driven Linux Service Controller

```bash
cd question4
gcc -Wall -Wextra -Wpedantic -o server_controller server_controller.c
./server_controller
```

Use Linux `kill` commands from another terminal to test the four required signals.

---

## Summary of Projects

### Question 1: ELF Binary Forensics and Comparative Threat Assessment

- Develops two related C programs simulating file synchronization utilities and investigates their compiled ELF executables.
- Examines ELF headers, sections, segments, entry points, symbols, dynamic linking, and shared-library dependencies.
- Compares the two binaries to identify implementation differences using static analysis tools.

### Question 2: Linux System Call Forensics and Execution Analysis

- Develops a C-based backup simulation that creates directories, logs operations, copies files, and uses child processes.
- Uses `strace` to investigate file-related and process-related system calls.
- Compares successful and failed backup operations using actual system-call traces.

### Question 3: Building a CPython Extension for High-Performance Data Analysis

- Implements numerical data-analysis operations using pure Python and a compiled C extension.
- Integrates C functions with Python through the CPython C API, including input validation and memory management.
- Tests implementation equivalence and benchmarks performance across different dataset sizes.

### Question 4: Signal-Driven Linux Service Controller

- Implements a continuously running Linux service that monitors its operational state.
- Uses Unix signals to request status reports, toggle maintenance mode, and initiate service termination.
- Demonstrates safe signal handling using `sigaction()` and `volatile sig_atomic_t` flags.

---

## AI Assistance Used

AI tools were used to support:

- Debugging and troubleshooting implementation issues.
- Understanding Linux system concepts and programming interfaces.
- Reviewing code against assignment requirements.
- Improving technical documentation and report clarity.

---

## Resources

The following resources were used for learning and reference during the development of these projects:

- [Linux man-pages](https://man7.org/linux/man-pages/)
- [GNU Binutils Documentation](https://sourceware.org/binutils/docs/)
- [GCC Documentation](https://gcc.gnu.org/onlinedocs/)
- [Python/C API Reference](https://docs.python.org/3/c-api/)
- [Python Documentation](https://docs.python.org/3/)

---

## License

This project is under the MIT License.

---

## Author

Monica Dhieu

