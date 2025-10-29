#include <iostream>
#include <vector>
#include <string>
using namespace std;

// ============================= Types =============================
typedef vector<int> Bits; // MSB at index 0, LSB at index width-1

// ============================= Section 1: Core Implementation (NO built-in + - * / % << >> on numeric types) =============================

// ---- basic constructors ----
Bits zeros(int n){ 
    Bits v(n); 
    for(int i=0;i<n;++i) 
    v[i]=0; 
    return v; 
}

// ---- 1-bit full adder ----
void fullAdder(int a,int b,int cin,int &sum,int &cout){
    int axb = a ^ b;
    sum = axb ^ cin;
    cout = (a & b) | (a & cin) | (b & cin);
}

// ---- ripple-carry add (unsigned) ----
Bits addBits(const Bits&A,const Bits&B,int &carryOut){
    int n=(int)A.size(); Bits R(n); int c=0; 
    for(int i=n-1;i>=0;--i){ 
        int s,k; 
        fullAdder(A[i],B[i],c,s,k); 
        R[i]=s; c=k; 
    }
    carryOut=c; 
    return R;
}

// ---- two's complement negate ----
Bits negateTwos(const Bits&A) {

     int n=(int)A.size(); 
     Bits inv=zeros(n); 
     
     for(int i=0;i<n;++i)
      inv[i]=A[i]?0:1; 
      
      Bits one=zeros(n); 
      one[n-1]=1; 
      int c=0; 
      
      return addBits(inv,one,c); 
    }

// ---- shifters (no << >>) ----
Bits shiftLeft1(const Bits&x){ 

    int n=(int)x.size(); 
    Bits y=zeros(n);

    for(int i=0;i<n-1;++i) 
    y[i]=x[i+1];

    y[n-1]=0; 
    
    return y;
    }

    
Bits shiftRight1Logical(const Bits&x){ 

    int n=(int)x.size(); 
    Bits y=zeros(n); 

    for(int i=n-1;i>=1;--i) 
    y[i]=x[i-1]; 
    
    y[0]=0; 
    return y; 
}

Bits shiftRight1Arithmetic(const Bits&x){

     int n=(int)x.size(); 
     Bits y=zeros(n); 

     for(int i=n-1;i>=1;--i) 
     y[i]=x[i-1];

     y[0]=x[0]; 
     return y;
    }

// ---- utils ----
int signBit(const Bits&x){ return x[0]; }
int isZeroBits(const Bits&x){ for(int b:x) if(b) return 0; return 1; }
Bits absSigned(const Bits&x){ return signBit(x)?negateTwos(x):x; }

// ---- extend/compare ----
Bits zeroExtend(const Bits&x,int newW){ 
    int w=(int)x.size(); 
    Bits y(newW); 
    int off=newW-w;

    for(int i=0;i<off;++i) 
    y[i]=0;

    for(int i=0;i<w;++i) 
    y[off+i]=x[i]; 
    
    return y; 
}

Bits signExtendTo(const Bits&x,int newW){ 

    int w=(int)x.size(); 
    Bits y(newW); 
    int off=newW-w; 
    int s=x[0];

    for(int i=0;i<off;++i) 
    y[i]=s; 
    
    for(int i=0;i<w;++i)
     y[off+i]=x[i]; 
     
     return y; 
    }

int uCmp(const Bits&A,const Bits&B){ 
    int n=(int)A.size(); 

    for(int i=0;i<n;++i){ 
        if(A[i]!=B[i])
        return (A[i]<B[i])?-1:1; 
    } 
         
    return 0; }

// ---------------- ALU (ADD/SUB) with flags ----------------
struct ALUFlags{ int N,Z,C,V; };
struct ALUResult{ Bits result; ALUFlags flags; };

ALUResult ALU(const Bits &a,const Bits &b,bool subtract){
    Bits opB = subtract ? negateTwos(b) : b;
    int carryOut=0; Bits sum = addBits(a,opB,carryOut);
    int sa=a[0], sb=b[0], sr=sum[0];
    int overflow = (!subtract)? ((sa==sb) && (sr!=sa)) : ((sa!=sb) && (sr!=sa));
    int zero=1; for(int bit:sum) if(bit){ zero=0; break; }
    ALUFlags f{ sr, zero, carryOut, overflow };
    return { sum, f };
}

