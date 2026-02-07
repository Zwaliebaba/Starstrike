#pragma once

struct Point;
struct Rect;
struct Insets;
struct Matrix;
struct Vec3;
struct Quaternion;
struct Plane;

constexpr double PI = 3.14159265358979323846;
constexpr double DEGREES = (PI / 180);

struct Rect
{
  
  Rect()
    : x(0),
      y(0),
      w(0),
      h(0) {}

  Rect(int ix, int iy, int iw, int ih)
    : x(ix),
      y(iy),
      w(iw),
      h(ih) {}

  int operator==(const Rect& r) const { return x == r.x && y == r.y && w == r.w && h == r.h; }
  int operator!=(const Rect& r) const { return x != r.x || y != r.y || w != r.w || h != r.h; }

  void Inflate(int dw, int dh);
  void Deflate(int dw, int dh);
  void Inset(int left, int right, int top, int bottom);
  int Contains(int x, int y) const;

  int x, y, w, h;
};

struct Insets
{
  Insets()
    : left(0),
      right(0),
      top(0),
      bottom(0) {}

  Insets(WORD l, WORD r, WORD t, WORD b)
    : left(l),
      right(r),
      top(t),
      bottom(b) {}

  WORD left;
  WORD right;
  WORD top;
  WORD bottom;
};

struct Matrix
{
  
  Matrix();
  Matrix(const Matrix& m);
  Matrix(const Point& vrt, const Point& vup, const Point& vpn);

  Matrix& operator =(const Matrix& m);
  Matrix& operator *=(const Matrix& m);

  double operator()(int i, int j) const { return elem[i][j]; }
  double& operator()(int i, int j) { return elem[i][j]; }

  void Identity();
  void Transpose();
  void Rotate(double roll, double pitch, double yaw);
  void Roll(double roll);
  void Pitch(double pitch);
  void Yaw(double yaw);
  void ComputeEulerAngles(double& roll, double& pitch, double& yaw) const;

  double Cofactor(int i, int j) const;
  void Invert();

  Matrix Inverse() const
  {
    Matrix result(*this);
    result.Invert();
    return result;
  }

  Matrix operator*(const Matrix& m) const;
  Point operator*(const Point& p) const;
  Vec3 operator*(const Vec3& v) const;

  double elem[3][3];

  private:
    Matrix([[maybe_unused]] int no_init) {}
};

struct Vec2
{
  
  Vec2() {}

  Vec2(int ix, int iy)
    : x(static_cast<float>(ix)),
      y(static_cast<float>(iy)) {}

  Vec2(float ix, float iy)
    : x(ix),
      y(iy) {}

  Vec2(double ix, double iy)
    : x(static_cast<float>(ix)),
      y(static_cast<float>(iy)) {}

  // operator void*() const { return static_cast<void*>(x || y); }
  int operator==(const Vec2& p) const { return x == p.x && y == p.y; }
  int operator!=(const Vec2& p) const { return x != p.x || y != p.y; }
  Vec2 operator+(const Vec2& p) const { return Vec2(x + p.x, y + p.y); }
  Vec2 operator-(const Vec2& p) const { return Vec2(x - p.x, y - p.y); }
  Vec2 operator-() const { return Vec2(-x, -y); }
  Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
  Vec2 operator/(float s) const { return Vec2(x / s, y / s); }
  float operator*(const Vec2& p) const { return (x * p.x + y * p.y); }

  Vec2& operator=(const Vec2& p)
  {
    x = p.x;
    y = p.y;
    return *this;
  }

  Vec2& operator+=(const Vec2& p)
  {
    x += p.x;
    y += p.y;
    return *this;
  }

  Vec2& operator-=(const Vec2& p)
  {
    x -= p.x;
    y -= p.y;
    return *this;
  }

  Vec2& operator*=(float s)
  {
    x *= s;
    y *= s;
    return *this;
  }

  Vec2& operator/=(float s)
  {
    x /= s;
    y /= s;
    return *this;
  }

