#ifndef _FRONTENDS_P4_FROMV1_0_PROGRAMSTRUCTURE_H_
#define _FRONTENDS_P4_FROMV1_0_PROGRAMSTRUCTURE_H_

#include <set>
#include <vector>

#include "lib/map.h"
#include "lib/cstring.h"
#include "frontends/p4/coreLibrary.h"
#include "ir/ir.h"
#include "frontends/p4/callGraph.h"
#include "v1model.h"

namespace P4V1 {

// Information about the structure of a P4 v1.0 program
class ProgramStructure {
    // In P4 v1.0 one can have multiple objects with different types with the same name
    // In P4 v1.2 this is not possible, so we may need to rename some objects.
    // We will preserve the original name using an @name("") annotation.
    template<typename T>
    class NamedObjectInfo {
        // If allNames is nullptr we don't check for duplicate names
        std::unordered_set<cstring> *allNames;
        std::map<cstring, T> nameToObject;
        std::map<T, cstring> objectToNewName;

        // Iterate in order of name, but return pair<T, newname>
        class iterator {
            friend class NamedObjectInfo;
         private:
            typename std::map<cstring, T>::iterator it;
            typename std::map<T, cstring> &objToName;
            iterator(typename std::map<cstring, T>::iterator it,
                     typename std::map<T, cstring> &objToName) :
                    it(it), objToName(objToName) {}
         public:
            const iterator& operator++() { ++it; return *this; }
            bool operator!=(const iterator& other) const { return it != other.it; }
            std::pair<T, cstring> operator*() const {
                return std::pair<T, cstring>(it->second, objToName[it->second]);
            }
        };

     public:
        explicit NamedObjectInfo(std::unordered_set<cstring>* allNames) : allNames(allNames) {}
        void emplace(T obj) {
            if (objectToNewName.find(obj) != objectToNewName.end())
                // Already done
                return;

            LOG1("Discovered " << obj);
            nameToObject.emplace(obj->name, obj);
            cstring newName;

            if (allNames == nullptr ||
                (allNames->find(obj->name) == allNames->end())) {
                newName = obj->name;
            } else {
                newName = cstring::make_unique(*allNames, obj->name, '_');
                // ::warning("Renaming %1% to %2%", obj, newName);
                // The old name will be stored in an annotation.
            }
            if (allNames != nullptr)
                allNames->emplace(newName);
            objectToNewName.emplace(obj, newName);
        }
        // Lookup using the original name
        T get(cstring name) const { return ::get(nameToObject, name); }
        // Get the new name
        cstring get(T object) const { return ::get(objectToNewName, object); }
        bool contains(cstring name) const { return nameToObject.find(name) != nameToObject.end(); }
        iterator begin() { return iterator(nameToObject.begin(), objectToNewName); }
        iterator end() { return iterator(nameToObject.end(), objectToNewName); }
    };

 public:
    ProgramStructure();

    P4V1::V1Model &v1model;
    P4::P4CoreLibrary &p4lib;

    std::unordered_set<cstring>                 allNames;
    NamedObjectInfo<const IR::Type_StructLike*> types;
    NamedObjectInfo<const IR::Metadata*>        metadata;
    NamedObjectInfo<const IR::Header*>          headers;
    NamedObjectInfo<const IR::HeaderStack*>     stacks;
    NamedObjectInfo<const IR::Control*>         controls;
    NamedObjectInfo<const IR::Parser*>          parserStates;
    NamedObjectInfo<const IR::Table*>           tables;
    NamedObjectInfo<const IR::ActionFunction*>  actions;
    NamedObjectInfo<const IR::Counter*>         counters;
    NamedObjectInfo<const IR::Register*>        registers;
    NamedObjectInfo<const IR::Meter*>           meters;
    NamedObjectInfo<const IR::ActionProfile*>   action_profiles;
    NamedObjectInfo<const IR::FieldList*>       field_lists;
    NamedObjectInfo<const IR::FieldListCalculation*> field_list_calculations;
    NamedObjectInfo<const IR::ActionSelector*>  action_selectors;
    std::vector<const IR::CalculatedField*>     calculated_fields;
    P4::CallGraph<cstring> calledActions;
    P4::CallGraph<cstring> calledControls;
    P4::CallGraph<cstring> calledCounters;
    P4::CallGraph<cstring> calledMeters;
    P4::CallGraph<cstring> calledRegisters;
    P4::CallGraph<cstring> parsers;
    std::map<cstring, IR::Vector<IR::Expression>> extracts;  // for each parser
    std::map<cstring, cstring> directCounters;  // map table to direct counter
    // map table name to direct meter
    std::map<cstring, const IR::Meter*> directMeters;
    std::map<const IR::Meter*, const IR::Declaration_Instance*> meterMap;

