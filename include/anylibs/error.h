#ifndef ANYLIBS_ERROR_H
#define ANYLIBS_ERROR_H

typedef enum c_error_t
{
    C_ERROR_none,
    C_ERROR_mem_allocation = 256,
    C_ERROR_wrong_len,
    C_ERROR_wrong_capacity,
    C_ERROR_wrong_index,
    C_ERROR_capacity_full,
    C_ERROR_empty,
    C_ERROR_wrong_range,
} c_error_t;

char const *c_error_to_str(c_error_t code);

#endif // ANYLIBS_ERROR_H
