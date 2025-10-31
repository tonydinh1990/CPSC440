#include <bits/stdc++.h>
using namespace std;

// ============================== Small helper macros ==============================
// Extract bits from a 32-bit word
static inline uint32_t get_bits(uint32_t x, int hi, int lo) {
    // inclusive [hi:lo], 0-based from LSB
    uint32_t width = (uint32_t)(hi - lo + 1);
    uint32_t mask = (width >= 32) ? 0xFFFFFFFFu : ((1u << width) - 1u);
    return (x >> lo) & mask;
}

// Converts a smaller signed number (like 12-bit immediate) into a 32-bit signed value.
static inline int32_t sign_extend(uint32_t val, int from_bits) {
    // Extend 'from_bits'-wide signed value to 32-bit signed
    uint32_t sign_bit = 1u << (from_bits - 1);
    uint32_t mask = (from_bits >= 32) ? 0xFFFFFFFFu : ((1u << from_bits) - 1u);
    uint32_t v = val & mask;
    if (v & sign_bit) {
        // negative
        uint32_t ext_mask = ~mask;
        v |= ext_mask;
    }
    return (int32_t)v;
}

// ============================== Memory model ==============================
// Simple byte-addressable memory with load/store operations
// Constructor: creates a memory of given siz
struct Mem {
    vector<uint8_t> bytes;      // memory bytes
    explicit Mem(size_t size_bytes) : bytes(size_bytes, 0) {}

    // Checks whether a read/write is inside memory bounds.
    bool in_range(uint32_t addr, size_t len = 1) const {
        if ((uint64_t)addr + len > bytes.size()) return false;
        return true;
    }

    // Little-endian 32-bit load/store
    // addr must be within range
    uint32_t load_u32(uint32_t addr) const {
        if (!in_range(addr, 4)) throw runtime_error("Data load out of range");
        return (uint32_t)bytes[addr] |
               ((uint32_t)bytes[addr+1] << 8) |     
               ((uint32_t)bytes[addr+2] << 16) |        
               ((uint32_t)bytes[addr+3] << 24);
    }

    void store_u32(uint32_t addr, uint32_t v) {
        if (!in_range(addr, 4)) throw runtime_error("Data store out of range");
        bytes[addr]   = (uint8_t)(v & 0xFF);
        bytes[addr+1] = (uint8_t)((v >> 8) & 0xFF);
        bytes[addr+2] = (uint8_t)((v >> 16) & 0xFF);
        bytes[addr+3] = (uint8_t)((v >> 24) & 0xFF);
    }

    // For instruction memory, instructions are word-addressed at word boundaries.
    void store_instr_word(uint32_t word_index, uint32_t instr) {
        uint32_t addr = word_index * 4;
        store_u32(addr, instr);
    }
};

// ============================== Register File ==============================
// 32 general-purpose integer registers x0..x31 (x0 is hardwired to zero)

// 32 registers, initialized to zero.
struct RegFile {
    uint32_t x[32]{}; // zero-initialized

    RegFile() { memset(x, 0, sizeof(x)); }

    // ===== Read/write with index checks =====
    uint32_t read(int idx) const {
        if (idx < 0 || idx > 31) throw runtime_error("bad reg index");
        return x[idx];
    }

    void write(int idx, uint32_t val) {
        if (idx < 0 || idx > 31) throw runtime_error("bad reg index");
        if (idx == 0) return; // x0 hardwired to 0
        x[idx] = val;
    }


    // ===== Debug dump of all registers =====
    void dump(ostream &os) const {
        os << hex << setfill('0');
        for (int i = 0; i < 32; i++) {
            os << "x" << dec << setw(2) << i << ": 0x" << hex << setw(8) << x[i];
            if (i % 4 == 3) os << "\n"; else os << "\t";
        }
        os << dec << setfill(' ');
    }
};

// ============================== ALU ==============================
// ALU with the operations needed for subset.
enum class ALUOp {
    ADD,
    SUB,
    AND_,
    OR_,
    XOR_,
    SLL,
    SRL,
    SRA
};

// Execute ALU operation
static inline uint32_t alu_ops(ALUOp op, uint32_t a, uint32_t b) {
    switch (op) {
        case ALUOp::ADD:  return a + b;
        case ALUOp::SUB:  return a - b;
        case ALUOp::AND_: return a & b;
        case ALUOp::OR_:  return a | b;
        case ALUOp::XOR_: return a ^ b;
        case ALUOp::SLL:  return a << (b & 0x1F);
        case ALUOp::SRL:  return a >> (b & 0x1F);
        case ALUOp::SRA:  return (uint32_t)(((int32_t)a) >> (b & 0x1F));
    }
    return 0; // unreachable
}

