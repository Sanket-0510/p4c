#include <stdlib.h>

#include <iostream>

#include "frontends/common/constantFolding.h"
#include "frontends/common/options.h"
#include "frontends/common/parseInput.h"
#include "frontends/common/parser_options.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/frontend.h"
#include "frontends/p4/toP4/toP4.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"
#include "ir/visitor.h"
#include "lib/compile_context.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "options.h"
#include "test/gtest/helpers.h"
#include "lib/nullstream.h"

namespace p4fmt {
class MidEnd : public PassManager {
    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;

 public:
    explicit MidEnd() {
        addPasses({
            new P4::TypeChecking(&refMap, &typeMap, true),
            new P4::ConstantFolding(&refMap, &typeMap),
        });
    }
};

}  
int main(int argc, char *const argv[]) {

    AutoCompileContext autoP4fmtContext(new P4fmt::P4fmtContext);
    auto &options = P4fmt::P4fmtContext::get().options();

    auto remainingOptions = options.process(argc,argv);
    if (remainingOptions == nullptr) {
        options.usage();
        return EXIT_FAILURE;
    }


    //set the inputfile using the serInpuFile() functions in parseOptions class
    options.setInputFile();

    //create a outputsteam  pointer we will parse this to the top4() for output stream option.
    std::ostream *outStream = nullptr;


    if (options.outFile.isNullOrEmpty()) {
        outStream = &std::cout;
    } else {
        outStream = openFile(options.outFile, false);
        if (!(*outStream)) {
            ::error(ErrorType::ERR_NOT_FOUND, "File not found: %s", options.outFile);
            options.usage();
            return EXIT_FAILURE;
        }
    }
   
    const IR::P4Program *program = P4::parseP4File(options);

    if (program == nullptr && ::errorCount() != 0) {
        return EXIT_FAILURE;
    }
    
  
    bool printIr = false;
    if(options.ir){
        printIr =true;
    }
    else {
        printIr = false;
    }
    auto top4 = P4::ToP4(outStream, printIr);

    if (options.Fpass) {
        std::cout<<"<--------------------------------WITH FRONTEND----------------------------> \n ";
        P4::FrontEnd fe;
        program = fe.run(options, program);
        program->apply(top4);

    } else if (options.Mpass) {
        std::cout<<"<--------------------------------WITH MIDEND------------------------------> \n ";
        program->apply(p4fmt::MidEnd());
        program->apply(top4);
    }else{
    std::cout<<"<-----------------------------Format Directly without any pass----------------> \n ";
    program->apply(top4);
    }


    return ::errorCount() > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}