// ---------------- MUL family (shift-add) ----------------
struct MulOut{ Bits low32; Bits high32; int overflow; };

// Unsigned 32x32 -> 64 via classic shift-add
Bits mulUnsigned32x32(const Bits& ua, const Bits& ub, bool trace){
    Bits acc = zeros(64);                // 64-bit accumulator/product
    Bits multiplicand = zeroExtend(ua,64);
    Bits multiplier   = ub;              // 32-bit

    for(int step=0; step<32; ++step){
        int lsb = multiplier[31];
        if(lsb){ int c=0; acc = addBits(acc, multiplicand, c); /* carry beyond 64 ignored */ }
        if(trace){ /* tracing prints are in helpers section; here we avoid helpers */ }
        multiplicand = shiftLeft1(multiplicand);
        multiplier   = shiftRight1Logical(multiplier);
    }
    return acc; // MSB..LSB (64 bits)
}

// Overflow visibility: true 64-bit product fits in signed 32-bit?
int mulOverflowFlag(const Bits& prod64){
    Bits low32(32); for(int i=0;i<32;++i) low32[i]=prod64[32+i];
    Bits se64 = signExtendTo(low32,64);
    int eq=1; for(int i=0;i<64;++i) if(se64[i]!=prod64[i]){ eq=0; break; }
    return eq?0:1;
}

MulOut mul_ss(const Bits& a, const Bits& b, bool trace){
    Bits aa=absSigned(a), bb=absSigned(b);
    int neg = signBit(a)^signBit(b);
    Bits prod = mulUnsigned32x32(aa,bb,trace);
    if(neg && !isZeroBits(prod)) prod = negateTwos(prod);
    Bits hi(32), lo(32); for(int i=0;i<32;++i){ hi[i]=prod[i]; lo[i]=prod[32+i]; }
    int of = mulOverflowFlag(prod);
    return { lo, hi, of };
}

MulOut mul_uu(const Bits& a, const Bits& b, bool trace){
    Bits prod = mulUnsigned32x32(a,b,trace); Bits hi(32), lo(32);
    for(int i=0;i<32;++i){ hi[i]=prod[i]; lo[i]=prod[32+i]; }
    return { lo, hi, 0 };
}

MulOut mul_su(const Bits& a, const Bits& b, bool trace){
    Bits aa=absSigned(a); Bits prod=mulUnsigned32x32(aa,b,trace);
    if(signBit(a) && !isZeroBits(prod)) prod=negateTwos(prod);
    Bits hi(32), lo(32); for(int i=0;i<32;++i){ hi[i]=prod[i]; lo[i]=prod[32+i]; }
    return { lo, hi, 0 };
}

// ---------------- DIV/REM family (restoring division) ----------------
struct DivOut{ Bits q; Bits r; int overflow; };
struct DivPair{ Bits q; Bits r; int overflow; };

// A - B: returns R and noBorrow flag (1 => no borrow => A>=B)
Bits uSub(const Bits& A, const Bits& B, int& noBorrow){ Bits Bn=negateTwos(B); Bits R=addBits(A,Bn,noBorrow); return R; }

DivOut divu_restoring(const Bits& dividend, const Bits& divisor, bool trace){
    if(isZeroBits(divisor)){
        Bits q=zeros(32); for(int i=0;i<32;++i) q[i]=1; // 0xFFFFFFFF
        return { q, dividend, 0 };
    }
    Bits R=zeros(32), Q=zeros(32);
    for(int i=0;i<32;++i){
        // shift-in next dividend bit (MSB-first)
        int bit_in = dividend[i];
        R = shiftLeft1(R); R[31]=bit_in;
        int noBorrow=0; Bits RminusD = uSub(R, divisor, noBorrow);
        int qbit = noBorrow ? 1 : 0; // if R>=divisor then set qbit and keep subtraction
        if(qbit) R = RminusD; // else restore (do nothing)
        Q = shiftLeft1(Q); Q[31]=qbit;
        // tracing avoided here (uses helpers in display section)
    }
    return { Q, R, 0 }; // overflow not used for unsigned
}

