#include "options.h"

namespace P4fmt {

P4fmtOptions::P4fmtOptions() {
    registerOption(
        "-o", "outfile",
        [this](const char *arg) {
            outFile = arg;
            return true;
        },
        "Write output to outfile");

    registerOption(
        "--with-frontend", nullptr,
        [this](const char *) {
            Fpass = true;
            return true;
        },
        "Use frontend pass");
    
    registerOption(
        "--with-midend", nullptr, 
        [this](const char *){
            Mpass = true;
            return true;
        },"with midend pass"
    );
    registerOption(
        "--print-IRnodes", nullptr,
        [this](const char *){
            ir = true;
            return true;
        }, "print Ir nodes"
    );
   }

} 