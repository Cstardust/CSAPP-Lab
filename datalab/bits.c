/* 
 * CS:APP Data Lab 
 * 
 * Shc 200110732
 * <Please put your name and userid here>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting if the shift amount
     is less than 0 or greater than 31.


EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implement floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants. You can use any arithmetic,
logical, or comparison operations on int or unsigned data.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operations (integer, logical,
     or comparison) that you are allowed to use for your implementation
     of the function.  The max operator count is checked by dlc.
     Note that assignment ('=') is not counted; you may use as many of
     these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif

/*
bits.c：你实际操作的文件，里面有着大量的注释，需要你自己去阅读。每道题上的注释就是出题者对你的要求，要求包括你需要修改这个函数以便完成的目标、你在这个函数中所能使用的操作符种类、能使用的操作符的数目等。
btest.c：编写好了的用于测试你写的函数是否正确的代码文件，编译它可以获得一个可运行的二进制文件，运行它可以跑一些测试用例，来让你对自己的代码进行测试。
dlc：一个可运行的二进制文件，是魔改过的gcc编译器，用来检测你的代码风格。是的，在这个lab中，出题人对你的代码风格进行了限制，他要求你将声明全部放在表达式之前，否则会判错。
driver.pl：一个可运行的文件，用来进行最终的分数判定
fshow.c&ishow.c：两个工具，编译后可以得到两个可运行的二进制文件，分别可以输出你输入的float数和integer数的内存表示形式，比如标志位是什么，阶码是多少等

阅读bits.c的注释与代码
修改它
命令行运行./dlc -e bits.c查看自己用了多少操作符，以及是否有代码风格问题
运行make clean && make btest编译文件
运行./btest检查自己是否做对了
return 1 直到全部做完
最终运行./driver.pl获得打分
*/

//1
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1  
 *   Legal ops: ~ &             允许符号
 *   Max ops: 14                符号最大数量
 *   Rating: 1                  等级（难度）
 * 解:
 *  x^y：计算x和y中不同的位：不同时为1，不同时为0
 *  t1 = x&y：x和y同时为1的位(t)为0
 *  t2 = ~x&~y：x和y同时为0的位(t)为0
 * ~t1：x和y不同时为1的位(t)为1
 * ~t2：x和y不同时为0的位(t)为1
 * ~t1 & ~t2：x和y中既不同时为1的位，也不同时为0的 位 为1
 */

int bitXor(int x, int y) {
  int t1 = x&y;
  int t2 = (~x)&(~y);
  return (~t1) & (~t2);
}
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
  return 1<<31;
}
//2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 * 
 * 思路：将x转化成0值，再取反；并且要注意排除非Tmax情况，如-1 0xffffffff。
 */
int isTmax(int x) {
  int t = x+1;      //  Tmax->Tmin 10000...000
  x = ~(x+t);       //  x <-- 0
  return !(x+!t) ;  //  +!t是为了排除初始x为-1的情况
}
/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   where bits are numbered from 0 (least significant) to 31 (most significant)
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {
  int m = 0xaa;
  m = m<<8 | 0xaa; //  0xaa
  m = m<<8 | 0xaa; //  0xaaaa
  m = m<<8 | 0xaa; //  0xaaaaaa
  m = m<<8 | 0xaa; //  0xaaaaaaaa
  return !((x&m)^m);    //  检验x中是否包含m。 x&m 保留相同位 ^m 看是否完全相同
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
  return ~x+1;
}
//3
/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
//  符号位相同(a-b不溢出的情况下)，判断a>=b？,通过做差，然后在通过符号位判断： a-b>=0 -> a+(~b+1)>=0 -> a+(~b+1)符号位为0
//  符号为不同，a<0,b>0 则a<b；a>0,b<0，则a>b
//  符号相同(不溢出的情况下)，判断a<b？，不用<符号：a-b<0 -> a+(~b+1)<0 -> a+(~b+1)符号位为1
//  判断x是否[0x30,0x39]
int isAsciiDigit(int x) {
  int t1 = x+(~0x30+1);     //  x>=0x30
  int t2 = 0x39 + (~x+1);   //  x<=0x39
  return (!(t1>>31)) & (!(t2>>31));
}
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 * 如何求x的bool值？：!!x;
 *    x=0,则!!x=0
 *    x!=0,则!!x=1
 */