DivPair div_signed(const Bits& A, const Bits& B, bool trace){
    if(isZeroBits(B)){
        Bits all1=zeros(32); for(int i=0;i<32;++i) all1[i]=1; // -1
        return { all1, A, 0 };
    }
    // INT_MIN / -1 edge
    Bits intMinBits = zeros(32); intMinBits[0]=1; for(int i=1;i<32;++i) intMinBits[i]=0;
    Bits NEG1=zeros(32); for(int i=0;i<32;++i) NEG1[i]=1;
    int AisIntMin=1; for(int i=0;i<32;++i) if(A[i]!=intMinBits[i]){ AisIntMin=0; break; }
    int BisNeg1=1;  for(int i=0;i<32;++i) if(B[i]!=NEG1[i]){  BisNeg1=0;  break; }
    if(AisIntMin && BisNeg1){ Bits q=intMinBits; Bits r=zeros(32); return { q, r, 1 }; }

    int sA=signBit(A), sB=signBit(B);
    Bits ua=absSigned(A), ub=absSigned(B);
    DivOut d = divu_restoring(ua, ub, trace);
    Bits q=d.q, r=d.r;
    if(sA ^ sB) q = negateTwos(q);   // quotient truncates toward zero
    if(sA)       r = negateTwos(r);  // remainder sign follows dividend
    return { q, r, 0 };
}

// ============================= Section 2: Test/Display Helpers (OK to use shifts & host ints) =============================

// dynamic hex printer for any width multiple of 4
string bitsToHexN(const Bits& b){
    static const string tab="0123456789ABCDEF";
    int n=(int)b.size(); int nibbles = (n+3)/4; int pad = nibbles*4 - n;
    // build a padded copy
    Bits tmp = zeros(nibbles*4); for(int i=0;i<n;++i) tmp[pad+i]=b[i];
    string h="0x"; 
    for(int i=0;i<nibbles;++i){ int v=0; for(int j=0;j<4;++j) v=(v<<1)|tmp[i*4+j]; h += tab[v]; }
    return h;
}

// 32-bit specific wrappers
string bitsToHex32(const Bits& b){ return bitsToHexN(b); }
string bitsToHex64(const Bits& b){ return bitsToHexN(b); }

// grouped binary (underscore every 8 bits)
string bitsToBinStr(const Bits& b){ string s=""; for(int i=0;i<(int)b.size();++i){ s += (b[i]?'1':'0'); if((i%8)==7 && i!=(int)b.size()-1) s+="_"; } return s; }

// int<->bits (TEST ONLY)
Bits intToBits(int value,int width=32){ 
    Bits bits(width); 
    unsigned int mask = 1u<<(width-1); 
    for(int i=0;i<width;++i){ 
        bits[i] = (value & mask) ? 1 : 0; 
        mask >>= 1; 
    } 
    return bits; 
}
long long bitsToInt(const Bits& b){ 
    int n=(int)b.size(); 
    unsigned long long v=0; 
    for(int i=0;i<n;++i){ 
        v=(v<<1)| (unsigned long long)b[i]; 
    } if(b[0]) v -= (1ULL<<n);
    return (long long)v;
}

// ADD/SUB pretty trace
void printALUTrace(const string& op,const Bits&a,const Bits&b,const Bits&r,const ALUFlags&f){
    if(op=="ADD"){
        cout << "\nBinary: A + B = " << bitsToBinStr(a) << " + " << bitsToBinStr(b) << " = " << bitsToBinStr(r) << "\n";
        cout << "Hex:    A + B = " << bitsToHex32(a) << " + " << bitsToHex32(b) << " = " << bitsToHex32(r) << "\n";
    }else{
        cout << "\nBinary: A - B = " << bitsToBinStr(a) << " - " << bitsToBinStr(b) << " = " << bitsToBinStr(r) << "\n";
        cout << "Hex:    A - B = " << bitsToHex32(a) << " - " << bitsToHex32(b) << " = " << bitsToHex32(r) << "\n";
    }
    cout << "Flags: N="<<f.N<<" Z="<<f.Z<<" C="<<f.C<<" V="<<f.V<<"\n";
}

// MUL/DIV pretty outputs
void printMulResult(const Bits&a,const Bits&b,const MulOut& mo,const string& tag){
    cout << "\n"<<tag<<": "<<bitsToHex32(a)<<" * "<<bitsToHex32(b)
         << " -> low="<<bitsToHex32(mo.low32)
         << " high="<<bitsToHex32(mo.high32)
         << " overflow="<<mo.overflow <<"\n";
}

