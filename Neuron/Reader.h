#ifndef READER_H
#define READER_H

class Reader
{
  public:
    Reader() {}
    virtual ~Reader() {}

    virtual std::string more() = 0;
};

class ConsoleReader : public Reader
{
  public:
    std::string more() override;

    void printPrimaryPrompt();
    void fillInputBuffer();

  private:
    char buffer[1000];
    char* p;
};

class FileReader : public Reader
{
  public:
    FileReader(const char* fname);
    std::string more() override;

  private:
    std::string filename;
    int done;
};

class BlockReader : public Reader
{
  public:
    BlockReader(std::string_view block);
    std::string more() override;

  private:
    std::string data;
    int done;
    int length;
};

#endif