  float length() const { return sqrt(x * x + y * y); }
  float Normalize();
  float dot(const Vec2& p) const { return (x * p.x + y * p.y); }
  Vec2 normal() const { return Vec2(-y, x); }

  float x, y;
};

struct Vec3
{
  
  Vec3() {}

  Vec3(int ix, int iy, int iz)
    : x(static_cast<float>(ix)),
      y(static_cast<float>(iy)),
      z(static_cast<float>(iz)) {}

  Vec3(float ix, float iy, float iz)
    : x(ix),
      y(iy),
      z(iz) {}

  Vec3(double ix, double iy, double iz)
    : x(static_cast<float>(ix)),
      y(static_cast<float>(iy)),
      z(static_cast<float>(iz)) {}

  operator bool() const { return x || y || z; }
  int operator==(const Vec3& p) const { return x == p.x && y == p.y && z == p.z; }
  int operator!=(const Vec3& p) const { return x != p.x || y != p.y || z != p.z; }
  Vec3 operator+(const Vec3& p) const { return Vec3(x + p.x, y + p.y, z + p.z); }
  Vec3 operator-(const Vec3& p) const { return Vec3(x - p.x, y - p.y, z - p.z); }
  Vec3 operator-() const { return Vec3(-x, -y, -z); }
  Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
  Vec3 operator/(float s) const { return Vec3(x / s, y / s, z / s); }
  float operator*(const Vec3& p) const { return (x * p.x + y * p.y + z * p.z); }
  float operator*(const Point& p) const;

  Vec3 operator*(const Matrix&) const;

  Vec3& operator=(const Vec3& p)
  {
    x = p.x;
    y = p.y;
    z = p.z;
    return *this;
  }

  Vec3& operator+=(const Vec3& p)
  {
    x += p.x;
    y += p.y;
    z += p.z;
    return *this;
  }

  Vec3& operator-=(const Vec3& p)
  {
    x -= p.x;
    y -= p.y;
    z -= p.z;
    return *this;
  }

  Vec3& operator*=(float s)
  {
    x *= s;
    y *= s;
    z *= s;
    return *this;
  }

  Vec3& operator/=(float s)
  {
    x /= s;
    y /= s;
    z /= s;
    return *this;
  }

  void SwapYZ()
  {
    float t = y;
    y = z;
    z = t;
  }

  float length() const { return sqrt(x * x + y * y + z * z); }
  float Normalize();

  float dot(const Vec3& p) const { return (x * p.x + y * p.y + z * p.z); }
  Vec3 cross(const Vec3& v) const { return Vec3((y * v.z) - (z * v.y), (z * v.x) - (x * v.z), (x * v.y) - (y * v.x)); }

  float x, y, z;
};

double ClosestApproachTime(const Vec3& loc1, const Vec3& vel1, const Vec3& loc2, const Vec3& vel2);

struct Point
{
  
  Point()
    : x(0),
      y(0),
      z(0) {}

  Point(double ix, double iy, double iz)
    : x(ix),
      y(iy),
      z(iz) {}

  Point(const Point& p)
    : x(p.x),
      y(p.y),
      z(p.z) {}

  Point(const Vec3& v)
    : x(v.x),
      y(v.y),
      z(v.z) {}