int conditional(int x, int y, int z) {
  x = !!x;
  x = ~x+1;   //  0->0：00...00->00...00  ； 1->-1：00...001 -> 111...1111
  return (x&y) | ((~x)&z);
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 * 符号位相同，通过做差，然后在通过符号位判断;  !((y+(~x)+1)>>31) 
 * 符号位不相同，可以直接判断x的符号位      
 */
int isLessOrEqual(int x,int y) {
  //  获取符号位时 一定要移位再&1，不然会有符号位扩展的造成的问题。
  int sub_sign = (y+(~x+1))>>31&1;  //  y-x
  // sub_sign = (sub_sign>>31)&1;  //  获取y-x最高符号位，并将其放到0位
  int x_sign = (x>>31)&1;     //  获取x最高符号位，并将其放到0位
  int y_sign = (y>>31)&1;     //  获取y最高符号位，并将其放到0位
  int sign_xor = x_sign ^ y_sign; //  x和y符号位是否相同 相同为0，不同为1。
  return ((!sign_xor)&(!sub_sign)) | (sign_xor&x_sign) ;
  /*
  * x和y符号位相同，sign_xor=0：若此时y-x>=0,即sub_sign=0x0。可以推出x<=y。返回1
  * x和y符号位不同，sign_xor=1; 若此时x为负数，即x_sign=0x1。可推出x<=y。返回1
  */  
 return 0;
}
//4
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 * 
 *   利用性质：
 *    -x = ~x+1;
 *      x==0时，-x=x=0。
 *      x==Tmin时,-x=x
 *      其余时,x!=-x
 *  
 */
int logicalNeg(int x) {
  int s = x | (~x+1); 
  return ((s>>31) + 1); 
  // 算术>> 时会发生符号扩展。
  // x>0||x<0，则s最高位必为1,>>31之后变成111...1111； +1变成000000...000
  // x==0,s最高位=0。>>31之后变成00000...000   	;   +1变成1
}

/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 * 对于正数，需要找到最高位的1，并在其左侧填上一个0作为符号位
 * 对于负数，需要找到最高位的0，并在其左侧填上一个1作为符号位
 * 如何求一个数对应的bool植: !!x
 * 有点二分查找内味儿，但也不全是。核心思想就是逐步抛弃右侧，缩小范围（缩小最高位1所在的范围）。
 */
int howManyBits(int x) {
  int b16,b8,b4,b2,b1,b0;
  int sign = x>>31;           //  >=0 则sign=0x0。 <=0 则sign=0xffffffff
  x = (sign&~x)|(~sign&x);    //  x>=0,x=x不变；x<0,则x=~x
  b16 = (!(!(x>>16)))<<4; //  如果高16位有1，那么b16=16；否则b16=0
  x = x>>b16;                 //  抛弃右16位，只看答案存在于的范围位
  b8 = (!(!(x>>8)))<<3;       //  如果x的[31,8]有1的话，那么右翼抛弃[7,0]位，只看[31,8]位。并记录下抛弃的位数8。若没有1的话，则不右移
  x = x>>b8;
  b4 = (!(!(x>>4)))<<2;
  x = x>>b4;
  b2 = (!(!(x>>2)))<<1;
  x = x>>b2;
  b1 = !(!(x>>1));
  x = x>>b1;
  b0 = x;
  //  b16 + b8 ... + b0 是指，从最高的1位，到第0位的位数
  return b16 + b8 + b4 + b2 + b1 + b0 + 1;    //  最后+的这个1是为我们要添加的符号位准备的
}

//float
/* 
 *   floatScale2 - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 * NUM:
 *   s exp frac
 *   arg:
 *    NAN：exp=11..11 ; frac!=0
 *    无穷：s=0/1 ;exp=111..111;frac=0
 *    0:exp=0 ; frac=0  
 *   区分非规格桦数，规格化数字，特殊值。
 */
unsigned floatScale2(unsigned uf) { 
  int s = (uf>>31)&0x1;             //  s
  int exp = (uf&0x7f800000)>>23;    //  exp阶码
  int frac = uf&0x007fffff;         //  frac
  if(exp==255)                      //  NAN / 无穷大
    return uf;           
  if(exp==0)                       //  由于非规格化：E = 1-bias; 规格化：E=e-bias。故可以平滑的从非规格化数到规格化数：故直接左移即可。
    return (s<<31) | (uf<<1);    
  if(++exp==255)                    // exp++即代表*2，虽然我不清楚那个bias怎么就不算了，可能忽略了？。。 *2 变成无穷大。不能用NAN表示，因为NAN表的是非实数
  {
    return exp<<23 | s<<31 ;    //  0x7t800000 | s<<31
  }
  return s<<31 | exp<<23 | frac;
}
/* 
 * floatFloat2Int - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 * float -> int
 * |float| = 0  -> int =0 
 * |float| < 1  -> int = 0;
 * E = e-bias
 * E >= 31  -> 超出int(因为：float : 2^E * M(M=1+f))
 * 0 < E < 31 移动frac
 * 0 < E < 23 舍弃frac部分位
 * 23 <= E < 31 不舍其frac部分位 
 */
int floatFloat2Int(unsigned uf) {
  int E,M;
  int bias,s,exp,frac;
  bias = 127;
  s = (uf>>31)&0x1;
  exp = (uf&0x7f800000)>>23;
  E = exp-bias;         //  计算E
  frac = uf&0x007fffff;
  M = frac + (1<<23);   //  计算M：加上隐含的1
  //  使用E，M
  if(E<0)
  {
    return 0;
  }
  else if(E>=31)
  {
    return 0x80000000u; 
  }
  else if(E>=0&&E<23)
  {
    M >>= (23-E);
  }
  else if(E>23)
  {
    M <<= (E-23);
  }
  if(s) return -M;  //  符号 -M = ~M + 1。可不是简单的在符号位上加个1阿
  else return M;
}

/* 
 * floatPower2 - Return bit-level equivalent of the expression 2.0^x
 *   (2.0 raised to the power x) for any 32-bit integer x.
 *
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 * 
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while 
 *   Max ops: 30 
 *   Rating: 4
 * 
      * 情况一，都是0：

      2^-150的二进制表示为0|00000000|00000000000000000000000

      情况二，阶码0:
      -------------------------------------------------------------------
      2^-149的二进制表示为0|00000000|00000000000000000000001
      2^-148的二进制表示为0|00000000|00000000000000000000010
      2^-147的二进制表示为0|00000000|00000000000000000000100
      ……
      2^-129的二进制表示为0|00000000|00100000000000000000000
      2^-128的二进制表示为0|00000000|01000000000000000000000
      2^-127的二进制表示为0|00000000|10000000000000000000000

      情况三，阶码非0：
      -------------------------------------------------------------------
      2^-126的二进制表示为0|00000001|00000000000000000000000
      2^-125的二进制表示为0|00000010|00000000000000000000000
      2^-124的二进制表示为0|00000011|00000000000000000000000
      ……
      2^0 的二进制表示为0|01111111|00000000000000000000000
      2^1 的二进制表示为0|10000000|00000000000000000000000
      ……
      2^127 的二进制表示为0|11111110|00000000000000000000000

      情况四，无穷大：
      -----------------------------------------------------------------
      2^128 的二进制表示为0|11111111|00000000000000000000000
      2^129 的二进制表示为0|11111111|00000000000000000000000
 *  非规格化：
 *    E = 1-bias = 1-127 = -126
 *    frac = 2^-24 ...  2^-1
 *  V = 2^E * frac
 * 故非规格化时，2的指数范围 为 2^-150 --- 2^-127
 *  规格化：
 *    E = e-bias = e-127 
 * 2指数范围：-126-127
 */
//  用单精度浮点数表示2.0^x
unsigned floatPower2(int x) {
    //  如果不能非规格化，就return 0
    if(x<=-127){
      return 0;
    }else if(x>=-126&&x<=127){
      return (x+127)<<23; //  frac位均为0 E = e-bias。故e = E+bias
    }else if(x>127){
      return 0x7f800000u;   //  即+INF 即0xff<<23 8+23=31位 再加上最高位的符号位0 共32位。
    }    
    return 0;
}
