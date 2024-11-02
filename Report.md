# Cosimulation Environment Report

---

## Issue 2: Initial Setup, Debugging, and Basic Operations

### 1. Initial Setup
The QEMU + SystemC cosimulation environment was set up using the provided repository.

- **Environment Configuration**: SystemC was configured for the development environment, and cosimulation was established with QEMU.
- **Running the Environment**: The cosimulation environment was run with QEMU executing the `debugread` program inside the simulated hardware, which interfaces with SystemC components.
- **Validation**: Successfully ran `debugread` in QEMU, verifying correct userspace output from QEMU.

### 2. Debugging the Environment
To debug the interaction between software and hardware:

- **Debugging `pDev` Address**: Used `gdb` to examine the address pointed to by `pDev` in `debugread`. Verified its alignment with the physical address in the `mmap` function, understanding how it correlates with SystemC's mapped register space.
- **Simulated Hardware Debugging**: Enabled debug flags (`-g`, `-O0`) in Makefile, cleaned the build, and debugged the SystemC code using `gdb`.
- **Observations**: In `debugdev.cc`, set breakpoints on `b_transport` and confirmed TLM payload addresses matched those accessed by the `pDev` pointer in `debugread`.

### 3. Volatile Keyword Impact
Removing `volatile` from the MMR read instruction in `debugread` led to compiler optimization issues. The `volatile` keyword is crucial in ensuring that hardware registers are read at each access, as removing it allows the compiler to potentially cache values, leading to incorrect or outdated data.

### 4. Performing Operations on the Debug Device
- **Sending Message to SystemC Console**: By writing to offset `0x04`, the message "Cosimulation Works" was successfully printed in the SystemC console.
- **Investigating Offset `0xf0`**: Observed that reading from offset `0xf0` caused an address error response due to the configuration in `debugdev.cc`.
- **Stopping Simulation**: Identified an MMR register responsible for halting simulation and modified `debugread.c` to trigger this stop command as the final operation.

---

## Issue 3: Adding a Control Status Register (CSR)

### 1. CSR Register in Vector Processor
- **CSR Implementation**: Added a new `uint32_t` member variable `CSR` in `vector_processor` to control processor operations.
- **MMR Access**: Modified the `b_transport` function to handle TLM read/write operations on `CSR`. A write operation sets `CSR`, while a read operation returns its value.

### 2. Interface with CSR Register from Userspace
- **New Program `vector_testbench.c`**: Cloned `debugread.c` into `vector_testbench.c`, updated the Makefile, and created an additional `vector_testbench` binary.
- **Testing CSR**: Wrote a value to `CSR` and read it back in `vector_testbench`, confirming the expected value and interface correctness.

---

## Issue 4: Simulating an Operation with Processing Delay

### 1. Processing Delay in SystemC
- **Creating `op_thread`**: Added an `SC_THREAD` `op_thread` to `vector_processor` to simulate an operation delay.
- **Start Event**: Created an `sc_event` called `start` which `op_thread` waits for, simulating processing triggered by the CSR write.
- **5ms Delay Simulation**: `op_thread` includes a 5ms annotated delay using `wait(sc_time(5, SC_MS))`, after which it sets `CSR` back to `0x0` to signal operation completion.

### 2. Interfacing with the IP from Userspace
- **Starting Processing**: Modified `vector_testbench.c` to write `1` to `CSR`, triggering the operation in `vector_processor`.
- **Measuring Processing Time**: Measured time from starting the operation until completion by:
  - Using `gettimeofday` to get QEMU time.
  - Reading SystemC time via the TRACE register at `0xC4`.

### Reflection on Time Differences
The differences between QEMU-reported and SystemC-reported times are attributed to:

- **QEMU Real-time Simulation**: QEMU aims to emulate real time, often faster than SystemC’s discrete-event clock.
- **SystemC Event-driven Time**: SystemC’s time advances only when events occur, reflecting a simulated hardware timing more precisely.
- **Cosimulation Asynchrony**: QEMU and SystemC operate on independent time domains, leading to inevitable time desynchronization between real-world and simulated environments. 

--- 