// ============================== CPU ==============================

// Simple RISC-V CPU simulator with integer registers and memory
struct CPU {
    // memories
    Mem imem; // instruction memory
    Mem dmem; // data memory

    // state
    uint32_t PC = 0; // program counter in bytes
    RegFile rf;

    // config flags
    bool trace = false;    // print per-instruction trace
    bool warn_unaligned = true; // warn on unaligned accesses

    // constructor
    explicit CPU(size_t imem_size = 1<<20, size_t dmem_size = 1<<20) // 1MB each by default
        : imem(imem_size), dmem(dmem_size) {}

    // =================== Loader for prog.hex ===================
    // Each line: 8 hex digits (one 32-bit word). Blank lines allowed.
    void load_hex_program(const string &path) {
        ifstream fin(path);
        if (!fin) throw runtime_error("Cannot open hex file: " + path);
        string line;
        uint32_t addr_word = 0;
        while (getline(fin, line)) {
            // trim spaces
            auto trim = [](string &s){                          // trim leading/trailing spaces
                size_t a = s.find_first_not_of(" \t\r\n");
                size_t b = s.find_last_not_of(" \t\r\n");
                if (a == string::npos) { s.clear(); return; }
                s = s.substr(a, b - a + 1);
            };
            trim(line);
            if (line.empty()) continue; // ignore blank lines
            // allow lowercase or uppercase hex without 0x prefix
            if (line.size() > 8) throw runtime_error("Invalid hex word length: " + line);
            // left-pad to 8 chars for stoul safety
            if (line.size() < 8) line = string(8 - line.size(), '0') + line;
            uint32_t instr = 0;

            try {
                instr = (uint32_t)stoul(line, nullptr, 16);         
            } catch(...) {
                throw runtime_error("Invalid hex number: " + line);
            }
            imem.store_instr_word(addr_word++, instr);
        }
    }

    // =================== Fetch/Decode helpers ===================
    // Fetch instruction at PC
    uint32_t fetch() {
        return imem.load_u32(PC);
    }

    // Field extractors
    // rd: bits[11:7], rs1: bits[19:15], rs2: bits[24:20]
    static inline int rd(uint32_t insn)   { return (int)get_bits(insn, 11, 7); }
    static inline int rs1(uint32_t insn)  { return (int)get_bits(insn, 19, 15); }
    static inline int rs2(uint32_t insn)  { return (int)get_bits(insn, 24, 20); }
    static inline uint32_t funct3(uint32_t insn){ return get_bits(insn, 14, 12); }
    static inline uint32_t funct7(uint32_t insn){ return get_bits(insn, 31, 25); }
    static inline uint32_t opcode(uint32_t insn){ return get_bits(insn, 6, 0); }

    // Immediate builders (sign-extended)
    // I-type, S-type, B-type, U-type, J-type
    static inline int32_t imm_i(uint32_t insn) { // 12-bit
        return sign_extend(get_bits(insn, 31, 20), 12);
    }

    // I-type
    static inline int32_t imm_s(uint32_t insn) { // 12-bit (store)
        uint32_t v = (get_bits(insn, 31, 25) << 5) | get_bits(insn, 11, 7);
        return sign_extend(v, 12);
    }

    // B-type
    static inline int32_t imm_b(uint32_t insn) { // 13-bit with 0 LSB
        uint32_t b12 = get_bits(insn, 31, 31);
        uint32_t b10_5 = get_bits(insn, 30, 25);
        uint32_t b4_1 = get_bits(insn, 11, 8);
        uint32_t b11 = get_bits(insn, 7, 7);
        uint32_t v = (b12 << 12) | (b11 << 11) | (b10_5 << 5) | (b4_1 << 1);
        return sign_extend(v, 13);
    }

    // U-type
    static inline int32_t imm_u(uint32_t insn) { // upper 20 bits, lower 12 are zeros
        return (int32_t)(insn & 0xFFFFF000u);
    }

    // J-type
    static inline int32_t imm_j(uint32_t insn) { // 21-bit with 0 LSB
        uint32_t b20 = get_bits(insn, 31, 31);
        uint32_t b10_1 = get_bits(insn, 30, 21);
        uint32_t b11 = get_bits(insn, 20, 20);
        uint32_t b19_12 = get_bits(insn, 19, 12);
        uint32_t v = (b20 << 20) | (b19_12 << 12) | (b11 << 11) | (b10_1 << 1);
        return sign_extend(v, 21);
    }

