#include <iostream>
#include <string>
#include <vector>
#include <cmath>

// 自定义 gcd 函数（如果编译器不支持 std::gcd）
unsigned long gcd(unsigned long a, unsigned long b) {
    while (b != 0) {
        unsigned long t = b;
        b = a % b;
        a = t;
    }
    return a;
}

// 简单的模幂运算函数
unsigned long mod_exp(unsigned long base, unsigned long exp, unsigned long mod) {
    unsigned long result = 1;
    while (exp > 0) {
        if (exp & 1) // 如果exp是奇数
            result = (result * base) % mod; // 模乘
        exp >>= 1; // exp右移一位，相当于除以2
        base = (base * base) % mod; // 平方并取模
    }
    return result;
}

// 消息哈希函数，确保哈希结果在模数 n 的范围内
std::string hash_message(const std::string& message, unsigned long n) {
    unsigned long hash = 0;
    for (char c : message) {
        hash = (hash * 31 + c) % n; // 使用模数 n 进行简化处理
    }
    return std::to_string(hash);
}

// 找到模逆元 d，使得 (e * d) % phi == 1
unsigned long mod_inverse(unsigned long a, unsigned long m) {
    unsigned long m0 = m, t, q;
    unsigned long x0 = 0, x1 = 1;

    if (m == 1)
      return 0;

    while (a > 1) {
        q = a / m;
        t = m;
        m = a % m, a = t;
        t = x0;
        x0 = x1 - q * x0;
        x1 = t;
    }

    if (x1 < 0)
        x1 += m0;

    // 验证模逆元计算是否正确
    if ((x1 * a) % m != 1) {
        std::cerr << "Error: Invalid modular inverse!" << std::endl;
        exit(1);
    }

    return x1;
}

// 生成简单的 RSA 密钥对（仅供演示用途）
void generate_rsa_keys(unsigned long& e, unsigned long& d, unsigned long& n) {
    // 选择两个较大的素数 p 和 q
    unsigned long p = 61, q = 53;
    n = p * q;
    unsigned long phi = (p - 1) * (q - 1);

    // 选择公钥指数 e
    e = 17; // 通常选择较小的素数如 3, 5, 17, 257, 65537 等
    while (gcd(e, phi) != 1) {
        ++e; // 确保 e 和 phi 互质
    }

    // 计算私钥指数 d
    d = mod_inverse(e, phi);

    // 验证密钥对是否有效
    if ((e * d) % phi != 1) {
        std::cout << "Error: Invalid key pair!" << std::endl;
        exit(1);
    }
}

// 使用RSA私钥(d, n)对消息进行签名。
std::string sign_message(const std::string& message, unsigned long d, unsigned long n) {
    std::string hashed = hash_message(message, n); // 对消息进行哈希
    std::cout << "Alice 对消息进行哈希：" << hashed << std::endl;
    unsigned long h = std::stoul(hashed); // 将哈希值转换为整数
    unsigned long signature = mod_exp(h, d, n); // 使用私钥对哈希值进行模幂运算
    std::cout << "Alice 对消息的哈希进行加密：" << signature << std::endl;
    return std::to_string(signature); // 返回签名字符串
}

// 使用RSA公钥(e, n)验证签名的消息。
bool verify_signature(const std::string& message, const std::string& signature_str, unsigned long e, unsigned long n) {
    unsigned long signature = std::stoul(signature_str); // 将签名字符串转换为整数
    unsigned long decrypted_hash = mod_exp(signature, e, n); // 使用公钥解密签名
    std::cout << "Bob 对签名进行解密，得到哈希值：" << decrypted_hash << std::endl;
    std::string hashed = hash_message(message, n); // 对原始消息进行哈希
    unsigned long h = std::stoul(hashed); // 将哈希值转换为整数

    return decrypted_hash == h; // 检查解密后的哈希值是否与原始消息的哈希值匹配
}

int main() {
    unsigned long e, d, n;
    generate_rsa_keys(e, d, n); // 生成有效的 RSA 密钥对

    std::cout << "公钥 (e, n): (" << e << ", " << n << ")\n";
    std::cout << "私钥 (d, n): (" << d << ", " << n << ")\n";

    std::string message = "hello ahwei"; // 待签名的消息
    std::string signature = sign_message(message, d, n); // 生成签名

    std::cout << "原始消息: " << message << "\n";
    std::cout << "签名: " << signature << "\n";

    if (verify_signature(message, signature, e, n)) { // 验证签名
        std::cout << "签名验证成功。\n";
    } else {
        std::cout << "签名验证失败。\n";
    }

    return 0;
}