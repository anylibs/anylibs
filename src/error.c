#include "anylibs/error.h"

char const *c_error_to_str(c_error_t code)
{
    switch (code)
    {
    case (C_ERROR_none):
        return "";
    case (C_ERROR_mem_allocation):
        return "memory allocation";
    case (C_ERROR_wrong_len):
        return "wrong len";
    case (C_ERROR_wrong_capacity):
        return "wrong capacity";
    case (C_ERROR_wrong_index):
        return "wrong index";
    case (C_ERROR_capacity_full):
        return "capacity is full";
    case (C_ERROR_empty):
        return "empty";
    case (C_ERROR_wrong_range):
        return "wrong range";
    default:
        return "";
    }
}
