// #pragma once

// #include "defines.h"

// #define KASSERTIONS_ENABLED

// #ifdef KASSERTIONS_ENABLED
// #if _MSC_VER
// #include <intrin.h>
// #define debugBreak() __debugbreak()
// #else
// #define debugBreak() __builtin_trap()
// #endif

// void report_assertion_failure(const char* expression, const char* message, const char* file, i32 line);

// #define KASSERT(expr)                                                \
//     {                                                                \
//         if (expr) {                                                  \
//         } else {                                                     \
//             report_assertion_failure(#expr, "", __FILE__, __LINE__); \
//             debugBreak();                                            \
//         }                                                            \
//     }

// #define KASSERT_MSG(expr, message)                                        \
//     {                                                                     \
//         if (expr) {                                                       \
//         } else {                                                          \
//             report_assertion_failure(#expr, message, __FILE__, __LINE__); \
//             debugBreak();                                                 \
//         }                                                                 \
//     }

// #ifdef _DEBUG
// #define KASSERT_DEBUG(expr)                                          \
//     {                                                                \
//         if (expr) {                                                  \
//         } else {                                                     \
//             report_assertion_failure(#expr, "", __FILE__, __LINE__); \
//             debugBreak();                                            \
//         }                                                            \
//     }
// #else
// #define KASSERT_DEBUG(expr)  // nop
// #endif

// #else

// #define KASSERT(expr)               // nop
// #define KASSERT_MSG(expr, message)  // nop
// #define KASSERT_DEBUG(expr)         // nop

// #endif