#pragma once

#include "input_types.h"
#include "llist.h"
#include "btree.h"
#include "hash_table.h"

class LangPhrase
{
  public:
    char* m_key; // malloced. The name used as an ID
    char* m_string; // malloced. This bit is different for each language

    LangPhrase();
    ~LangPhrase();
};

class LangTable
{
  LangPhrase m_notFound;
  BTree<LangPhrase*> m_phrasesRaw;
  HashTable<int>* m_phrasesKbd;
  HashTable<int>* m_phrasesXin;
  char* m_chunk;

  bool specific_key_exists(const char* _key, InputMode _mood);
  bool RawDoesPhraseExist(const char* _key);
  HashTable<int>* GetCurrentTable();
  HashTable<int>* GetCurrentTable(InputMode _mood);

  void RebuildTable(HashTable<int>* _phrases, std::ostringstream& stream, InputMode _mood);

  public:
    LangTable(const char* _filename);
    ~LangTable();

    void ParseLanguageFile(const char* _filename);
    void RebuildTables();

    bool DoesPhraseExist(const char* _key);
    char* LookupPhrase(const char* _key);

    char* RawLookupPhrase(const char* _key);

    bool RawDoesPhraseExist(const char* _key, InputMode _mood);
    char* RawLookupPhrase(const char* _key, InputMode _mood);

    DArray<LangPhrase*>* GetPhraseList();

    void TestAgainstEnglish();
};

LList<const char*>* WordWrapText(const char* _string, float _linewidth, float _fontWidth, bool _wrapToWindow = true);

#define LANGUAGEPHRASE(x)       g_context->m_langTable->LookupPhrase(x)
#define ISLANGUAGEPHRASE(x)     g_context->m_langTable->DoesPhraseExist(x)
#define ISLANGUAGEPHRASE_ANY(x) g_context->m_langTable->DoesPhraseExist(x)

#define RAWLANGUAGEPHRASE(x)           g_context->m_langTable->RawLookupPhrase(x)
#define MOODYLANGUAGEPHRASE(x,y)       g_context->m_langTable->RawLookupPhrase((x),(y))
#define MOODYISLANGUAGEPHRASE(x,y)     g_context->m_langTable->RawDoesPhraseExist((x),(y))