    std::map<const IR::Table*, const IR::Control*> tableMapping;
    std::map<const IR::Table*, const IR::Apply*> tableInvocation;

    struct ConversionContext {
        const IR::Expression* header;
        const IR::Expression* userMetadata;
        const IR::Expression* standardMetadata;
        void clear() {
            header = nullptr;
            userMetadata = nullptr;
            standardMetadata = nullptr;
        }
    };

    ConversionContext conversionContext;
    IR::Vector<IR::Type>* emptyTypeArguments;
    const IR::Parameter* parserPacketIn;
    const IR::Parameter* parserHeadersOut;

    // output is constructed here
    IR::Vector<IR::Node>* declarations;

 protected:
    cstring makeUniqueName(cstring base);

    void include(cstring filename);
    const IR::Statement* convertPrimitive(const IR::Primitive* primitive);
    void checkHeaderType(const IR::Type_StructLike* hrd, bool toStruct);
    const IR::Annotations* addNameAnnotation(cstring name, const IR::Annotations* annos = nullptr);
    const IR::ParserState* convertParser(const IR::Parser* prs);
    const IR::Statement* convertParserStatement(const IR::Expression* expr);
    const IR::ControlContainer* convertControl(const IR::Control* control, cstring newName);
    const IR::Declaration_Instance* convertDirectMeter(const IR::Meter* m, cstring newName);
    const IR::Declaration_Instance* convert(const IR::CounterOrMeter* cm, cstring newName);
    const IR::Declaration_Instance* convert(const IR::Register* reg, cstring newName);
    const IR::TableContainer*
    convertTable(const IR::Table* table, cstring newName,
                 IR::NameMap<IR::Declaration, ordered_map>* stateful);
    const IR::ActionContainer* convertAction(const IR::ActionFunction* action, cstring newName, const IR::Meter* meterToAccess);
    const IR::Type_Control* controlType(IR::ID name);
    const IR::PathExpression* getState(IR::ID dest);
    const IR::Declaration_Instance* checksumUnit(const IR::FieldListCalculation* flc);
    const IR::FieldList* getFieldLists(const IR::FieldListCalculation* flc);
    const IR::Expression* convertFieldList(const IR::Expression* expression);
    const IR::Type_Struct* createFieldListType(const IR::Expression* expression);
    const IR::Expression* convertHashAlgorithm(IR::ID algorithm);
    const IR::Statement* sliceAssign(Util::SourceInfo srcInfo, const IR::Expression* left,
                                     const IR::Expression* right, const IR::Expression* mask);
    const IR::Expression* counterType(const IR::CounterOrMeter* cm) const;
    cstring mapAlgorithm(IR::ID algorithm) const;
    void createChecksumVerifications();
    void createChecksumUpdates();
    void createStructures();
    void createTypes();
    void createParser();
    void createControls();
    void createDeparser();
    void createMain();

 public:
    const IR::Expression* paramReference(const IR::Parameter* param);
    void tablesReferred(const IR::Control* control, std::vector<const IR::Table*> &out);
    bool isHeader(const IR::ConcreteHeaderRef* nhr) const;

    const IR::Control* ingress;
    IR::ID ingressReference;
    
    const IR::ControlContainer* verifyChecksums;
    const IR::ControlContainer* updateChecksums;
    const IR::ControlContainer* deparser;
    // Latest extraction
    const IR::Expression* latest;
    const int defaultRegisterWidth = 32;

    void loadModel();
    const IR::P4Program* create(Util::SourceInfo info);
};

}  // namespace P4V1

#endif /* _FRONTENDS_P4_FROMV1_0_PROGRAMSTRUCTURE_H_ */
