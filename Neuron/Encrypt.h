#ifndef Encrypt_h
#define Encrypt_h

class Encryption
{
  public:
    // private key encryption / decryption of 
    // arbitrary blocks of data
    static std::string Encrypt(std::string_view block);
    static std::string Decrypt(std::string_view block);

    // encode / decode binary blocks into
    // ascii strings for use in text files
    static std::string Encode(std::string_view block);
    static std::string Decode(std::string_view block);
};

#endif Encrypt_h
