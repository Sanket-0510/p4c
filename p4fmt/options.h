#ifndef BACKENDS_P4FMT_OPTIONS_H_
#define BACKENDS_P4FMT_OPTIONS_H_

#include "frontends/common/options.h"
#include "frontends/common/parser_options.h"

namespace P4fmt {

class P4fmtOptions : public CompilerOptions {
 public:
      
    bool Fpass = false;
    bool Mpass = false;
    bool ir = false;
    bool visitor = false;
    bool enableSameFile = false;
    P4fmtOptions();
    virtual ~P4fmtOptions() = default;

    P4fmtOptions(const P4fmtOptions &) = default;
    P4fmtOptions(P4fmtOptions &&) = delete;
    P4fmtOptions &operator=(const P4fmtOptions &) = default;
    P4fmtOptions &operator=(P4fmtOptions &&) = delete;

    cstring outFile = nullptr;
 
};

using P4fmtContext = P4CContextWithOptions<P4fmtOptions>;

} 

#endif 