void printDivResultSigned(const Bits&a,const Bits&b,const DivPair& d){
    long long qVal = bitsToInt(d.q);
    long long rVal = bitsToInt(d.r);
    cout << "\nDIV " << bitsToInt(a) << " / " << bitsToInt(b)
         << " -> q = " << qVal << " (" << bitsToHex32(d.q) << ")"
         << "; r = " << rVal << " (" << bitsToHex32(d.r) << ")"
         << "; overflow=" << d.overflow << "\n";
}

void printDivResultUnsigned(const Bits&a,const Bits&b,const DivOut& d){
    unsigned long long qVal = bitsToInt(d.q);
    unsigned long long rVal = bitsToInt(d.r);
    cout << "DIVU " << bitsToInt(a) << " / " << bitsToInt(b)
         << " -> q = " << qVal << " (" << bitsToHex32(d.q) << ")"
         << "; r = " << rVal << " (" << bitsToHex32(d.r) << ")"
         << "; overflow=0\n";
}


// ============================= Section 4: IEEE-754 Float32 Representation =============================
// Representation: 1 sign bit, 8 exponent bits (bias = 127), 23 fraction bits (implicit 1 for normals)
// All arithmetic on bits, no host float operations inside encoding logic
// -------------------------------------------------------------------------------------

struct Float32 {
    int sign;        // 0 = positive, 1 = negative
    Bits exponent;   // 8 bits, unsigned with bias 127
    Bits fraction;   // 23 bits (mantissa without the leading 1)
};

// ---- Decode a 32-bit float bit-vector into sign, exponent, and fraction ----
Float32 decodeFloat32(const Bits& bits) {
    Float32 f;
    f.sign = bits[0];
    f.exponent = Bits(bits.begin() + 1, bits.begin() + 9);       // bits[1..8]
    f.fraction = Bits(bits.begin() + 9, bits.end());             // bits[9..31]
    return f;
}

// ---- Encode a Float32 struct back into a 32-bit bit-vector ----
Bits encodeFloat32(const Float32& f) {
    Bits bits(32);
    bits[0] = f.sign;
    for (int i = 0; i < 8; ++i) bits[1 + i] = f.exponent[i];
    for (int i = 0; i < 23; ++i) bits[9 + i] = f.fraction[i];
    return bits;
}


// ============================= Float32 Addition/Subtraction =============================
// Perform IEEE-754 addition/subtraction using bit operations on sign/exponent/mantissa.
// Simplified algorithm:  no rounding modes yet (round-to-nearest only).

// Normalize helper: shift fraction left until MSB=1 (for normalized values)
struct NormResult { Bits frac; int expAdjust; };
NormResult normalizeFrac(const Bits& frac, int expBias) {
    Bits f = frac;
    int shiftCount = 0;
    while (f[0] == 0 && shiftCount < 24) {  // find leading 1
        f = shiftLeft1(f);
        shiftCount++;
    }
    return { f, -shiftCount };
}

// Align exponents by shifting the smaller mantissa right
void alignExponents(Bits& fracA, Bits& fracB, int& expA, int& expB) {
    while (expA < expB) { fracA = shiftRight1Logical(fracA); expA++; }
    while (expB < expA) { fracB = shiftRight1Logical(fracB); expB++; }
}

