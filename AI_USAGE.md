Overview

AI tools were used selectively to assist with understanding complex bit-level arithmetic and verifying logic. The majority of implementation — including ALU, MDU, FPU, and helper utilities — was hand-coded and debugged manually.  
AI assistance was treated as a learning resource rather than an automated generator.

---

Tools Used

| Tool | Purpose | Frequency |
|------|----------|------------|
| **ChatGPT (OpenAI)** | Clarified algorithm steps for two’s-complement arithmetic, overflow flags, RISC-V M-extension logic, and IEEE-754 float operations. Helped refactor some functions for readability and modularity. | Moderate |
| **GitHub Copilot** | Minor syntax hints, vector initialization, loop boilerplate. | Low |

---
 How AI Helped Implementation

Arithmetic Logic Unit (ALU)
- Explained the ripple-carry adder concept so I could implement `fullAdder()` and `addBits()` using only bitwise operations.  
- Helped me understand correct overflow detection (`V`) and carry flag (`C`) behavior in signed arithmetic.  
- Guided me on testing edge cases such as `0x7FFFFFFF + 1 → 0x80000000`.

Shifters
- Provided examples of simulating logical and arithmetic shifts (`SLL`, `SRL`, `SRA`) without built-in operators.  
- Helped me debug bit propagation and indexing errors.

Multiply/Divide Unit (MDU)
- Clarified the shift-add algorithm for unsigned multiplication and how to extend it to signed operands.  
- Explained the **restoring division** process, which I then coded manually (`divu_restoring`, `div_signed`).  

Floating-Point Unit (FPU)
- Taught me how IEEE-754 float32 values are encoded (sign, exponent, fraction) 
- Verify correctness of the `floatAddSub()` and `floatMultiply()` functions

Register File
- Suggested a design pattern for a small `Reg` and `RegisterFile` class that simulates synchronous load/clear and keeps `x0` hard-wired to zero.  
- I implemented the code manually using loops and vector copies, avoiding any built-in arithmetic.

Debugging & Output
- Explained causes of mismatched binary/hex outputs.  
- Helped me format output functions like `bitsToHexN()` and `bitsToBinStr()` for clear trace printing.  
- Assisted in verifying constraint compliance (no `+ - * / % << >>`).
- Debug error messages like segmentation fault, declare scope and etc.

---

 What Was Done Manually

- Full hand-coding of all bitwise logic, vector operations, and test cases.  
- Manual debugging of ALU, MDU, and FPU operations in C++.  
- Hand-written tests to validate overflow and normalization behavior.  
