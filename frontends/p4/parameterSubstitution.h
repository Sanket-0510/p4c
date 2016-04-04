#ifndef _IR_PARAMETERSUBSTITUTION_H_
#define _IR_PARAMETERSUBSTITUTION_H_

// This file must be included after ir/ir.h

#include <iostream>

#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/enumerator.h"

namespace IR {

/* Maps Parameters to Expressions */
class ParameterSubstitution {
    std::map<cstring, const IR::Expression*>  parameterValues;
    // Map from parameter name to parameter.
    std::map<cstring, const IR::Parameter*>   parameters;

 public:
    void add(const IR::Parameter* parameter, const IR::Expression* value) {
        LOG1("Mapping " << parameter << " to " << value);
        cstring name = parameter->name.name;
        auto par = get(parameters, name);
        BUG_CHECK(par == nullptr,
                  "Two parameters with the same name %1% in a substitution", name);
        parameterValues.emplace(name, value);
        parameters.emplace(name, parameter);
    }

    const IR::Expression* lookupName(cstring name) const {
        return get(parameterValues, name);
    }

    bool containsName(cstring name) const
    { return parameterValues.find(name) != parameterValues.end(); }

    ParameterSubstitution* append(const ParameterSubstitution* other) const {
        ParameterSubstitution* result = new ParameterSubstitution(*this);
        for (auto param : other->parameters)
            result->add(param.second, other->lookupName(param.first));
        return result;
    }

    const IR::Expression* lookup(const IR::Parameter* param) const
    { return lookupName(param->name.name); }

    bool contains(const IR::Parameter* param) const {
        if (!containsName(param->name.name))
            return false;
        auto it = parameters.find(param->name.name);
        if (param != it->second)
            return false;
        return true;
    }

    const IR::Parameter* getParameter(cstring name) const
    { return get(parameters, name); }

    bool empty() const
    { return parameterValues.empty(); }

    void populate(const IR::ParameterList* params,
                  const IR::Vector<IR::Expression>* args) {
        BUG_CHECK(params->size() == args->size(),
                  "Incompatible number of arguments for parameter list");

        auto pe = params->getEnumerator();
        for (auto a : *args) {
            bool success = pe->moveNext();
            BUG_CHECK(success, "Enumerator finished too soon");
            add(pe->getCurrent(), a);
        }
    }

    void dbprint(std::ostream& out) const {
        for (auto s : parameters)
            out << s.second << "=>" << lookupName(s.first) << std::endl;
    }
};

}  // namespace IR

#endif /* _IR_PARAMETERSUBSTITUTION_H_ */
