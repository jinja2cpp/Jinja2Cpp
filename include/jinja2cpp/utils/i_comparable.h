#ifndef JINJA2CPP_ICOMPARABLE_H
#define JINJA2CPP_ICOMPARABLE_H


namespace jinja2 {

struct IComparable
{
    virtual ~IComparable() {}
    virtual bool IsEqual(const IComparable& other) const = 0;
};

} // namespace jinja2

#endif // JINJA2CPP_ICOMPARABLE_H
