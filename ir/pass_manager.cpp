#include "ir.h"

const IR::Node *PassManager::apply_visitor(const IR::Node *program, const char *) {
    vector<std::pair<vector<Visitor *>::iterator, const IR::Node *>> backup;

    for (auto it = passes.begin(); it != passes.end();) {
        if (dynamic_cast<Backtrack *>(*it))
            backup.emplace_back(it, program);
        try {
            try {
                LOG1("Invoking " << (*it)->name());
                program = program->apply(**it);
                int errors = ErrorReporter::instance.getErrorCount();
                if (stop_on_error && errors > 0) {
                    ::error("%1% errors ecountered, aborting compilation", errors);
                    program = nullptr;
                }
                if (program == nullptr) break;
            } catch (Backtrack::trigger::type_t trig_type) {
                throw Backtrack::trigger(trig_type);
            }
        } catch (Backtrack::trigger trig) {
            while (!backup.empty()) {
                if (backup.back().first == it) {
                    backup.pop_back();
                    continue; }
                it = backup.back().first;
                auto b = dynamic_cast<Backtrack *>(*it);
                program = backup.back().second;
                if (b->backtrack(trig))
                    break; }
            if (backup.empty())
                throw trig;
            continue; }
        it++; }
    return program;
}

const IR::Node *PassRepeated::apply_visitor(const IR::Node *program, const char *name) {
    bool done = false;
    unsigned iterations = 0;
    while (!done) {
        auto newprogram = PassManager::apply_visitor(program, name);
        if (program == newprogram || newprogram == nullptr)
            done = true;
        int errors = ErrorReporter::instance.getErrorCount();
        if (stop_on_error && errors > 0)
            return nullptr;
        iterations++;
        if (repeats != 0 && iterations > repeats)
            done = true;
        program = newprogram;
    }
    return program;
}
