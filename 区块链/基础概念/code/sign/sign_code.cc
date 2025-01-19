#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <sstream>

// 错误处理函数：打印错误信息并终止程序
void handleErrors() {
    ERR_print_errors_fp(stderr);
    abort();
}

// 生成指定位数的RSA密钥对
EVP_PKEY* generate_rsa_key(int bits) {
    EVP_PKEY_CTX *ctx = NULL;
    EVP_PKEY *pkey = NULL;

    // 创建一个上下文对象用于生成密钥
    ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    if (!ctx)
        handleErrors();

    // 初始化密钥生成操作
    if (EVP_PKEY_keygen_init(ctx) <= 0)
        handleErrors();

    // 设置密钥长度
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, bits) <= 0)
        handleErrors();

    // 执行密钥生成
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0)
        handleErrors();

    // 释放上下文对象
    EVP_PKEY_CTX_free(ctx);
    return pkey;
}

// 使用私钥对消息进行签名
std::string sign_message(const std::string& message, EVP_PKEY* pkey) {
    unsigned char* signature = nullptr;
    size_t sig_len;

    // 创建一个新的摘要签名上下文对象
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) handleErrors();

    // 初始化签名操作，使用SHA-256作为摘要算法
    if (EVP_DigestSignInit(mdctx, NULL, EVP_sha256(), NULL, pkey) <= 0)
        handleErrors();

    // 更新要签名的消息数据
    if (EVP_DigestSignUpdate(mdctx, message.c_str(), message.size()) <= 0)
        handleErrors();

    // 获取签名的长度
    if (EVP_DigestSignFinal(mdctx, NULL, &sig_len) <= 0)
        handleErrors();

    // 分配内存来保存签名
    signature = (unsigned char*)OPENSSL_malloc(sig_len);
    if (!signature) handleErrors();

    // 实际执行签名操作
    if (EVP_DigestSignFinal(mdctx, signature, &sig_len) <= 0)
        handleErrors();

    // 打印出签名前的消息散列值（十六进制格式）
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, message.c_str(), message.size());
    SHA256_Final(hash, &sha256);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << (int)hash[i];
    }
    std::cout << "消息散列值: " << ss.str() << "\n";

    // 输出签名内容（十六进制格式）
    std::stringstream sig_ss;
    for (size_t i = 0; i < sig_len; ++i) {
        sig_ss << std::hex << (int)signature[i];
    }
    std::cout << "签名 (HEX): " << sig_ss.str() << "\n";

    // 释放摘要签名上下文对象
    EVP_MD_CTX_free(mdctx);

    return std::string((char*)signature, sig_len);
}

// 使用公钥验证签名
bool verify_signature(const std::string& message, const std::string& signature, EVP_PKEY* pkey) {
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) handleErrors();

    // 初始化签名验证操作
    if (EVP_DigestVerifyInit(mdctx, NULL, EVP_sha256(), NULL, pkey) <= 0)
        handleErrors();

    // 更新要验证的消息数据
    if (EVP_DigestVerifyUpdate(mdctx, message.c_str(), message.size()) <= 0)
        handleErrors();

    // 执行签名验证，并获取结果
    int result = EVP_DigestVerifyFinal(mdctx, (const unsigned char*)signature.data(), signature.size());

    // 打印出验证签名时的消息散列值（十六进制格式）
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, message.c_str(), message.size());
    SHA256_Final(hash, &sha256);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << (int)hash[i];
    }
    std::cout << "接收到的消息散列值: " << ss.str() << "\n";

    // 释放摘要签名上下文对象
    EVP_MD_CTX_free(mdctx);

    return result == 1;
}

int main() {
    // 生成 RSA 密钥对
    EVP_PKEY* pkey = generate_rsa_key(2048);
    std::cout << "RSA 密钥对已生成。\n";

    // 提取公钥和私钥并转换为字符串表示
    BIO* pri_bio = BIO_new(BIO_s_mem()); // 创建一个内存BIO对象用于保存私钥
    BIO* pub_bio = BIO_new(BIO_s_mem()); // 创建一个内存BIO对象用于保存公钥
    PEM_write_bio_PrivateKey(pri_bio, pkey, NULL, NULL, 0, NULL, NULL); // 将私钥写入BIO对象
    PEM_write_bio_PUBKEY(pub_bio, pkey); // 将公钥写入BIO对象

    // 将密钥保存到文件或字符串中
    char* private_key_str;
    char* public_key_str;
    long pri_len = BIO_get_mem_data(pri_bio, &private_key_str); // 获取私钥字符串及其长度
    long pub_len = BIO_get_mem_data(pub_bio, &public_key_str); // 获取公钥字符串及其长度

    // 创建新的 EVP_PKEY 对象用于签名和验证
    EVP_PKEY* pkey_private = PEM_read_bio_PrivateKey(BIO_new_mem_buf(private_key_str, pri_len), NULL, NULL, NULL);
    EVP_PKEY* pkey_public = PEM_read_bio_PUBKEY(BIO_new_mem_buf(public_key_str, pub_len), NULL, NULL, NULL);
    std::cout << "密钥读取完成。\n";

    // 测试消息
    std::string message = "hello ahwei";
    std::cout << "原始消息: " << message << "\n";

    // 签名消息
    std::string signature = sign_message(message, pkey_private);
    std::cout << "签名大小: " << signature.size() << " bytes\n";

    // 验证签名
    if (verify_signature(message, signature, pkey_public)) {
        std::cout << "签名验证成功。\n";
    } else {
        std::cout << "签名验证失败。\n";
    }

    // 清理资源
    EVP_PKEY_free(pkey);
    EVP_PKEY_free(pkey_private);
    EVP_PKEY_free(pkey_public);
    BIO_free_all(pri_bio);
    BIO_free_all(pub_bio);

    return 0;
}