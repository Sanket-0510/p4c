#include "ir.h"

class IRDumper : public Inspector {
    std::ostream                &out;
    std::set<const IR::Node *>  dumped;
    unsigned                    maxdepth;
    cstring                     ignore;
    bool preorder(const IR::Node *n) override {
        if (auto ctxt = getContext()) {
            if (unsigned(ctxt->depth) > maxdepth)
                return false;
            if (ctxt->child_name && ignore == ctxt->child_name)
                return false;
            out << indent_t(ctxt->depth);
            if (ctxt->child_name)
                out << ctxt->child_name << ": "; }
        out << "[" << n->id << "] " << n->node_type_name();
        n->dump_fields(out);
        if (dumped.count(n)) {
            out << "..." << std::endl;
            return false; }
        dumped.insert(n);
        out << std::endl;
        return true; }
    void postorder(const IR::Node *n) override {
        if (getChildrenVisited() == 0)
            dumped.erase(n); }

 public:
    IRDumper(std::ostream &o, unsigned m, cstring ign) : out(o), maxdepth(m), ignore(ign)
    { visitDagOnce = false; }
};

void dump(std::ostream &out, const IR::Node *n, unsigned maxdepth) {
    n->apply(IRDumper(out, maxdepth, nullptr)); }
void dump(std::ostream &out, const IR::Node *n) { dump(out, n, ~0U); }
void dump(const IR::Node *n, unsigned maxdepth) { dump(std::cout, n, maxdepth); }
void dump(const IR::Node *n) { dump(n, ~0U); }
void dump_notype(const IR::Node *n) { n->apply(IRDumper(std::cout, ~0U, "type")); }

void dump(uintptr_t p, unsigned maxdepth) {
    dump(reinterpret_cast<const IR::Node *>(p), maxdepth); }
void dump(uintptr_t p) { dump(p, ~0U); }

void dump(const Visitor::Context *ctxt) {
    if (!ctxt) return;
    dump(ctxt->parent);
    std::cout << indent_t(ctxt->depth-1);
    if (ctxt->original != ctxt->node) {
        std::cout << "<" << static_cast<const void *>(ctxt->original) << ":["
                  << ctxt->original->id << "] " << ctxt->original->node_type_name();
        ctxt->original->dump_fields(std::cout);
        std::cout << std::endl << indent_t(ctxt->depth-1) << ">"; }
    std::cout << static_cast<const void *>(ctxt->node) << ":[" << ctxt->node->id << "] "
              << ctxt->node->node_type_name();
    ctxt->node->dump_fields(std::cout);
    std::cout << std::endl;
}

std::string dumpToString(const IR::Node* n) {
    std::stringstream str;
    dump(n); return str.str(); }