// ---- Float addition/subtraction main ----
Bits floatAddSub(const Bits& a, const Bits& b, bool subtract) {
    Float32 A = decodeFloat32(a);
    Float32 B = decodeFloat32(b);

    // Extract exponent & convert to integer (use bitsToInt for bias)
    int expA = (int)bitsToInt(signExtendTo(A.exponent, 32));
    int expB = (int)bitsToInt(signExtendTo(B.exponent, 32));
    expA -= 127; expB -= 127;  // remove bias

    // Build full 24-bit mantissas (implicit 1 for normals)
    Bits fracA = Bits(24); fracA[0] = 1;
    for (int i = 0; i < 23; ++i) fracA[i + 1] = A.fraction[i];

    Bits fracB = Bits(24); fracB[0] = 1;
    for (int i = 0; i < 23; ++i) fracB[i + 1] = B.fraction[i];

    // Align exponents
    alignExponents(fracA, fracB, expA, expB);
    int expRes = expA;

    // Perform addition or subtraction on mantissas
    Bits resFrac;
    int carry = 0;
    if (A.sign == B.sign ^ subtract) {
        // opposite signs => subtraction
        Bits negB = negateTwos(fracB);
        resFrac = addBits(fracA, negB, carry);
    } else {
        // same sign => addition
        resFrac = addBits(fracA, fracB, carry);
    }

    // Normalize result mantissa
    NormResult norm = normalizeFrac(resFrac, expRes);
    resFrac = norm.frac;
    expRes += norm.expAdjust;

    // Re-bias exponent
    int biasedExp = expRes + 127;
    Bits expBits = intToBits(biasedExp, 8);

    // Rebuild Float32
   Float32 R;
    R.sign = (A.sign ^ subtract) ? 1 : 0;
    R.exponent = expBits;
    R.fraction = Bits(23);
    for (int i = 0; i < 23; ++i)
    R.fraction[i] = resFrac[i + 1];
    return encodeFloat32(R);
}


// ============================= Float32 Multiplication =============================
// Performs IEEE-754 single-precision multiply using bit logic (shift-add multiply)
// Steps:
// 1. Decode operands into sign/exponent/mantissa
// 2. Compute result sign = XOR(signA, signB)
// 3. Multiply 24-bit mantissas (1.f * 1.f)
// 4. Add exponents and subtract bias (127)
// 5. Normalize and round mantissa
// 6. Re-encode into 32-bit Float32 bit vector

Bits floatMultiply(const Bits& a, const Bits& b) {
    Float32 A = decodeFloat32(a);
    Float32 B = decodeFloat32(b);

    // Step 1: Determine sign bit
    int resultSign = A.sign ^ B.sign;

    // Step 2: Convert exponents to integers and remove bias
    int c = 0;
    Bits expSum = addBits(A.exponent, B.exponent, c);        // expA + expB
    Bits negBias = negateTwos(bias127_8());                  // -127
    Bits expRes  = addBits(expSum, negBias, c);              // sum - 127

    // Step 3: Build 24-bit mantissas (implicit 1 + 23 fraction)
    Bits mA(24), mB(24); 
    mA[0]=1; 
    mB[0]=1;

    for(int i=0;i<23;++i){
         mA[i+1]=A.fraction[i];
        mB[i+1]=B.fraction[i]; 
    }

    // Step 4: Multiply mantissas using your integer bit multiplier
    Bits mA32 = zeroExtend(mA, 32);
    Bits mB32 = zeroExtend(mB, 32);
    Bits prod64 = mulUnsigned32x32(mA32, mB32, false);       // 64-bit MSB..LSB

    // Extract the lower 48 bits (since (2^24-1)^2 < 2^48) → positions [16..63]
    Bits prod48(48);
    for(int i=0;i<48;++i) 
    prod48[i] = prod64[16+i];

    // Normalize:
    // If prod48[0] == 1  → product in [2.0, 4.0), take mant = prod48[0..23], exp += 1
    // else                → product in [1.0, 2.0), take mant = prod48[1..24], exp stays
    Bits mant24(24);
    if (prod48[0]==1) {
        for(int i=0;i<24;++i) mant24[i] = prod48[i];         // take [0..23]
        expRes = addBits(expRes, one8(), c);                 // exp += 1
    } else {
        for(int i=0;i<24;++i) mant24[i] = prod48[i+1];       // take [1..24]
    }


    // Fraction = mant24[1..23]
    Bits frac(23);
    for(int i=0;i<23;++i) frac[i] = mant24[i+1];

    // Pack result
    Float32 R; R.sign = resultSign; R.exponent = expRes; R.fraction = frac;
    return encodeFloat32(R);
}




void printFloat32(const Bits& bits) {
    Float32 f = decodeFloat32(bits);
    cout << "Float32 bits: " << bitsToHex32(bits)
         << " (sign=" << f.sign
         << ", exp=" << bitsToHexN(f.exponent)
         << ", frac=" << bitsToHexN(f.fraction) << ")" << endl;
}




