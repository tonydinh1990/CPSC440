#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
using namespace std;

typedef vector<int> Bits; // Each element = 0 or 1 (MSB leftmost)

// ================================================================
//  SECTION 1: Helper Functions (Core Implementation - No arithmetic)
// ================================================================

// Make an n-bit vector filled with zeros
Bits zeros(int n) {
    Bits v(n);
    for (int i = 0; i < n; ++i)
        v[i] = 0;
    return v;
}

// -----------------------------
// Full Adder: 1-bit addition
// -----------------------------
void fullAdder(int a, int b, int cin, int &sum, int &cout) {
    int axb = a ^ b;
    sum = axb ^ cin;
    cout = (a & b) | (a & cin) | (b & cin);
}

// -----------------------------
// Ripple-Carry Adder (for ADD)
// -----------------------------
Bits addBits(const Bits &A, const Bits &B, int &carryOut) {
    int n = A.size();
    Bits result(n);
    int carry = 0;
    for (int i = n - 1; i >= 0; --i) {
        int s, c;
        fullAdder(A[i], B[i], carry, s, c);
        result[i] = s;
        carry = c;
    }
    carryOut = carry;
    return result;
}

// -----------------------------
// Negate (Two’s Complement): ~A + 1
// -----------------------------
Bits negateTwos(const Bits &A) {
    int n = A.size();
    Bits inv = zeros(n);
    for (int i = 0; i < n; ++i)
        inv[i] = (A[i] == 1) ? 0 : 1; // bitwise NOT

    Bits one = zeros(n);
    one[n - 1] = 1;
    int carryOut = 0;
    return addBits(inv, one, carryOut);
}

// -----------------------------
// ALU (ADD / SUB) with Flags
// -----------------------------
struct ALUFlags { int N, Z, C, V; };
struct ALUResult { Bits result; ALUFlags flags; };

ALUResult ALU(const Bits &a, const Bits &b, bool subtract) {
    Bits opB = subtract ? negateTwos(b) : b;
    int carryOut = 0;
    Bits sum = addBits(a, opB, carryOut);

    int sa = a[0], sb = b[0], sr = sum[0];

    int overflow;
    if (!subtract)
        overflow = (sa == sb) && (sr != sa);
    else
        overflow = (sa != sb) && (sr != sa);

    int zero = 1;
    for (int bit : sum)
        if (bit) zero = 0;

    ALUFlags f = { sr, zero, carryOut, overflow };
    return { sum, f };
}

// ================================================================
//  SECTION 2: Helper Functions (Testing/Display - Allowed arithmetic)
// ================================================================

// Convert integer to Bits (for test input)
Bits intToBits(int value, int width = 32) {
    Bits bits(width);
    unsigned int mask = 1u << (width - 1);
    for (int i = 0; i < width; ++i) {
        bits[i] = (value & mask) ? 1 : 0;
        mask >>= 1;
    }
    return bits;
}

// Convert Bits → integer (for decoded output)
long long bitsToInt(const Bits &b) {
    int n = b.size();
    uint64_t value = 0;
    for (int i = 0; i < n; ++i)
        value = (value << 1) | (uint64_t)b[i];
    // If sign bit set, subtract 2^n to get the negative value in two's complement
    if (b[0] == 1) {
        value -= (1ULL << n);
    }
    return (long long)value;
}

// Convert Bits → grouped binary string
string bitsToBinStr(const Bits &b) {
    string s = "";
    for (int i = 0; i < (int)b.size(); ++i) {
        s += (b[i] ? '1' : '0');
        if (i == 7 || i == 15 || i == 23) s += "_";
    }
    return s;
}

// Convert Bits → manual hex
string bitsToHex(const Bits &b) {
    string table = "0123456789ABCDEF";
    string hex = "0x";
    for (int i = 0; i < 8; ++i) {
        int nibble = 0;
        for (int j = 0; j < 4; ++j)
            nibble = (nibble << 1) | b[i * 4 + j];
        hex += table[nibble];
    }
    return hex;
}

// Print operation trace
void printTrace(const string &op, const Bits &a, const Bits &b, const ALUResult &r) {

    if (op == "ADD") {
       cout << "\nBinary: A + B = " << bitsToBinStr(a) << " + " << bitsToBinStr(b) << " = " << bitsToBinStr(r.result) << "\n";
       cout << "Hex: A + B = " << bitsToHex(a) << " + " << bitsToHex(b) << " = " << bitsToHex(r.result) << "\n";
    } else{
        cout << "\nBinary: A - B = " << bitsToBinStr(a) << " - " << bitsToBinStr(b) << " = " << bitsToBinStr(r.result) << "\n";
        cout << "Hex: A - B = " << bitsToHex(a) << " - " << bitsToHex(b) << " = " << bitsToHex(r.result) << "\n";
    }
        cout << "Flags: N=" << r.flags.N << " Z=" << r.flags.Z
         << " C=" << r.flags.C << " V=" << r.flags.V << "\n";

}
// ================================================================
//  SECTION 3: Main Simulation (User Interaction)
// ================================================================
int main() {
    cout << "===== Numeric Operations Simulator (RV32I ADD/SUB) =====\n";

    int num1 = -1, num2 = -1;

    Bits a = intToBits(num1);
    Bits b = intToBits(num2);

    cout << "\n--- Input Encodings ---\n";
    cout << num1 << " → binary: " << bitsToBinStr(a) << " Hex: " << bitsToHex(a) << "\n";
    cout << num2 << " → binary: " << bitsToBinStr(b) << " Hex: " << bitsToHex(b) << "\n";

    // Perform ADD
    auto add = ALU(a, b, false);
    cout << "\nAddition Operation: "<< num1 << " + " << num2;           // Display reference
    printTrace("ADD", a, b, add);
    cout << "Decoded result = " << bitsToInt(add.result) << "\n";

    // Perform SUB
    auto sub = ALU(a, b, true);
    cout << "\nSubtraction Operation: "<< num1 << " - " << num2;        // Display reference
    printTrace("SUB", a, b, sub);
    cout << "Decoded result = " << bitsToInt(sub.result) << "\n";


    

    
    return 0;
}
