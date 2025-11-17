# CPSC440
The Midterm Alternative Project: RISC‑V Numeric Ops Simulator
Overview:
A complete RISC-V numeric-operations simulator implemented in only two C++ files.

This repository contains my submission for The Midterm Alternative Project, where I built a fully-functional RISC-V Numeric Operations Simulator including integer ALU ops, memory model, branching, M-extension behaviors, and IEEE-754 Float32 arithmetic — all implemented from scratch without using built-in arithmetic operations for the bit-level module. Unlike many multi-file implementations, my simulator is intentionally written in a two-file structure for clarity:

sim.cpp — a runnable RISC-V CPU simulator (fetch/decode/execute, memory, registers, PC control).

midterm.cpp — the numeric-ops engine implementing ADD/SUB, MUL/DIV (M-extension), two’s-complement, bit-vector logic, and IEEE-754 Float32 encode/decode & ALU operations strictly using bit-level logic.


Feature

⦁	RISC-V CPU Core (sim.cpp)
⦁	32 general-purpose registers (x0–x31)
⦁	Byte-addressable memory model (1 MB instruction + 1 MB data memory)
⦁	Instruction fetch/decode/execute pipeline
⦁	Supports key RV32I instructions:
	R-type: ADD, SUB, AND, OR, XOR, SLL, SRL, SRA
	I-type: ADDI, LW
	S-type: SW
	B-type: BEQ, BNE
	U-type: LUI, AUIPC
	J-type: JAL, HALT convention (jal x0, 0)
	JALR
⦁	Unaligned access warnings
⦁	Per-instruction execution tracing
⦁	Program loader for .hex files

Numeric Operations Module (midterm.cpp)
This file implements RISC-V numeric behavior without using C++ + − * / % << >> for the algorithmic core (as required).

Includes:
Two’s-Complement Engine
⦁	Manual encode/decode
⦁	Negation via bit-inversion + ripple-carry add
⦁	Sign extension and magnitude extraction

Bit-Level ALU
⦁	Full-adder
⦁	Ripple-carry addition & subtraction
⦁	Overflow, carry, zero, negative flags

M-Extension (Multiply/Divide)
⦁	MUL, MULH, signed/unsigned variants
⦁	32×32 → 64 shift-add multiplier
⦁	Restoring division (signed & unsigned)
⦁	Overflow detection on multiplication

IEEE-754 Float32 Implementation
⦁	Bit-accurate encoding & decoding
⦁	Addition & subtraction using:
	exponent alignment
	mantissa addition/subtraction
	normalization

⦁	Float32 multiplication using:
	24-bit mantissa multiply
	exponent adjustment
	normalization


Project Structure
  /project-root
│
├── sim.cpp          # RISC-V CPU simulator core
├── midterm.cpp      # All numeric & floating-point logic
└── README.md


How to Run

1. complie
g++ -std=c++17 sim.cpp -o sim
g++ -std=c++17 midterm.cpp -o midterm

2. Run CPU simulator
Place your test_base.hex in the same directory.
./sim

3. Run Numeric Tests
./midterm

What This Project Demonstrates
Understanding of binary arithmetic, full adders, two’s complement
Full RISC-V instruction cycle flow
Implementation of memory-safe byte-addressable RAM
Branching, PC updates, and register write-back
Bit-vector based floating-point arithmetic
Multiply & divide algorithms from scratch
Ability to consolidate complexity into a small, maintainable codebase

This project highlights the ability to implement low-level CPU behavior and software emulation using minimal files and clean code organization — proving that a multi-file architecture is optional, not required.
NO host float arithmetic inside conversion logic

