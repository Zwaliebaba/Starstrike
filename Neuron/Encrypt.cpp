#include "pch.h"
#include "Encrypt.h"

static long k[4] = {0x3B398E26, 0x40C29501, 0x614D7630, 0x7F59409A};

static void encypher(long* v)
{
  DWORD y = v[0];
  DWORD z = v[1];
  DWORD sum = 0;
  DWORD delta = 0x9e3779b9;                // a key schedule constant
  DWORD n = 32;                            // num iterations

  while (n-- > 0)
  {                        // basic cycle start
    sum += delta;
    y += (z << 4) + k[0] ^ z + sum ^ (z >> 5) + k[1];
    z += (y << 4) + k[2] ^ y + sum ^ (y >> 5) + k[3];
  }

  v[0] = y;
  v[1] = z;
}

static void decypher(long* v)
{
  DWORD y = v[0];
  DWORD z = v[1];
  DWORD sum = 0;
  DWORD delta = 0x9e3779b9;                // a key schedule constant
  DWORD n = 32;                            // num iterations

  sum = delta << 5;

  while (n-- > 0)
  {
    z -= (y << 4) + k[2] ^ y + sum ^ (y >> 5) + k[3];
    y -= (z << 4) + k[0] ^ z + sum ^ (z >> 5) + k[1];
    sum -= delta;
  }

  v[0] = y;
  v[1] = z;
}



std::string Encryption::Encrypt(std::string_view block)
{
  size_t len = block.length();

  if (len < 1)
    return {};

  // pad to eight byte chunks
  if (len & 0x7)
  {
    len /= 8;
    len *= 8;
    len += 8;
  }

  auto work = NEW BYTE[len];
  ZeroMemory(work, len);
  CopyMemory(work, block.data(), block.length());

  auto v = (long*)work;
  for (int i = 0; i < len / 8; i++)
  {
    encypher(v);
    v += 2;
  }

  std::string cypher((const char*)work, len);
  delete [] work;
  return cypher;
}



std::string Encryption::Decrypt(std::string_view block)
{
  int len = block.length();

  if (len & 0x7)
  {
    DebugTrace("WARNING: attempt to decrypt odd length block (len={})\n", len);
    return {};
  }

  auto work = NEW BYTE[len];
  CopyMemory(work, block.data(), len);

  auto v = (long*)work;
  for (int i = 0; i < len / 8; i++)
  {
    decypher(v);
    v += 2;
  }

  std::string clear((const char*)work, len);
  delete [] work;
  return clear;
}



static auto codes = "abcdefghijklmnop";

std::string Encryption::Encode(std::string_view block)
{
  size_t len = block.size() * 2;
  auto work = NEW char[len + 1];

  for (size_t i = 0; i < block.size(); i++)
  {
    BYTE b = static_cast<BYTE>(block[i]);
    work[2 * i] = codes[b >> 4 & 0xf];
    work[2 * i + 1] = codes[b & 0xf];
  }

  work[len] = 0;

  std::string code(work, len);
  delete [] work;
  return code;
}



std::string Encryption::Decode(std::string_view block)
{
  int len = block.length() / 2;
  auto work = NEW char[len + 1];

  for (int i = 0; i < len; i++)
  {
    char u = block[2 * i];
    char l = block[2 * i + 1];

    work[i] = (u - codes[0]) << 4 | (l - codes[0]);
  }

  work[len] = 0;

  std::string clear(work, len);
  delete [] work;
  return clear;
}