    // =================== Single instruction step ===================
    // Returns false if HALT detected, true otherwise.
    // Updates PC and state.
    bool step() {
        uint32_t insn = fetch();
        uint32_t opc = opcode(insn);
        uint32_t f3 = funct3(insn);
        uint32_t f7 = funct7(insn);
        int r_d  = rd(insn);
        int r1   = rs1(insn);
        int r2   = rs2(insn);
        uint32_t pc_next = PC + 4; // default next PC

        // Read registers
        auto R1 = rf.read(r1);
        auto R2 = rf.read(r2);

        // Trace printout
        if (trace) {
            cout << hex << setfill('0');
            cout << "PC=0x" << setw(8) << PC << " INSN=0x" << setw(8) << insn << dec << setfill(' ');
        }

        // Instruction decode and execute
        switch (opc) {
            case 0x33: { // R-type
                // funct3 selects op family; funct7 disambiguates (e.g., add/sub, srl/sra)
                uint32_t res = 0;
                if (f3 == 0x0) {
                    if (f7 == 0x00)      res = alu_ops(ALUOp::ADD, R1, R2); // add
                    else if (f7 == 0x20) res = alu_ops(ALUOp::SUB, R1, R2); // sub
                    else goto illegal;
                } else if (f3 == 0x7) {
                    if (f7 == 0x00)      res = alu_ops(ALUOp::AND_, R1, R2); // and
                    else goto illegal;
                } else if (f3 == 0x6) {
                    if (f7 == 0x00)      res = alu_ops(ALUOp::OR_, R1, R2); // or
                    else goto illegal;
                } else if (f3 == 0x4) {
                    if (f7 == 0x00)      res = alu_ops(ALUOp::XOR_, R1, R2); // xor
                    else goto illegal;
                } else if (f3 == 0x1) {
                    if (f7 == 0x00)      res = alu_ops(ALUOp::SLL, R1, R2); // sll
                    else goto illegal;
                } else if (f3 == 0x5) {
                    if (f7 == 0x00)      res = alu_ops(ALUOp::SRL, R1, R2); // srl
                    else if (f7 == 0x20) res = alu_ops(ALUOp::SRA, R1, R2); // sra
                    else goto illegal;
                } else {
                    goto illegal;
                }
                rf.write(r_d, res);
                if (trace) cout << "  R-type -> x" << r_d << " = 0x" << hex << setw(8) << res << dec;
                break;
            }
            case 0x13: { // I-type ALU (addi, slli/srli/srai via 0x13 too in full ISA, but we'll keep addi)
                int32_t imm = imm_i(insn);
                uint32_t res = 0;
                if (f3 == 0x0) { // addi
                    res = (uint32_t)((int32_t)R1 + imm);
                } else {
                    goto illegal; // keeping subset small
                }
                // Write result
                rf.write(r_d, res);
                if (trace) cout << "  addi -> x" << r_d << " = 0x" << hex << setw(8) << res << dec;
                break;
            }
            case 0x03: { // Loads
                // I-type
                int32_t imm = imm_i(insn);
                uint32_t addr = (uint32_t)((int32_t)R1 + imm);

                // Warn on unaligned access
                if (warn_unaligned && (addr & 3)) cerr << "[WARN] Unaligned LW at 0x" << hex << addr << dec << "\n";
                
                // Load based on funct3
                if (f3 == 0x2) { // LW
                    uint32_t val = dmem.load_u32(addr);
                    rf.write(r_d, val);
                    if (trace) cout << "  lw -> x" << r_d << " = 0x" << hex << setw(8) << val << dec;
                } else {
                    goto illegal;
                }
                break;
            }
            case 0x23: { // Stores
                // S-type
                int32_t imm = imm_s(insn);
                uint32_t addr = (uint32_t)((int32_t)R1 + imm);

                // Warn on unaligned access
                if (warn_unaligned && (addr & 3)) cerr << "[WARN] Unaligned SW at 0x" << hex << addr << dec << "\n";
                
                
                // Store based on funct3
                if (f3 == 0x2) { // SW
                    dmem.store_u32(addr, R2);
                    if (trace) cout << "  sw mem[0x" << hex << addr << "] = 0x" << setw(8) << R2 << dec;
                } else {
                    goto illegal;
                }
                break;
            }
            case 0x63: { // Branches

                // B-type
                int32_t off = imm_b(insn);
                bool take = false;

                // beq, bne only for subset
                if (f3 == 0x0) { // beq
                    take = (R1 == R2);
                } else if (f3 == 0x1) { // bne
                    take = (R1 != R2);
                } else {
                    goto illegal;
                }
                // Branch taken?
                if (take) pc_next = (uint32_t)((int32_t)PC + off);

                // Trace printout
                if (trace) cout << (take ? "  branch TAKEN" : "  branch not taken");
                break;
            }
            case 0x6F: { // JAL

                // J-type
                int32_t off = imm_j(insn);
                uint32_t ret = PC + 4;

                // Write return address
                rf.write(r_d, ret);
                uint32_t newPC = (uint32_t)((int32_t)PC + off);

                // HALT detection: jal x0, 0
                if (r_d == 0 && off == 0) {
                    // Convention: jal x0, 0 => HALT
                    if (trace) cout << "  HALT";
                    PC = pc_next; // or PC stays? We'll stop after this step anyway
                    if (trace) cout << "\n";
                    return false;
                }
                pc_next = newPC;

                // Trace printout
                if (trace) cout << "  jal -> x" << r_d << "=0x" << hex << setw(8) << ret
                                 << " PC=0x" << setw(8) << pc_next << dec;
                break;
            }
            case 0x67: { // JALR

                // I-type
                int32_t off = imm_i(insn);
                uint32_t ret = PC + 4;
                uint32_t target = (uint32_t)((int32_t)R1 + off);        // target address
                target &= ~1u; // spec: clear LSB

                // Write return address
                rf.write(r_d, ret);
                pc_next = target;

                // Trace printout   
                if (trace) cout << "  jalr -> x" << r_d << "=0x" << hex << setw(8) << ret
                                 << " PC=0x" << setw(8) << pc_next << dec;
                break;
            }
            case 0x37: { // LUI

                // U-type
                int32_t imm = imm_u(insn);
                rf.write(r_d, (uint32_t)imm);

                // Trace printout
                if (trace) cout << "  lui -> x" << r_d << " = 0x" << hex << setw(8) << (uint32_t)imm << dec;
                break;
            }
            case 0x17: { // AUIPC

                // U-type
                int32_t imm = imm_u(insn);
                uint32_t res = PC + (uint32_t)imm;
                rf.write(r_d, res);     // write result

                // Trace printout
                if (trace) cout << "  auipc -> x" << r_d << " = 0x" << hex << setw(8) << res << dec;
                break;
            }
            default:
                goto illegal;
        }

        // Finish trace line
        if (trace) cout << "\n";
        PC = pc_next;
        return true;

        // =================== Illegal instruction handler ===================
    illegal:
        cerr << "[ERROR] Illegal or unsupported instruction at PC=0x" << hex << PC
             << ", INSN=0x" << setw(8) << insn << dec << "\n";
        // For a student project, we can stop on illegal insn to avoid infinite loops.
        return false;
    }

