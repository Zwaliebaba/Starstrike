#pragma once

class Director
{
  public:
    Director() = default;
    virtual ~Director() = default;

    // accessors:
    [[nodiscard]] virtual int Type() const { return 0; }
    [[nodiscard]] virtual int Subframe() const { return 0; }

    // operations
    virtual void ExecFrame([[maybe_unused]] double factor) {}
};
