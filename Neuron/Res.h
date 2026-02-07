#ifndef Res_h
#define Res_h

class Resource
{
  public:
    Resource();
    virtual ~Resource();

    int operator ==(const Resource& r) const { return id == r.id; }

    HANDLE Handle() const { return id; }

  protected:
    HANDLE id;
};

#endif Res_h