  operator Vec3() const { return Vec3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)); }

  operator bool() const { return x || y || z; }
  int operator==(const Point& p) const { return x == p.x && y == p.y && z == p.z; }
  int operator!=(const Point& p) const { return x != p.x || y != p.y || z != p.z; }
  Point operator+(const Point& p) const { return Point(x + p.x, y + p.y, z + p.z); }
  Point operator-(const Point& p) const { return Point(x - p.x, y - p.y, z - p.z); }
  Point operator-() const { return Point(-x, -y, -z); }
  Point operator*(double s) const { return Point(x * s, y * s, z * s); }
  Point operator/(double s) const { return Point(x / s, y / s, z / s); }
  Point operator*(float s) const { return Point(x * s, y * s, z * s); }
  Point operator/(float s) const { return Point(x / s, y / s, z / s); }
  double operator*(const Point& p) const { return (x * p.x + y * p.y + z * p.z); }
  Point operator*(const Matrix& m) const;

  Point& operator=(const Point& p)
  {
    x = p.x;
    y = p.y;
    z = p.z;
    return *this;
  }

  Point& operator+=(const Point& p)
  {
    x += p.x;
    y += p.y;
    z += p.z;
    return *this;
  }

  Point& operator-=(const Point& p)
  {
    x -= p.x;
    y -= p.y;
    z -= p.z;
    return *this;
  }

  Point& operator*=(double s)
  {
    x *= s;
    y *= s;
    z *= s;
    return *this;
  }

  Point& operator/=(double s)
  {
    x /= s;
    y /= s;
    z /= s;
    return *this;
  }

  double length() const { return sqrt(x * x + y * y + z * z); }
  double Normalize();

  void SwapYZ()
  {
    double t = y;
    y = z;
    z = t;
  }

  Point OtherHand() const { return Point(-x, z, y); }

  void SetElement(int i, double v);

  double dot(const Point& p) const { return (x * p.x + y * p.y + z * p.z); }

  Point cross(const Point& p) const
  {
    return Point((y * p.z) - (z * p.y), (z * p.x) - (x * p.z), (x * p.y) - (y * p.x));
  }

  double x, y, z;
};

double ClosestApproachTime(const Point& loc1, const Point& vel1, const Point& loc2, const Point& vel2);

struct Quaternion
{
  
  Quaternion()
    : x(0),
      y(0),
      z(0),
      w(0) {}

  Quaternion(double ix, double iy, double iz, double iw)
    : x(ix),
      y(iy),
      z(iz),
      w(iw) {}

  Quaternion(const Quaternion& q)
    : x(q.x),
      y(q.y),
      z(q.z),
      w(q.w) {}

  int operator==(const Quaternion& q) const { return x == q.x && y == q.y && z == q.z && w == q.w; }
  int operator!=(const Quaternion& q) const { return x != q.x || y != q.y || z != q.z || w != q.w; }

  Quaternion operator+(const Quaternion& q) const { return Quaternion(x + q.x, y + q.y, z + q.z, w + q.w); }
  Quaternion operator-(const Quaternion& q) const { return Quaternion(x - q.x, y - q.y, z - q.z, w - q.w); }
  Quaternion operator-() const { return Quaternion(-x, -y, -z, -w); }
  Quaternion operator*(double s) const { return Quaternion(x * s, y * s, z * s, w * s); }
  Quaternion operator/(double s) const { return Quaternion(x / s, y / s, z / s, w / s); }

  Quaternion& operator=(const Quaternion& q)
  {
    x = q.x;
    y = q.y;
    z = q.z;
    w = q.w;
    return *this;
  }

  Quaternion& operator+=(const Quaternion& q)
  {
    x += q.x;
    y += q.y;
    z += q.z;
    w += q.w;
    return *this;
  }

  Quaternion& operator-=(const Quaternion& q)
  {
    x -= q.x;
    y -= q.y;
    z -= q.z;
    w -= q.w;
    return *this;
  }

  Quaternion& operator*=(double s)
  {
    x *= s;
    y *= s;
    z *= s;
    w *= s;
    return *this;
  }

  Quaternion& operator/=(double s)
  {
    x /= s;
    y /= s;
    z /= s;
    w /= s;
    return *this;
  }

  double length() const { return sqrt(x * x + y * y + z * z + w * w); }
  double Normalize();

  double x, y, z, w;
};

struct Plane
{
  
  Plane();
  Plane(const Point& p0, const Point& p1, const Point& p2);
  Plane(const Vec3& v0, const Vec3& v1, const Vec3& v2);

  void Rotate(const Vec3& v0, const Matrix& m);
  void Translate(const Vec3& v0);

  float distance;
  Vec3 normal;
};

double DotProduct(const Point& a, const Point& b);
void CrossProduct(const Point& a, const Point& b, Point& out);
void MConcat(double in1[3][3], double in2[3][3], double out[3][3]);

int lines_intersect(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4, double& x,
                    double& y);
