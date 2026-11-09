#ifndef COUNTER_HPP
#define COUNTER_HPP

struct Counter
{
    template <typename T>
    struct Instance
    {
        static inline int counter = 0;
    };

    template <typename TypeIns, typename T>
    static int index()
    {
        static const int idx = Instance<TypeIns>::counter++;
        return idx;
    }
};

#endif