    // =================== Run loop ===================
    void run(uint64_t max_steps = 5'000'000) {
        uint64_t steps = 0;
        while (steps < max_steps) {
            bool cont = step();
            steps++;
            if (!cont) break;
        }
        if (steps >= max_steps) cerr << "[WARN] Max steps reached; stopping to avoid hang.\n";
    }
};

// ============================== Utility: dump memory window ==============================
// Dumps 'words' 32-bit words starting from 'addr' in memory 'm' to output stream 'os'.
static void dump_mem_words(const Mem &m, uint32_t addr, size_t words, ostream &os) {
    os << hex << setfill('0');
    for (size_t i = 0; i < words; i++) {
        uint32_t a = addr + (uint32_t)(i*4);
        if (!m.in_range(a, 4)) break;
        uint32_t w = m.load_u32(a);
        os << "[0x" << setw(8) << a << "] = 0x" << setw(8) << w << "\n";
    }
    os << dec << setfill(' ');
}

// ============================== Main ==============================
   int main() {
    // Create CPU with 1MB instruction & data memory
    CPU cpu(1 << 20, 1 << 20);

    // Enable trace so you can see each instruction
    cpu.trace = true;

    // Load and run default program (must be in same folder)
    cpu.load_hex_program("test_base.hex");
    cpu.run();

    // Show final registers and memory
    cout << "\n==== FINAL REGISTER DUMP ====\n";
    cpu.rf.dump(cout);

    cout << "\n==== DATA MEM [0x00010000 .. 0x00010040) ====\n";
    dump_mem_words(cpu.dmem, 0x00010000u, 16, cout);

    return 0;
}