// ============================= Section 3: Main (demo / quick tests) =============================
int main(){
    cout << "===== Numeric Operations Simulator (RV32 ALU + M Extension) =====\n";

    // ---- Get two integers from user for ADD/SUB demo ----
/*     int num1, num2; cout << "Enter two integers for ADD/SUB: "; if(!(cin>>num1>>num2)){ cout<<"\nInput error.\n"; return 0; }
 */    
    int num1 = -15, num2 = -5;
    Bits A=intToBits(num1), B=intToBits(num2);

    cout << "\n--- Input Encodings ---\n";
    cout << num1 << " -> bin " << bitsToBinStr(A) << ", hex " << bitsToHex32(A) << "\n";
    cout << num2 << " -> bin " << bitsToBinStr(B) << ", hex " << bitsToHex32(B) << "\n";

    // ADD
    auto add = ALU(A,B,false);
    cout << "\nAddition: "<<num1<<" + "<<num2<<"\n";
    printALUTrace("ADD",A,B,add.result,add.flags);
    cout << "Decoded result = " << bitsToInt(add.result) << "\n";

    // SUB
    auto sub = ALU(A,B,true);
    cout << "\nSubtraction: "<<num1<<" - "<<num2<<"\n";
    printALUTrace("SUB",A,B,sub.result,sub.flags);
    cout << "Decoded result = " << bitsToInt(sub.result) << "\n";

    // ---- M Extension demos (match spec expectations) ----
    cout << "\n===== M Extension Demos =====\n";

    cout << "MULT: "<<num1<<" * "<<num2;
    Bits M1=intToBits(num1), M2=intToBits(num2);
    auto mss = mul_ss(M1,M2,true); // trace enabled internally (minimal)
    printMulResult(M1,M2,mss,"MUL(ss)");
    cout << "MUL low32 = "<<bitsToHex32(mss.low32)<<" overflow="<<mss.overflow<<"\n";
    cout << "MULH high32 = "<<bitsToHex32(mss.high32)<<"\n";

    // DIV -7 / 3

    cout << "\nDIV: "<<num1<<" / "<<num2;
    Bits D1=intToBits(num1), D2=intToBits(num2);
    auto ds = div_signed(D1,D2,true);
    printDivResultSigned(D1,D2,ds);

    // DIVU 0x80000000 / 3
    Bits UA=intToBits(num1), UB=intToBits(num2);
    auto du = divu_restoring(UA,UB,true);
    printDivResultUnsigned(UA,UB,du);



     
    cout << "\n===== IEEE-754 Float32 Decode Tests =*****====\n";

    // Example: +1.0 (0x3F800000), 25 = 0x41C80000
    Bits f1 = intToBits(-2.5);
    printFloat32(f1);

    // Example: -2.5 (0xC0200000)         
    Bits f2 = intToBits(0xC0200000);
    printFloat32(f2);


    cout << "\n===== IEEE-754 Float32 Add/Sub Tests =====\n";

    // Example: 1.5 (0x3FC00000) + 2.25 (0x40100000)
    Bits fA = intToBits(25);
    Bits fB = intToBits(2.25);

    cout << "\n--- Float XXXXxamples ---\n";
    // Print fA using the Float32 helper (vector<int> can't be streamed directly)
    printFloat32(fA);
    cout << "\n--- Float XXXXxamples ---\n";

    Bits fSum = floatAddSub(fA, fB, false);
    cout << "Adding 1.5 + 2.25:\n";         // expect 3.75 (0x40700000)
    printFloat32(fSum);

    // Example: 5.5 (0x40B00000) − 2.25 (0x40100000)
    Bits fC = intToBits(0x40B00000);
    Bits fD = intToBits(0x40100000);
    Bits fDiff = floatAddSub(fC, fD, true);
    cout << "Subtracting 5.5 - 2.25:\n";        // expect 3.25 (0x4050000)
    printFloat32(fDiff);


    cout << "\n===== IEEE-754 Float32 Multiply Tests =====\n";

// Example 1: 1.5 * 2.25 = 3.375  (0x3FC00000 * 0x40200000 → 0x40580000)
    Bits fMulA = intToBits(0x3FC00000);
    Bits fMulB = intToBits(0x40100000);
    Bits fMulR = floatMultiply(fMulA, fMulB);
    cout << "Multiplying 1.5 * 2.25:\n"; 
     // expect 3.375 (0x40580000)    
    printFloat32(fMulR);


    cout << "\nDone.\n";
    return 0;
}
